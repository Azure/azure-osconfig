// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "Worker.h"
#include "WorkerProtocol.h"
#include "ResolverProtocol.h"
#include "Deadline.h"
#include "Event.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
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

_Static_assert(32 == sizeof(TelemetryWorkerFrame), "Unexpected worker frame layout");

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
    int status = 0;
    int64_t limit = 0;

    if (0 != (status = TelemetryMonotonicTime(start)))
    {
        worker->budget = 0;
        return status;
    }

    if ((worker->budget <= 0) || (*start >= worker->lifetimeDeadline))
    {
        return ETIMEDOUT;
    }

    limit = (worker->budget < worker->operationLimit) ? worker->budget : worker->operationLimit;

    if (limit > (worker->lifetimeDeadline - *start))
    {
        limit = worker->lifetimeDeadline - *start;
    }

    *deadline = *start + limit;

    return 0;
}

static int ChargeBudget(TelemetryWorker* worker, int64_t start, OsConfigLogHandle log)
{
    int64_t now = 0;
    int status = 0;
    int64_t elapsed = 0;

    if (0 != (status = TelemetryMonotonicTime(&now)))
    {
        worker->budget = 0;
        OsConfigLogError(log, "ChargeBudget: TelemetryMonotonicTime failed with %d (%s)", status, strerror(status));
        return status;
    }

    elapsed = now - start;

    if (elapsed < 0)
    {
        worker->budget = 0;
        OsConfigLogError(log, "ChargeBudget: elapsed-time validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    worker->budget = (elapsed >= worker->budget) ? 0 : (worker->budget - elapsed);

    return 0;
}

static int ReleaseChild(TelemetryWorker* worker, int64_t gracefulDeadline, OsConfigLogHandle log)
{
    int status = 0;
    int childStatus = 0;
    int remaining = 0;
    int timingStatus = 0;
    pid_t waited = 0;
    int error = 0;

    if (worker->descriptor >= 0)
    {
        if (0 != close(worker->descriptor))
        {
            status = errno;
            OsConfigLogError(log, "ReleaseChild: close failed with %d (%s)", status, strerror(status));
        }

        worker->descriptor = -1;
    }

    if (worker->child <= 0)
    {
        return status;
    }

    for (;;)
    {
        childStatus = 0;
        waited = waitpid(worker->child, &childStatus, WNOHANG);

        if (worker->child == waited)
        {
            worker->child = -1;

            if ((0 == WIFEXITED(childStatus)) || (0 != WEXITSTATUS(childStatus)))
            {
                OsConfigLogError(log, "ReleaseChild: child exit validation failed with %d (%s); wait status %d", EIO, strerror(EIO), childStatus);
                return (0 != status) ? status : EIO;
            }

            return status;
        }

        if ((waited < 0) && (EINTR != errno))
        {
            error = errno;
            OsConfigLogError(log, "ReleaseChild: waitpid(WNOHANG) failed with %d (%s)", error, strerror(error));

            if (ECHILD == error)
            {
                // The host consumed the child: its PID must not be signaled.
                worker->child = -1;
            }

            return (0 != status) ? status : error;
        }

        if (gracefulDeadline <= 0)
        {
            break;
        }

        remaining = 0;

        if (0 != (timingStatus = TelemetryDeadlineRemaining(gracefulDeadline, &remaining)))
        {
            if (ETIMEDOUT != timingStatus)
            {
                OsConfigLogError(log, "ReleaseChild: TelemetryDeadlineRemaining failed with %d (%s)", timingStatus, strerror(timingStatus));

                if (0 == status)
                {
                    status = timingStatus;
                }
            }

            break;
        }

        if ((poll(NULL, 0, (remaining > 10) ? 10 : remaining) < 0) && (EINTR != errno))
        {
            status = errno;
            OsConfigLogError(log, "ReleaseChild: poll failed with %d (%s)", status, strerror(status));
            break;
        }
    }

    if ((0 != kill(worker->child, SIGKILL)) && (ESRCH != errno))
    {
        error = errno;
        OsConfigLogError(log, "ReleaseChild: kill(SIGKILL) failed with %d (%s)", error, strerror(error));
        return (0 != status) ? status : error;
    }

    waited = 0;

    do
    {
        waited = waitpid(worker->child, NULL, 0);
    } while ((waited < 0) && (EINTR == errno));

    if (waited < 0)
    {
        error = errno;
        OsConfigLogError(log, "ReleaseChild: waitpid failed with %d (%s)", error, strerror(error));

        if (ECHILD == error)
        {
            worker->child = -1;
        }

        return (0 != status) ? status : error;
    }

    worker->child = -1;
    if ((0 == status) && (gracefulDeadline > 0))
    {
        OsConfigLogError(log, "ReleaseChild: graceful shutdown deadline check failed with %d (%s)", ETIMEDOUT, strerror(ETIMEDOUT));
    }

    return (0 != status) ? status : ((gracefulDeadline > 0) ? ETIMEDOUT : 0);
}

static int Transfer(int descriptor, void* buffer, size_t size, bool writing, int64_t deadline, OsConfigLogHandle log)
{
    unsigned char* bytes = buffer;
    size_t offset = 0;
    int remaining = 0;
    int status = 0;
    ssize_t count = 0;
    struct pollfd item = {0};

    while (offset < size)
    {
        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            OsConfigLogError(log, "Transfer: TelemetryDeadlineRemaining failed with %d (%s)", status, strerror(status));
            return status;
        }

        count = writing ? send(descriptor, bytes + offset, size - offset, MSG_NOSIGNAL) : recv(descriptor, bytes + offset, size - offset, 0);

        if (count > 0)
        {
            offset += (size_t)count;
            continue;
        }

        if (0 == count)
        {
            status = TelemetryDeadlineRemaining(deadline, &remaining);
            status = (0 != status) ? status : ((0 == offset) ? EPIPE : EPROTO);
            OsConfigLogError(log, "Transfer: %s completion check failed with %d (%s); transferred %zu of %zu bytes",
                writing ? "send" : "recv", status, strerror(status), offset, size);
            return status;
        }

        if (EINTR == errno)
        {
            continue;
        }

        if ((EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
            status = errno;
            OsConfigLogError(log, "Transfer: %s failed with %d (%s)", writing ? "send" : "recv", status, strerror(status));
            return status;
        }

        item = (struct pollfd){descriptor, (short)(writing ? POLLOUT : POLLIN), 0};

        if ((poll(&item, 1, remaining) < 0) && (EINTR != errno))
        {
            status = errno;
            OsConfigLogError(log, "Transfer: poll failed with %d (%s)", status, strerror(status));
            return status;
        }
    }

    return 0;
}

static int ReceiveReply(TelemetryWorker* worker, uint32_t operation, uint32_t sequence, void* body, size_t size, int64_t deadline, OsConfigLogHandle log)
{
    TelemetryWorkerFrame reply = {0};
    int status = 0;
    unsigned char extra = '\0';
    ssize_t count = 0;
    int remaining = 0;

    if (0 != (status = Transfer(worker->descriptor, &reply, sizeof(reply), false, deadline, log)))
    {
        return status;
    }

    if ((TELEMETRY_WORKER_MAGIC != reply.magic) || (TELEMETRY_WORKER_VERSION != reply.version) ||
        (reply.operation != operation) || (reply.sequence != sequence) || (0 != reply.deadline) ||
        (reply.status < 0) || (reply.size != ((0 != reply.status) ? 0 : size)))
    {
        OsConfigLogError(log, "ReceiveReply: reply header validation failed with %d (%s)", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    if (0 != reply.status)
    {
        OsConfigLogError(log, "ReceiveReply: worker operation %u failed with %d (%s)", operation, reply.status, strerror(reply.status));
        return reply.status;
    }

    if (0 != (status = Transfer(worker->descriptor, body, size, false, deadline, log)))
    {
        return status;
    }

    count = recv(worker->descriptor, &extra, 1, MSG_PEEK);

    if (count > 0)
    {
        OsConfigLogError(log, "ReceiveReply: reply boundary validation failed with %d (%s)", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    if (0 == count)
    {
        OsConfigLogError(log, "ReceiveReply: recv(MSG_PEEK) failed with %d (%s); worker disconnected", EPIPE, strerror(EPIPE));
        return EPIPE;
    }

    if ((EAGAIN != errno) && (EWOULDBLOCK != errno) && (EINTR != errno))
    {
        status = errno;
        OsConfigLogError(log, "ReceiveReply: recv(MSG_PEEK) failed with %d (%s)", status, strerror(status));
        return status;
    }

    status = TelemetryDeadlineRemaining(deadline, &remaining);
    if (0 != status)
    {
        OsConfigLogError(log, "ReceiveReply: TelemetryDeadlineRemaining failed with %d (%s)", status, strerror(status));
    }

    return status;
}

static int PrepareDescriptor(int* descriptor, OsConfigLogHandle log)
{
    int replacement = 0;
    int status = 0;

    if (*descriptor <= STDERR_FILENO)
    {
        replacement = fcntl(*descriptor, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);

        if (replacement < 0)
        {
            status = errno;
            OsConfigLogError(log, "PrepareDescriptor: fcntl(F_DUPFD_CLOEXEC) failed with %d (%s)", status, strerror(status));
            return status;
        }

        status = (0 == close(*descriptor)) ? 0 : errno;
        if (0 != status)
        {
            OsConfigLogError(log, "PrepareDescriptor: close failed with %d (%s)", status, strerror(status));
        }
        *descriptor = replacement;

        return status;
    }

    status = (0 == fcntl(*descriptor, F_SETFD, FD_CLOEXEC)) ? 0 : errno;
    if (0 != status)
    {
        OsConfigLogError(log, "PrepareDescriptor: fcntl(F_SETFD) failed with %d (%s)", status, strerror(status));
    }

    return status;
}

static int StartChild(TelemetryWorker* worker, int64_t deadline, OsConfigLogHandle log)
{
    struct sigaction action = {0};
    posix_spawn_file_actions_t actions = {0};
    posix_spawnattr_t attributes = {0};
    bool haveActions = false;
    bool haveAttributes = false;
    int descriptors[2] = {-1, -1};
    int status = 0;
    int remaining = 0;
    sigset_t defaults = {0};
    sigset_t mask = {0};
    char lifetime[32] = {0};
    char startup[32] = {0};
    char* arguments[] = {worker->path, TELEMETRY_WORKER_ARGUMENT, lifetime, startup, NULL};
    int flags = 0;
    pid_t child = -1;
    size_t i = 0;
    int error = 0;
    const char* operation = "PrepareDescriptor";

    if (worker->child > 0)
    {
        status = (worker->descriptor >= 0) ? 0 : EBUSY;
        if (0 != status)
        {
            OsConfigLogError(log, "StartChild: child IPC state validation failed with %d (%s)", status, strerror(status));
        }

        return status;
    }

    if (0 != sigaction(SIGCHLD, NULL, &action))
    {
        status = errno;
        OsConfigLogError(log, "StartChild: sigaction(SIGCHLD) failed with %d (%s)", status, strerror(status));
        return status;
    }

    if ((SIG_DFL != action.sa_handler) || (0 != (action.sa_flags & SA_NOCLDWAIT)))
    {
        OsConfigLogError(log, "StartChild: SIGCHLD ownership validation failed with %d (%s)", ENOTSUP, strerror(ENOTSUP));
        return ENOTSUP;
    }

    snprintf(lifetime, sizeof(lifetime), "%" PRId64, worker->lifetimeDeadline);
    snprintf(startup, sizeof(startup), "%" PRId64, deadline);

    if (0 != socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors))
    {
        status = errno;
        OsConfigLogError(log, "StartChild: socketpair failed with %d (%s)", status, strerror(status));
        return status;
    }

    if ((0 != (status = PrepareDescriptor(&descriptors[0], log))) || (0 != (status = PrepareDescriptor(&descriptors[1], log))))
    {
        goto cleanup;
    }

    flags = fcntl(descriptors[0], F_GETFL);
    operation = (flags < 0) ? "fcntl(F_GETFL)" : "fcntl(F_SETFL)";

    if ((flags < 0) || (0 != fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK)))
    {
        status = errno;
        goto cleanup;
    }

    operation = "posix_spawn_file_actions_init";
    if (0 != (status = posix_spawn_file_actions_init(&actions)))
    {
        goto cleanup;
    }

    haveActions = true;

    operation = "posix_spawn_file_actions_adddup2";
    if (0 != (status = posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDIN_FILENO)))
    {
        goto cleanup;
    }

    operation = "posix_spawn_file_actions_addclose";
    if ((0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[0]))) ||
        (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[1]))))
    {
        goto cleanup;
    }

    operation = "posix_spawn_file_actions_addopen";
    if ((0 != (status = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0))) ||
        (0 != (status = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0))))
    {
        goto cleanup;
    }

    operation = "posix_spawnattr_init";
    if (0 != (status = posix_spawnattr_init(&attributes)))
    {
        goto cleanup;
    }

    haveAttributes = true;
    sigemptyset(&defaults);
    sigaddset(&defaults, SIGALRM);
    sigaddset(&defaults, SIGPIPE);
    sigemptyset(&mask);

    operation = "posix_spawnattr_setsigdefault";
    if (0 != (status = posix_spawnattr_setsigdefault(&attributes, &defaults)))
    {
        goto cleanup;
    }

    operation = "posix_spawnattr_setsigmask";
    if (0 != (status = posix_spawnattr_setsigmask(&attributes, &mask)))
    {
        goto cleanup;
    }

    operation = "posix_spawnattr_setflags";
    if (0 != (status = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK)))
    {
        goto cleanup;
    }

    operation = "TelemetryDeadlineRemaining";
    if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        goto cleanup;
    }

    operation = "posix_spawn";
    if (0 == (status = posix_spawn(&child, worker->path, &actions, &attributes, arguments, environ)))
    {
        worker->child = child;
        worker->descriptor = descriptors[0];
        worker->sequence = 0;
        descriptors[0] = -1;
    }

