// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "IpcInterface.h"
#include "Utils.h"
#include "EventPayload.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
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

struct TelemetryWorker
{
    char* path;
    int readDescriptor;
    int writeDescriptor;
    pid_t child;
    uint32_t sequence;
    int64_t lifetimeDeadline;
    int64_t budget;
    int64_t operationLimit;
    bool suppressed;
    int startupStatus;
};

static int OperationDeadline(TelemetryWorker* worker, int64_t* start, int64_t* deadline)
{
    int status = 0;
    int64_t limit = 0;

    TelemetryMonotonicTime(start, status);
    if (0 != status)
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

    TelemetryMonotonicTime(&now, status);
    if (0 != status)
    {
        worker->budget = 0;
        OsConfigLogInfo(log, "ChargeBudget: TelemetryMonotonicTime failed with %d (%s)", status, strerror(status));
        return status;
    }

    elapsed = now - start;

    if (elapsed < 0)
    {
        worker->budget = 0;
        OsConfigLogInfo(log, "ChargeBudget: elapsed-time validation failed with %d (%s)", EINVAL, strerror(EINVAL));
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

    if (worker->writeDescriptor >= 0)
    {
        if (0 != close(worker->writeDescriptor))
        {
            status = errno;
            OsConfigLogInfo(log, "ReleaseChild: close failed with %d (%s)", status, strerror(status));
        }

        worker->writeDescriptor = -1;
    }
    if (worker->readDescriptor >= 0)
    {
        if (0 != close(worker->readDescriptor))
        {
            status = errno;
            OsConfigLogInfo(log, "ReleaseChild: close failed with %d (%s)", status, strerror(status));
        }
        worker->readDescriptor = -1;
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
                OsConfigLogInfo(log, "ReleaseChild: child exit validation failed with %d (%s); wait status %d", EIO, strerror(EIO), childStatus);
                return (0 != status) ? status : EIO;
            }

            return status;
        }

        if ((waited < 0) && (EINTR != errno))
        {
            error = errno;
            OsConfigLogInfo(log, "ReleaseChild: waitpid(WNOHANG) failed with %d (%s)", error, strerror(error));

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

        TelemetryDeadlineRemaining(gracefulDeadline, &remaining, timingStatus);
        if (0 != timingStatus)
        {
            if (ETIMEDOUT != timingStatus)
            {
                OsConfigLogInfo(log, "ReleaseChild: TelemetryDeadlineRemaining failed with %d (%s)", timingStatus, strerror(timingStatus));

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
            OsConfigLogInfo(log, "ReleaseChild: poll failed with %d (%s)", status, strerror(status));
            break;
        }
    }

    if ((0 != kill(worker->child, SIGKILL)) && (ESRCH != errno))
    {
        error = errno;
        OsConfigLogInfo(log, "ReleaseChild: kill(SIGKILL) failed with %d (%s)", error, strerror(error));
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
        OsConfigLogInfo(log, "ReleaseChild: waitpid failed with %d (%s)", error, strerror(error));

        if (ECHILD == error)
        {
            worker->child = -1;
        }

        return (0 != status) ? status : error;
    }

    worker->child = -1;
    if ((0 == status) && (gracefulDeadline > 0))
    {
        OsConfigLogInfo(log, "ReleaseChild: graceful shutdown deadline check failed with %d (%s)", ETIMEDOUT, strerror(ETIMEDOUT));
    }

    return (0 != status) ? status : ((gracefulDeadline > 0) ? ETIMEDOUT : 0);
}

// Block only this thread's SIGPIPE; preserve a signal already pending for it.
static ssize_t WritePipe(int descriptor, const void* buffer, size_t size)
{
    sigset_t blocked;
    sigset_t previous;
    sigset_t pending;
    struct timespec zero = {0};
    ssize_t count = -1;
    int status = 0;
    int saved = 0;
    int waited = 0;
    bool wasPending = false;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    if (0 != (status = pthread_sigmask(SIG_BLOCK, &blocked, &previous)))
    {
        errno = status;
        return -1;
    }
    if (0 == sigpending(&pending))
    {
        wasPending = (1 == sigismember(&pending, SIGPIPE));
        count = write(descriptor, buffer, size);
        saved = errno;
        if ((count < 0) && (EPIPE == saved) && !wasPending)
        {
            do
            {
                waited = sigtimedwait(&blocked, NULL, &zero);
            } while ((waited < 0) && (EINTR == errno));
            if ((waited < 0) && (EAGAIN != errno))
            {
                saved = errno;
            }
        }
    }
    else
    {
        saved = errno;
    }
    if (0 != (status = pthread_sigmask(SIG_SETMASK, &previous, NULL)))
    {
        errno = status;
        return -1;
    }
    errno = saved;
    return count;
}

static int WorkerStatus(uint32_t status)
{
    switch (status)
    {
        case 0: return 0;
        case 1: case 2: return EINVAL;
        case 3: case 5: case 6: return EIO;
        case 4: return ETIMEDOUT;
        case 7: return ECANCELED;
        default: return EPROTO;
    }
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
        TelemetryDeadlineRemaining(deadline, &remaining, status);
        if (0 != status)
        {
            OsConfigLogInfo(log, "Transfer: TelemetryDeadlineRemaining failed with %d (%s)", status, strerror(status));
            return status;
        }

        count = writing ? WritePipe(descriptor, bytes + offset, size - offset) : read(descriptor, bytes + offset, size - offset);

        if (count > 0)
        {
            offset += (size_t)count;
            continue;
        }

        if (0 == count)
        {
            TelemetryDeadlineRemaining(deadline, &remaining, status);
            status = (0 != status) ? status : ((0 == offset) ? EPIPE : EPROTO);
            OsConfigLogInfo(log, "Transfer: %s completion check failed with %d (%s); transferred %zu of %zu bytes",
                writing ? "write" : "read", status, strerror(status), offset, size);
            return status;
        }

        if (EINTR == errno)
        {
            continue;
        }

        if ((EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
            status = errno;
            OsConfigLogInfo(log, "Transfer: %s failed with %d (%s)", writing ? "write" : "read", status, strerror(status));
            return status;
        }

        item = (struct pollfd){descriptor, (short)(writing ? POLLOUT : POLLIN), 0};

        if ((poll(&item, 1, remaining) < 0) && (EINTR != errno))
        {
            status = errno;
            OsConfigLogInfo(log, "Transfer: poll failed with %d (%s)", status, strerror(status));
            return status;
        }
    }

    return 0;
}

static int ReceiveReply(TelemetryWorker* worker, uint32_t operation, uint32_t sequence, void* body, size_t size, int64_t deadline, OsConfigLogHandle log)
{
    TelemetryWorkerFrame reply = {0};
    unsigned char header[TELEMETRY_WORKER_HEADER_SIZE] = {0};
    int status = 0;
    unsigned char extra = '\0';
    ssize_t count = 0;
    int remaining = 0;

    if (0 != (status = Transfer(worker->readDescriptor, header, sizeof(header), false, deadline, log)))
    {
        return status;
    }

    TelemetryDecodeFrame(header, &reply);
    if ((TELEMETRY_WORKER_MAGIC != reply.magic) || (TELEMETRY_WORKER_VERSION != reply.version) ||
        (reply.operation != operation) || (reply.sequence != sequence) || (0 != reply.deadline) ||
        (reply.status > 7) || (reply.size != ((0 != reply.status) ? 0 : size)))
    {
        OsConfigLogInfo(log, "ReceiveReply: reply header validation failed with %d (%s)", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    if (0 != reply.status)
    {
        status = WorkerStatus(reply.status);
        OsConfigLogInfo(log, "ReceiveReply: worker operation %u failed with %d (%s)", operation, status, strerror(status));
        return status;
    }

    if (0 != (status = Transfer(worker->readDescriptor, body, size, false, deadline, log)))
    {
        return status;
    }

    count = read(worker->readDescriptor, &extra, 1);

    if (count > 0)
    {
        OsConfigLogInfo(log, "ReceiveReply: reply boundary validation failed with %d (%s)", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    if ((0 == count) && (TELEMETRY_WORKER_READY == operation))
    {
        OsConfigLogInfo(log, "ReceiveReply: read failed with %d (%s); worker disconnected before use", EPIPE, strerror(EPIPE));
        return EPIPE;
    }

    if ((count < 0) && (EAGAIN != errno) && (EWOULDBLOCK != errno) && (EINTR != errno))
    {
        status = errno;
        OsConfigLogInfo(log, "ReceiveReply: read failed with %d (%s)", status, strerror(status));
        return status;
    }

    TelemetryDeadlineRemaining(deadline, &remaining, status);
    if (0 != status)
    {
        OsConfigLogInfo(log, "ReceiveReply: TelemetryDeadlineRemaining failed with %d (%s)", status, strerror(status));
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
            OsConfigLogInfo(log, "PrepareDescriptor: fcntl(F_DUPFD_CLOEXEC) failed with %d (%s)", status, strerror(status));
            return status;
        }

        status = (0 == close(*descriptor)) ? 0 : errno;
        if (0 != status)
        {
            OsConfigLogInfo(log, "PrepareDescriptor: close failed with %d (%s)", status, strerror(status));
        }
        *descriptor = replacement;

        return status;
    }

    status = (0 == fcntl(*descriptor, F_SETFD, FD_CLOEXEC)) ? 0 : errno;
    if (0 != status)
    {
        OsConfigLogInfo(log, "PrepareDescriptor: fcntl(F_SETFD) failed with %d (%s)", status, strerror(status));
    }

    return status;
}

static int StartChild(TelemetryWorker* worker, int64_t deadline, OsConfigLogHandle log)
{
    struct sigaction action;
    posix_spawn_file_actions_t actions = {0};
    posix_spawnattr_t attributes = {0};
    bool haveActions = false;
    bool haveAttributes = false;
    int descriptors[4] = {-1, -1, -1, -1};
    int status = 0;
    int remaining = 0;
    sigset_t defaults;
    sigset_t mask;
    char lifetime[32] = {0};
    char startup[32] = {0};
    char* arguments[] = {worker->path, TELEMETRY_WORKER_ARGUMENT,
        "--lifetime-ms", lifetime, "--budget-ms", startup,
        "--api-key-env", "OsConfigTelemetryApiKey", "--sdk-version", "OSConfig-C/0.1", NULL};
    int flags = 0;
    pid_t child = -1;
    size_t i = 0;
    int error = 0;
    const char* operation = "PrepareDescriptor";

    if (0 != worker->startupStatus)
    {
        operation = "previous permanent startup failure";
        status = worker->startupStatus;
        goto cleanup;
    }

    if (worker->child > 0)
    {
        status = ((worker->readDescriptor >= 0) && (worker->writeDescriptor >= 0)) ? 0 : EBUSY;
        if (0 != status)
        {
            OsConfigLogInfo(log, "StartChild: child IPC state validation failed with %d (%s)", status, strerror(status));
        }

        goto finished;
    }

    if (0 != sigaction(SIGCHLD, NULL, &action))
    {
        status = errno;
        operation = "sigaction(SIGCHLD)";
        goto cleanup;
    }

    if ((SIG_DFL != action.sa_handler) || (0 != (action.sa_flags & SA_NOCLDWAIT)))
    {
        operation = "SIGCHLD ownership validation";
        status = ENOTSUP;
        goto cleanup;
    }

    TelemetryDeadlineRemaining(worker->lifetimeDeadline, &remaining, status);
    if (0 != status)
    {
        goto cleanup;
    }
    snprintf(lifetime, sizeof(lifetime), "%d", (remaining > 600000) ? 600000 : remaining);
    TelemetryDeadlineRemaining(deadline, &remaining, status);
    if (0 != status)
    {
        goto cleanup;
    }
    snprintf(startup, sizeof(startup), "%d", (remaining > 500) ? 500 : remaining);

    if ((0 != pipe2(descriptors, O_CLOEXEC)) || (0 != pipe2(descriptors + 2, O_CLOEXEC)))
    {
        status = errno;
        operation = "pipe2";
        goto cleanup;
    }

    for (i = 0; i < 4; ++i)
    {
        if (0 != (status = PrepareDescriptor(&descriptors[i], log)))
        {
            goto cleanup;
        }
    }

    for (i = 1; i <= 2; ++i)
    {
        flags = fcntl(descriptors[i], F_GETFL);
        operation = (flags < 0) ? "fcntl(F_GETFL)" : "fcntl(F_SETFL)";
        if ((flags < 0) || (0 != fcntl(descriptors[i], F_SETFL, flags | O_NONBLOCK)))
        {
            status = errno;
            goto cleanup;
        }
    }

    operation = "posix_spawn_file_actions_init";
    if (0 != (status = posix_spawn_file_actions_init(&actions)))
    {
        goto cleanup;
    }

    haveActions = true;

    operation = "posix_spawn_file_actions_adddup2";
    if ((0 != (status = posix_spawn_file_actions_adddup2(&actions, descriptors[0], STDIN_FILENO))) ||
        (0 != (status = posix_spawn_file_actions_adddup2(&actions, descriptors[3], STDOUT_FILENO))))
    {
        goto cleanup;
    }

    operation = "posix_spawn_file_actions_addclose";
    for (i = 0; i < 4; ++i)
    {
        if (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[i])))
        {
            goto cleanup;
        }
    }

    operation = "posix_spawn_file_actions_addopen";
    if (0 != (status = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0)))
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
    TelemetryDeadlineRemaining(deadline, &remaining, status);
    if (0 != status)
    {
        goto cleanup;
    }

    operation = "posix_spawn";
    if (0 == (status = posix_spawn(&child, worker->path, &actions, &attributes, arguments, environ)))
    {
        worker->child = child;
        worker->writeDescriptor = descriptors[1];
        worker->readDescriptor = descriptors[2];
        worker->sequence = 0;
        descriptors[1] = descriptors[2] = -1;
    }

cleanup:
    if ((0 != status) && (ENOENT != status) && (ENOTDIR != status))
    {
        OsConfigLogInfo(log, "StartChild: %s failed with %d (%s)", operation, status, strerror(status));
    }

    if (haveActions)
    {
        if (0 != (error = posix_spawn_file_actions_destroy(&actions)))
        {
            OsConfigLogInfo(log, "StartChild: posix_spawn_file_actions_destroy failed with %d (%s)", error, strerror(error));

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
            OsConfigLogInfo(log, "StartChild: posix_spawnattr_destroy failed with %d (%s)", error, strerror(error));

            if (0 == status)
            {
                status = error;
            }
        }
    }

    for (i = 0; i < 4; ++i)
    {
        if ((descriptors[i] >= 0) && (0 != close(descriptors[i])))
        {
            error = errno;
            OsConfigLogInfo(log, "StartChild: close failed with %d (%s)", error, strerror(error));

            if (0 == status)
            {
                status = error;
            }
        }
    }

    if (0 == status)
    {
        status = ReceiveReply(worker, TELEMETRY_WORKER_READY, 0, NULL, 0, deadline, log);
    }

finished:
    // Do not repeatedly spawn an unusable executable or negotiate a broken READY.
    // Resource exhaustion and timeouts can still recover on a later operation.
    if ((0 == worker->startupStatus) &&
        ((EINVAL == status) || (ENOTSUP == status) || (ENOENT == status) || (EACCES == status) || (ENOEXEC == status) ||
        (ENOTDIR == status) || (ELOOP == status) || (ENAMETOOLONG == status) || (EPROTO == status) || (EPIPE == status)))
    {
        worker->startupStatus = status;
        if ((ENOENT != status) && (ENOTDIR != status))
        {
            OsConfigLogInfo(log, "StartChild: disabling startup for this invocation after %d (%s)", status, strerror(status));
        }
    }

    return status;
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
    else
    {
        operation = "TelemetryMonotonicTime";
        TelemetryMonotonicTime(&now, status);
    }

    if (0 == status)
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
            created->readDescriptor = -1;
            created->writeDescriptor = -1;
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

        OsConfigLogInfo(log, "TelemetryWorkerCreate: %s failed with %d (%s)", operation, status, strerror(status));
        return status;
    }

    *worker = created;

    return 0;
}

