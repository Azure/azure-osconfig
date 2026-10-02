// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryWorker.h"
#include "TelemetryWorkerProtocol.h"
#include "TelemetryResolverProtocol.h"
#include "TelemetryDeadline.h"
#include "TelemetryEvent.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char** environ;

_Static_assert(sizeof(TelemetryWorkerFrame) == 32, "Unexpected worker frame layout");

struct TelemetryWorker
{
    char* path;
    int descriptor;
    pid_t child;
    uint32_t sequence;
    int64_t lifetimeDeadline;
    int64_t budget;
    int64_t operationLimit;
    bool suppressed;
};

static int OperationDeadline(TelemetryWorker* worker, int64_t* start, int64_t* deadline)
{
    int status = TelemetryMonotonicTime(start);
    if (0 != status)
    {
        worker->budget = 0;
        return status;
    }
    if ((worker->budget <= 0) || (*start >= worker->lifetimeDeadline))
    {
        return ETIMEDOUT;
    }
    int64_t limit = (worker->budget < worker->operationLimit) ? worker->budget : worker->operationLimit;
    if (limit > worker->lifetimeDeadline - *start)
    {
        limit = worker->lifetimeDeadline - *start;
    }
    *deadline = *start + limit;
    return 0;
}

static int ChargeBudget(TelemetryWorker* worker, int64_t start, OsConfigLogHandle log)
{
    int64_t now = 0;
    int status = TelemetryMonotonicTime(&now);
    if (0 != status)
    {
        worker->budget = 0;
        OsConfigLogInfo(log, "TelemetryWorker: Cannot account for elapsed time (status=%d)", status);
        return status;
    }
    int64_t elapsed = now - start;
    if (elapsed < 0)
    {
        worker->budget = 0;
        OsConfigLogError(log, "TelemetryWorker: Invalid elapsed-time interval");
        return EINVAL;
    }
    worker->budget = (elapsed >= worker->budget) ? 0 : worker->budget - elapsed;
    return 0;
}

static int ReleaseChild(TelemetryWorker* worker, int64_t gracefulDeadline, OsConfigLogHandle log)
{
    int status = 0;
    if (worker->descriptor >= 0)
    {
        if (0 != close(worker->descriptor))
        {
            status = errno;
            OsConfigLogInfo(log, "TelemetryWorker: Cannot close IPC (status=%d)", status);
        }
        worker->descriptor = -1;
    }
    if (worker->child <= 0)
    {
        return status;
    }

    for (;;)
    {
        int childStatus = 0;
        pid_t waited = waitpid(worker->child, &childStatus, WNOHANG);
        if (worker->child == waited)
        {
            worker->child = -1;
            if (!WIFEXITED(childStatus) || (0 != WEXITSTATUS(childStatus)))
            {
                OsConfigLogInfo(log, "TelemetryWorker: Child ended abnormally (wait status=%d)", childStatus);
                return status ? status : EIO;
            }
            return status;
        }
        if ((waited < 0) && (EINTR != errno))
        {
            int error = errno;
            OsConfigLogInfo(log, "TelemetryWorker: Cannot observe owned child (status=%d)", error);
            if (ECHILD == error)
            {
                // The host consumed the child: its PID must not be signaled.
                worker->child = -1;
            }
            return status ? status : error;
        }
        if (gracefulDeadline <= 0)
        {
            break;
        }
        int remaining = 0;
        int timingStatus = TelemetryDeadlineRemaining(gracefulDeadline, &remaining);
        if (0 != timingStatus)
        {
            if (ETIMEDOUT != timingStatus)
            {
                OsConfigLogInfo(log, "TelemetryWorker: Cannot time child cleanup (status=%d)", timingStatus);
                if (0 == status)
                {
                    status = timingStatus;
                }
            }
            break;
        }
        if (poll(NULL, 0, (remaining > 10) ? 10 : remaining) < 0 && EINTR != errno)
        {
            status = errno;
            OsConfigLogInfo(log, "TelemetryWorker: Cannot wait for child exit (status=%d)", status);
            break;
        }
    }

    if ((0 != kill(worker->child, SIGKILL)) && (ESRCH != errno))
    {
        int error = errno;
        OsConfigLogInfo(log, "TelemetryWorker: Cannot terminate owned child (status=%d)", error);
        return status ? status : error;
    }
    pid_t waited;
    do
    {
        waited = waitpid(worker->child, NULL, 0);
    } while ((waited < 0) && (EINTR == errno));
    if (waited < 0)
    {
        int error = errno;
        OsConfigLogInfo(log, "TelemetryWorker: Cannot reap owned child (status=%d)", error);
        if (ECHILD == error)
        {
            worker->child = -1;
        }
        return status ? status : error;
    }
    worker->child = -1;
    OsConfigLogInfo(log, "TelemetryWorker: Terminated and reaped child");
    return status ? status : ((gracefulDeadline > 0) ? ETIMEDOUT : 0);
}