cleanup:
    if (0 != status)
    {
        OsConfigLogError(log, "StartChild: %s failed with %d (%s)", operation, status, strerror(status));
    }

    if (haveActions)
    {
        if (0 != (error = posix_spawn_file_actions_destroy(&actions)))
        {
            OsConfigLogError(log, "StartChild: posix_spawn_file_actions_destroy failed with %d (%s)", error, strerror(error));

            if (0 == status)
            {
                status = error;
            }
        }
    }

    if (haveAttributes)
    {
        if (0 != (error = posix_spawnattr_destroy(&attributes)))
        {
            OsConfigLogError(log, "StartChild: posix_spawnattr_destroy failed with %d (%s)", error, strerror(error));

            if (0 == status)
            {
                status = error;
            }
        }
    }

    for (i = 0; i < 2; ++i)
    {
        if ((descriptors[i] >= 0) && (0 != close(descriptors[i])))
        {
            error = errno;
            OsConfigLogError(log, "StartChild: close failed with %d (%s)", error, strerror(error));

            if (0 == status)
            {
                status = error;
            }
        }
    }

    return (0 != status) ? status : ReceiveReply(worker, TELEMETRY_WORKER_READY, 0, NULL, 0, deadline, log);
}

int TelemetryWorkerCreate(const char* workerPath, int lifetimeMilliseconds, int budgetMilliseconds, int operationMilliseconds, TelemetryWorker** worker, OsConfigLogHandle log)
{
    int status = 0;
    int64_t now = 0;
    TelemetryWorker* created = NULL;
    const char* operation = "argument validation";

    if ((NULL == worker) || (NULL == workerPath) || ('/' != workerPath[0]) ||
        (strnlen(workerPath, PATH_MAX) >= PATH_MAX) || (lifetimeMilliseconds <= 0) ||
        (budgetMilliseconds <= 0) || (operationMilliseconds <= 0))
    {
        status = EINVAL;
    }
    else if (NULL != *worker)
    {
        operation = "invocation state validation";
        status = EALREADY;
    }
    else if (0 != (status = TelemetryMonotonicTime(&now)))
    {
        operation = "TelemetryMonotonicTime";
    }
    else
    {
        if (now > (INT64_MAX - ((int64_t)lifetimeMilliseconds * 1000000)))
        {
            operation = "lifetime range check";
            status = EOVERFLOW;
        }
        else if (NULL == (created = calloc(1, sizeof(*created))))
        {
            operation = "calloc";
            status = ENOMEM;
        }
        else if (NULL == (created->path = strdup(workerPath)))
        {
            operation = "strdup";
            status = ENOMEM;
        }
        else
        {
            created->descriptor = -1;
            created->child = -1;
            created->lifetimeDeadline = now + ((int64_t)lifetimeMilliseconds * 1000000);
            created->budget = (int64_t)budgetMilliseconds * 1000000;
            created->operationLimit = (int64_t)operationMilliseconds * 1000000;
            operation = "ChargeBudget";
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

        OsConfigLogError(log, "TelemetryWorkerCreate: %s failed with %d (%s)", operation, status, strerror(status));
        return status;
    }

    *worker = created;

    return 0;
}

int TelemetryWorkerResolve(TelemetryWorker* worker, const char* host, TelemetryResolvedHost* result, OsConfigLogHandle log)
{
    TelemetryResolverReply reply = {0};
    TelemetryResolvedHost resolved = {0};
    TelemetryWorkerFrame request = {0};
    int64_t start = 0;
    int64_t deadline = 0;
    int status = 0;
    int remaining = 0;
    int error = 0;
    const char* operation = "OperationDeadline";

    if (NULL != result)
    {
        memset(result, 0, sizeof(*result));
    }

    if ((NULL == worker) || (NULL == result) || (NULL == host) || ('\0' == host[0]) || (strnlen(host, TELEMETRY_RESOLVER_HOST_LIMIT + 1) > TELEMETRY_RESOLVER_HOST_LIMIT))
    {
        OsConfigLogError(log, "TelemetryWorkerResolve: argument validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if (0 != (status = OperationDeadline(worker, &start, &deadline)))
    {
        goto failed;
    }

    operation = "StartChild";
    if (0 != (status = StartChild(worker, deadline, log)))
    {
        goto failed;
    }

    if (UINT32_MAX == worker->sequence)
    {
        operation = "sequence range check";
        status = EOVERFLOW;
        goto failed;
    }

    request.magic = TELEMETRY_WORKER_MAGIC;
    request.version = TELEMETRY_WORKER_VERSION;
    request.operation = TELEMETRY_WORKER_RESOLVE;
    request.size = (uint32_t)strlen(host) + 1;
    request.sequence = ++worker->sequence;
    request.deadline = deadline;

    operation = "Transfer";
    if ((0 != (status = Transfer(worker->descriptor, &request, sizeof(request), true, deadline, log))) ||
        (0 != (status = Transfer(worker->descriptor, (void*)host, request.size, true, deadline, log))))
    {
        goto failed;
    }

    operation = "ReceiveReply";
    if (0 != (status = ReceiveReply(worker, request.operation, request.sequence, &reply, sizeof(reply), deadline, log)))
    {
        goto failed;
    }

    operation = "TelemetryDecodeResolverReply";
    if (EPROTO == (status = TelemetryDecodeResolverReply(&reply, &resolved)))
    {
        goto failed;
    }

    if (0 == status)
    {
        operation = "TelemetryDeadlineRemaining";
        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
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
        error = ChargeBudget(worker, start, log);

        if ((0 == status) && (0 != error))
        {
            operation = "ChargeBudget";
            status = error;
            ReleaseChild(worker, 0, log);
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryWorkerResolve: %s failed with %d (%s); getaddrinfo returned %d (%s)",
            operation, status, strerror(status), reply.lookupError, (0 != reply.lookupError) ? gai_strerror(reply.lookupError) : "no resolver error");
    }
    else
    {
        *result = resolved;
    }

    return status;
}

int TelemetryWorkerAccountPreparation(TelemetryWorker* worker, int64_t started, OsConfigLogHandle log)
{
    int status = 0;

    if ((NULL == worker) || (started <= 0))
    {
        OsConfigLogError(log, "TelemetryWorkerAccountPreparation: argument validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if ((0 == (status = ChargeBudget(worker, started, log))) && (worker->budget <= 0))
    {
        status = ETIMEDOUT;
        OsConfigLogError(log, "TelemetryWorkerAccountPreparation: remaining budget check failed with %d (%s)", status, strerror(status));
    }

    return status;
}

int TelemetryWorkerSendEvent(TelemetryWorker* worker, const char* name, const TelemetryProperty* properties, size_t count, OsConfigLogHandle log)
{
    int64_t start = 0;
    int64_t deadline = 0;
    int status = 0;
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {0};
    size_t size = 0;
    TelemetryWorkerSendReply reply = {0};
    TelemetryWorkerFrame request = {0};
    bool sendStarted = false;
    int error = 0;
    const char* operation = "OperationDeadline";

    if (NULL == worker)
    {
        OsConfigLogError(log, "TelemetryWorkerSendEvent: invocation pointer validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if (worker->suppressed)
    {
        OsConfigLogError(log, "TelemetryWorkerSendEvent: invocation suppression check failed with %d (%s)", ECANCELED, strerror(ECANCELED));
        return ECANCELED;
    }

    if (0 != (status = OperationDeadline(worker, &start, &deadline)))
    {
        goto finished;
    }

    operation = "TelemetryPackEvent";
    if (0 != (status = TelemetryPackEvent(name, properties, count, payload, sizeof(payload), &size, log)))
    {
        goto finished;
    }

    operation = "StartChild";
    if (0 != (status = StartChild(worker, deadline, log)))
    {
        goto failed;
    }

    if (UINT32_MAX == worker->sequence)
    {
        operation = "sequence range check";
        status = EOVERFLOW;
        goto failed;
    }

    request.magic = TELEMETRY_WORKER_MAGIC;
    request.version = TELEMETRY_WORKER_VERSION;
    request.operation = TELEMETRY_WORKER_SEND;
    request.size = (uint32_t)size;
    request.sequence = ++worker->sequence;
    request.deadline = deadline;
    sendStarted = true;

    operation = "Transfer";
    if ((0 != (status = Transfer(worker->descriptor, &request, sizeof(request), true, deadline, log))) ||
        (0 != (status = Transfer(worker->descriptor, payload, size, true, deadline, log))))
    {
        goto failed;
    }

    operation = "ReceiveReply";
    if (0 != (status = ReceiveReply(worker, request.operation, request.sequence, &reply, sizeof(reply), deadline, log)))
    {
        goto failed;
    }

    if ((reply.status < 0) || (reply.suppressed > 1))
    {
        operation = "reply validation";
        status = EPROTO;
        goto failed;
    }

    worker->suppressed = 0 != reply.suppressed;
    operation = "SendEvent(worker)";
    status = reply.status;
    goto finished;

failed:
    // An ambiguous SEND reply could hide collector suppression. Stop this
    // invocation rather than restarting a child and losing that control.
    if (sendStarted)
    {
        worker->suppressed = true;
    }

    ReleaseChild(worker, 0, log);

finished:
    if (start > 0)
    {
        error = ChargeBudget(worker, start, log);

        if ((0 == status) && (0 != error))
        {
            operation = "ChargeBudget";
            status = error;
            worker->suppressed = true;
            ReleaseChild(worker, 0, log);
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryWorkerSendEvent: %s failed with %d (%s)", operation, status, strerror(status));
    }

    return status;
}

int TelemetryWorkerDestroy(TelemetryWorker** worker, OsConfigLogHandle log)
{
    int64_t start = 0;
    int64_t deadline = 0;
    int timingStatus = 0;
    int status = 0;

    if (NULL == worker)
    {
        OsConfigLogError(log, "TelemetryWorkerDestroy: context pointer validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if (NULL == *worker)
    {
        return 0;
    }

    timingStatus = OperationDeadline(*worker, &start, &deadline);
    status = ReleaseChild(*worker, (0 == timingStatus) ? deadline : 0, log);

    if ((0 != timingStatus) && (ETIMEDOUT != timingStatus))
    {
        OsConfigLogError(log, "TelemetryWorkerDestroy: OperationDeadline failed with %d (%s)", timingStatus, strerror(timingStatus));

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