int TelemetryWorkerAccountPreparation(TelemetryWorker* worker, int64_t started, OsConfigLogHandle log)
{
    int status = 0;

    if ((NULL == worker) || (started <= 0))
    {
        OsConfigLogInfo(log, "TelemetryWorkerAccountPreparation: argument validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if ((0 == (status = ChargeBudget(worker, started, log))) && (worker->budget <= 0))
    {
        status = ETIMEDOUT;
        OsConfigLogInfo(log, "TelemetryWorkerAccountPreparation: remaining budget check failed with %d (%s)", status, strerror(status));
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
    unsigned char header[TELEMETRY_WORKER_HEADER_SIZE] = {0};
    unsigned char replyBytes[8] = {0};
    int remaining = 0;
    bool sendStarted = false;
    int error = 0;
    const char* operation = "OperationDeadline";

    if (NULL == worker)
    {
        OsConfigLogInfo(log, "TelemetryWorkerSendEvent: invocation pointer validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    if (worker->suppressed)
    {
        OsConfigLogInfo(log, "TelemetryWorkerSendEvent: invocation suppression check failed with %d (%s)", ECANCELED, strerror(ECANCELED));
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
    TelemetryDeadlineRemaining(deadline, &remaining, status);
    if (0 != status)
    {
        goto failed;
    }
    request.deadline = (remaining > 500) ? 500 : (uint64_t)remaining;
    TelemetryEncodeFrame(&request, header);
    sendStarted = true;

    operation = "Transfer";
    if ((0 != (status = Transfer(worker->writeDescriptor, header, sizeof(header), true, deadline, log))) ||
        (0 != (status = Transfer(worker->writeDescriptor, payload, size, true, deadline, log))))
    {
        goto failed;
    }

    operation = "ReceiveReply";
    if (0 != (status = ReceiveReply(worker, request.operation, request.sequence, replyBytes, sizeof(replyBytes), deadline, log)))
    {
        goto failed;
    }

    reply.status = TelemetryReadWord(replyBytes);
    reply.suppressed = TelemetryReadWord(replyBytes + 4);
    if ((reply.status > 7) || (reply.suppressed > 1))
    {
        operation = "reply validation";
        status = EPROTO;
        goto failed;
    }

    worker->suppressed = 0 != reply.suppressed;
    operation = "SendEvent(worker)";
    status = WorkerStatus(reply.status);
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

    if ((0 != status) && (ENOENT != worker->startupStatus) && (ENOTDIR != worker->startupStatus))
    {
        OsConfigLogInfo(log, "TelemetryWorkerSendEvent: %s failed with %d (%s)", operation, status, strerror(status));
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
        OsConfigLogInfo(log, "TelemetryWorkerDestroy: context pointer validation failed with %d (%s)", EINVAL, strerror(EINVAL));
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
        OsConfigLogInfo(log, "TelemetryWorkerDestroy: OperationDeadline failed with %d (%s)", timingStatus, strerror(timingStatus));

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