static int Transfer(int descriptor, void* buffer, size_t size, bool writing, int64_t deadline)
{
    unsigned char* bytes = buffer;
    size_t offset = 0;
    while (offset < size)
    {
        int remaining;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status)
        {
            return status;
        }
        ssize_t count = writing ?
            send(descriptor, bytes + offset, size - offset, MSG_NOSIGNAL) :
            recv(descriptor, bytes + offset, size - offset, 0);
        if (count > 0)
        {
            offset += (size_t)count;
            continue;
        }
        if (0 == count)
        {
            status = TelemetryDeadlineRemaining(deadline, &remaining);
            return status ? status : ((0 == offset) ? EPIPE : EPROTO);
        }
        if (EINTR == errno)
        {
            continue;
        }
        if ((EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
            return errno;
        }
        struct pollfd item = {descriptor, (short)(writing ? POLLOUT : POLLIN), 0};
        if ((poll(&item, 1, remaining) < 0) && (EINTR != errno))
        {
            return errno;
        }
    }
    return 0;
}

static int ReceiveReply(TelemetryWorker* worker, uint32_t operation, uint32_t sequence,
    void* body, size_t size, int64_t deadline)
{
    TelemetryWorkerFrame reply = {0};
    int status = Transfer(worker->descriptor, &reply, sizeof(reply), false, deadline);
    if (0 != status)
    {
        return status;
    }
    if ((reply.magic != TELEMETRY_WORKER_MAGIC) || (reply.version != TELEMETRY_WORKER_VERSION) ||
        (reply.operation != operation) || (reply.sequence != sequence) || (reply.deadline != 0) ||
        (reply.status < 0) || (reply.size != ((0 != reply.status) ? 0 : size)))
    {
        return EPROTO;
    }
    if (0 != reply.status)
    {
        return reply.status;
    }
    if (0 != (status = Transfer(worker->descriptor, body, size, false, deadline)))
    {
        return status;
    }

    unsigned char extra;
    ssize_t count = recv(worker->descriptor, &extra, 1, MSG_PEEK);
    if (count > 0)
    {
        return EPROTO;
    }
    if (0 == count)
    {
        return EPIPE;
    }
    if ((EAGAIN != errno) && (EWOULDBLOCK != errno) && (EINTR != errno))
    {
        return errno;
    }
    int remaining;
    return TelemetryDeadlineRemaining(deadline, &remaining);
}

static int PrepareDescriptor(int* descriptor)
{
    if (*descriptor <= STDERR_FILENO)
    {
        int replacement = fcntl(*descriptor, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
        if (replacement < 0)
        {
            return errno;
        }
        int status = (0 == close(*descriptor)) ? 0 : errno;
        *descriptor = replacement;
        return status;
    }
    return (0 == fcntl(*descriptor, F_SETFD, FD_CLOEXEC)) ? 0 : errno;
}

static int StartChild(TelemetryWorker* worker, int64_t deadline, OsConfigLogHandle log)
{
    struct sigaction action = {0};
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    bool haveActions = false;
    bool haveAttributes = false;
    int descriptors[2] = {-1, -1};
    int status = 0;
    int remaining;
    sigset_t defaults;
    sigset_t mask;
    char lifetime[32];
    char startup[32];
    char* arguments[] = {worker->path, TELEMETRY_WORKER_ARGUMENT, lifetime, startup, NULL};

    if (worker->child > 0)
    {
        return (worker->descriptor >= 0) ? 0 : EBUSY;
    }
    if (0 != sigaction(SIGCHLD, NULL, &action))
    {
        return errno;
    }
    if ((SIG_DFL != action.sa_handler) || (0 != (action.sa_flags & SA_NOCLDWAIT)))
    {
        return ENOTSUP;
    }
    snprintf(lifetime, sizeof(lifetime), "%" PRId64, worker->lifetimeDeadline);
    snprintf(startup, sizeof(startup), "%" PRId64, deadline);
    if (0 != socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors))
    {
        return errno;
    }
    if ((0 != (status = PrepareDescriptor(&descriptors[0]))) ||
        (0 != (status = PrepareDescriptor(&descriptors[1]))))
    {
        goto cleanup;
    }
    int flags = fcntl(descriptors[0], F_GETFL);
    if ((flags < 0) || (0 != fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK)))
    {
        status = errno;
        goto cleanup;
    }
    if (0 != (status = posix_spawn_file_actions_init(&actions)))
    {
        goto cleanup;
    }
    haveActions = true;
    if ((0 != (status = posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDIN_FILENO))) ||
        (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[0]))) ||
        (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[1]))) ||
        (0 != (status = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0))) ||
        (0 != (status = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0))) ||
        (0 != (status = posix_spawnattr_init(&attributes))))
    {
        goto cleanup;
    }
    haveAttributes = true;
    sigemptyset(&defaults);
    sigaddset(&defaults, SIGALRM);
    sigaddset(&defaults, SIGPIPE);
    sigemptyset(&mask);
    if ((0 != (status = posix_spawnattr_setsigdefault(&attributes, &defaults))) ||
        (0 != (status = posix_spawnattr_setsigmask(&attributes, &mask))) ||
        (0 != (status = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK))) ||
        (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining))))
    {
        goto cleanup;
    }
    pid_t child = -1;
    status = posix_spawn(&child, worker->path, &actions, &attributes, arguments, environ);
    if (0 == status)
    {
        worker->child = child;
        worker->descriptor = descriptors[0];
        worker->sequence = 0;
        descriptors[0] = -1;
    }

cleanup:
    if (haveActions)
    {
        int error = posix_spawn_file_actions_destroy(&actions);
        if (0 != error)
        {
            OsConfigLogInfo(log, "TelemetryWorker: Cannot release spawn actions (status=%d)", error);
            if (0 == status)
            {
                status = error;
            }
        }
    }
    if (haveAttributes)
    {
        int error = posix_spawnattr_destroy(&attributes);
        if (0 != error)
        {
            OsConfigLogInfo(log, "TelemetryWorker: Cannot release spawn attributes (status=%d)", error);
            if (0 == status)
            {
                status = error;
            }
        }
    }
    for (size_t i = 0; i < 2; ++i)
    {
        if ((descriptors[i] >= 0) && (0 != close(descriptors[i])))
        {
            int error = errno;
            OsConfigLogInfo(log, "TelemetryWorker: Cannot close spawn IPC (status=%d)", error);
            if (0 == status)
            {
                status = error;
            }
        }
    }
    return status ? status : ReceiveReply(worker, TELEMETRY_WORKER_READY, 0, NULL, 0, deadline);
}

int TelemetryWorkerCreate(const char* workerPath, int lifetimeMilliseconds,
    int budgetMilliseconds, int operationMilliseconds, TelemetryWorker** worker,
    OsConfigLogHandle log)
{
    int status = 0;
    int64_t now = 0;
    TelemetryWorker* created = NULL;
    if ((NULL == worker) || (NULL == workerPath) || ('/' != workerPath[0]) ||
        (strnlen(workerPath, PATH_MAX) >= PATH_MAX) || (lifetimeMilliseconds <= 0) ||
        (budgetMilliseconds <= 0) || (operationMilliseconds <= 0))
    {
        status = EINVAL;
    }
    else if (NULL != *worker)
    {
        status = EALREADY;
    }
    else if (0 == (status = TelemetryMonotonicTime(&now)))
    {
        if (now > INT64_MAX - (int64_t)lifetimeMilliseconds * 1000000)
        {
            status = EOVERFLOW;
        }
        else if (NULL == (created = calloc(1, sizeof(*created))))
        {
            status = ENOMEM;
        }
        else if (NULL == (created->path = strdup(workerPath)))
        {
            status = ENOMEM;
        }
        else
        {
            created->descriptor = -1;
            created->child = -1;
            created->lifetimeDeadline = now + (int64_t)lifetimeMilliseconds * 1000000;
            created->budget = (int64_t)budgetMilliseconds * 1000000;
            created->operationLimit = (int64_t)operationMilliseconds * 1000000;
            status = ChargeBudget(created, now, log);
        }
    }
    if (0 != status)
    {
        if (NULL != created)
        {
            free(created->path);
            free(created);
        }
        OsConfigLogInfo(log, "TelemetryWorkerCreate: Failed (status=%d)", status);
        return status;
    }
    *worker = created;
    return 0;
}

int TelemetryWorkerResolve(TelemetryWorker* worker, const char* host,
    TelemetryResolvedHost* result, OsConfigLogHandle log)
{
    TelemetryResolverReply reply = {0};
    TelemetryResolvedHost resolved = {0};
    TelemetryWorkerFrame request = {0};
    int64_t start = 0;
    int64_t deadline = 0;
    int status = 0;
    if (NULL != result)
    {
        memset(result, 0, sizeof(*result));
    }
    if ((NULL == worker) || (NULL == result) || (NULL == host) || ('\0' == host[0]) ||
        (strnlen(host, TELEMETRY_RESOLVER_HOST_LIMIT + 1) > TELEMETRY_RESOLVER_HOST_LIMIT))
    {
        OsConfigLogInfo(log, "TelemetryWorkerResolve: Invalid arguments");
        return EINVAL;
    }
    if ((0 != (status = OperationDeadline(worker, &start, &deadline))) ||
        (0 != (status = StartChild(worker, deadline, log))))
    {
        goto failed;
    }
    if (UINT32_MAX == worker->sequence)
    {
        status = EOVERFLOW;
        goto failed;
    }
    request.magic = TELEMETRY_WORKER_MAGIC;
    request.version = TELEMETRY_WORKER_VERSION;
    request.operation = TELEMETRY_WORKER_RESOLVE;
    request.size = (uint32_t)strlen(host) + 1;
    request.sequence = ++worker->sequence;
    request.deadline = deadline;
    if ((0 != (status = Transfer(worker->descriptor, &request, sizeof(request), true, deadline))) ||
        (0 != (status = Transfer(worker->descriptor, (void*)host, request.size, true, deadline))) ||
        (0 != (status = ReceiveReply(worker, request.operation, request.sequence, &reply, sizeof(reply), deadline))))
    {
        goto failed;
    }
    status = TelemetryDecodeResolverReply(&reply, &resolved);
    if (EPROTO == status)
    {
        goto failed;
    }
    if (0 == status)
    {
        int remaining;
        status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status)
        {
            goto failed;
        }
    }
    goto finished;

failed:
    // Never retry this request. Only a later API call may create a new child.
    ReleaseChild(worker, 0, log);
finished:
    if (start > 0)
    {
        int error = ChargeBudget(worker, start, log);
        if ((0 == status) && (0 != error))
        {
            status = error;
            ReleaseChild(worker, 0, log);
        }
    }
    if (0 != status)
    {
        OsConfigLogInfo(log, "TelemetryWorkerResolve: Failed (status=%d, lookup=%d)", status, reply.lookupError);
    }
    else
    {
        *result = resolved;
    }
    return status;
}

int TelemetryWorkerAccountPreparation(TelemetryWorker* worker, int64_t started, OsConfigLogHandle log)
{
    if (!worker || started <= 0)
    {
        OsConfigLogError(log, "TelemetryWorker: Invalid preparation accounting");
        return EINVAL;
    }
    int status = ChargeBudget(worker, started, log);
    if (!status && worker->budget <= 0)
    {
        status = ETIMEDOUT;
        OsConfigLogInfo(log, "TelemetryWorker: Preparation exhausted invocation budget");
    }
    return status;
}

int TelemetryWorkerSend(TelemetryWorker* worker, const char* name,
    const TelemetryProperty* properties, size_t count, OsConfigLogHandle log)
{
    if (!worker)
    {
        OsConfigLogError(log, "TelemetryWorkerSend: Missing invocation");
        return EINVAL;
    }
    if (worker->suppressed)
    {
        OsConfigLogInfo(log, "TelemetryWorkerSend: Invocation suppressed; event dropped");
        return ECANCELED;
    }
    int64_t start = 0, deadline = 0;
    int status = OperationDeadline(worker, &start, &deadline);
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE];
    size_t size = 0;
    TelemetryWorkerSendReply reply = {0};
    TelemetryWorkerFrame request = {0};
    bool sendStarted = false;
    if (status) goto finished;
    status = TelemetryPackEvent(name, properties, count, payload, sizeof(payload), &size, log);
    if (status) goto finished;
    if (0 != (status = StartChild(worker, deadline, log))) goto failed;
    if (UINT32_MAX == worker->sequence) { status = EOVERFLOW; goto failed; }
    request.magic = TELEMETRY_WORKER_MAGIC;
    request.version = TELEMETRY_WORKER_VERSION;
    request.operation = TELEMETRY_WORKER_SEND;
    request.size = (uint32_t)size;
    request.sequence = ++worker->sequence;
    request.deadline = deadline;
    sendStarted = true;
    if ((0 != (status = Transfer(worker->descriptor, &request, sizeof(request), true, deadline))) ||
        (0 != (status = Transfer(worker->descriptor, payload, size, true, deadline))) ||
        (0 != (status = ReceiveReply(worker, request.operation, request.sequence, &reply, sizeof(reply), deadline))))
        goto failed;
    if (reply.status < 0 || reply.suppressed > 1) { status = EPROTO; goto failed; }
    worker->suppressed = reply.suppressed != 0;
    status = reply.status;
    goto finished;
failed:
    // An ambiguous SEND reply could hide collector suppression. Stop this
    // invocation rather than restarting a child and losing that control.
    if (sendStarted) worker->suppressed = true;
    ReleaseChild(worker, 0, log);
finished:
    if (start > 0)
    {
        int error = ChargeBudget(worker, start, log);
        if (!status && error)
        {
            status = error;
            worker->suppressed = true;
            ReleaseChild(worker, 0, log);
        }
    }
    if (status) OsConfigLogInfo(log, "TelemetryWorkerSend: Event not delivered (status=%d)", status);
    return status;
}

int TelemetryWorkerDestroy(TelemetryWorker** worker, OsConfigLogHandle log)
{
    if (NULL == worker)
    {
        OsConfigLogInfo(log, "TelemetryWorkerDestroy: Invalid context pointer");
        return EINVAL;
    }
    if (NULL == *worker)
    {
        return 0;
    }
    int64_t start = 0;
    int64_t deadline = 0;
    int timingStatus = OperationDeadline(*worker, &start, &deadline);
    int status = ReleaseChild(*worker, (0 == timingStatus) ? deadline : 0, log);
    if ((0 != timingStatus) && (ETIMEDOUT != timingStatus))
    {
        OsConfigLogInfo(log, "TelemetryWorkerDestroy: Cannot time cleanup (status=%d)", timingStatus);
        if (0 == status)
        {
            status = timingStatus;
        }
    }
    if ((*worker)->child > 0)
    {
        // Keep ownership so the caller can retry cleanup; do not orphan it.
        return status;
    }
    free((*worker)->path);
    free(*worker);
    *worker = NULL;
    return status;
}
