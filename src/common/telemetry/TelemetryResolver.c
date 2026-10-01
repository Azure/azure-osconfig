// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryResolver.h"
#include "TelemetryResolverProtocol.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char** environ;

_Static_assert(TELEMETRY_MAX_RESOLVED_ADDRESSES == TELEMETRY_RESOLVER_ADDRESS_LIMIT,
    "Resolver protocol and public address limits must match");

static int RemainingMilliseconds(const struct timespec* deadline, int* remaining)
{
    struct timespec now;
    int64_t nanoseconds;

    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return errno;
    }

    nanoseconds = ((int64_t)deadline->tv_sec - (int64_t)now.tv_sec) * INT64_C(1000000000) +
        deadline->tv_nsec - now.tv_nsec;
    if (nanoseconds <= 0)
    {
        return ETIMEDOUT;
    }

    nanoseconds = (nanoseconds + 999999) / 1000000;
    *remaining = (nanoseconds > INT_MAX) ? INT_MAX : (int)nanoseconds;
    return 0;
}

static int PreparePipeDescriptor(int* descriptor)
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
    else if (0 != fcntl(*descriptor, F_SETFD, FD_CLOEXEC))
    {
        return errno;
    }

    return 0;
}

static int DecodeReply(const TelemetryResolverReply* reply, TelemetryResolvedHost* result)
{
    size_t i;

    if ((TELEMETRY_RESOLVER_PROTOCOL_VERSION != reply->version) ||
        (reply->count > TELEMETRY_RESOLVER_ADDRESS_LIMIT) || (reply->error < 0) ||
        ((0 != reply->error) && (0 != reply->count)))
    {
        return EPROTO;
    }
    if (0 != reply->error)
    {
        return reply->error;
    }
    if ((0 == reply->count) || (0 != reply->lookupError))
    {
        return EPROTO;
    }

    for (i = 0; i < reply->count; ++i)
    {
        if (AF_INET == reply->addresses[i].family)
        {
            struct sockaddr_in address = {0};
            address.sin_family = AF_INET;
            memcpy(&address.sin_addr, reply->addresses[i].bytes, sizeof(address.sin_addr));
            memcpy(&result->addresses[i], &address, sizeof(address));
            result->lengths[i] = sizeof(address);
        }
        else if (AF_INET6 == reply->addresses[i].family)
        {
            struct sockaddr_in6 address = {0};
            address.sin6_family = AF_INET6;
            address.sin6_scope_id = reply->addresses[i].scopeId;
            memcpy(&address.sin6_addr, reply->addresses[i].bytes, sizeof(address.sin6_addr));
            memcpy(&result->addresses[i], &address, sizeof(address));
            result->lengths[i] = sizeof(address);
        }
        else
        {
            return EPROTO;
        }
    }

    result->count = reply->count;
    return 0;
}

int TelemetryResolveHost(const char* workerPath, const char* host, int timeoutMilliseconds,
    TelemetryResolvedHost* result, OsConfigLogHandle log)
{
    TelemetryResolverReply reply = {0};
    TelemetryResolvedHost resolved = {0};
    struct timespec deadline = {0};
    struct sigaction childAction = {0};
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    sigset_t defaults;
    sigset_t mask;
    int descriptors[2] = {-1, -1};
    pid_t child = -1;
    int childStatus = 0;
    int status = 0;
    int remaining = 0;
    bool actionsInitialized = false;
    bool attributesInitialized = false;
    bool reaped = false;
    bool eof = false;
    unsigned char bytes[sizeof(reply) + 1];
    size_t received = 0;
    char seconds[32];
    char nanoseconds[16];
    char* arguments[] = {(char*)workerPath, (char*)host, seconds, nanoseconds, NULL};
    const char* stage = "arguments";

    if (NULL != result)
    {
        memset(result, 0, sizeof(*result));
    }
    if ((NULL == result) || (NULL == workerPath) || ('/' != workerPath[0]) ||
        (NULL == host) || ('\0' == host[0]) ||
        (strnlen(host, TELEMETRY_RESOLVER_HOST_LIMIT + 1) > TELEMETRY_RESOLVER_HOST_LIMIT) ||
        (timeoutMilliseconds <= 0))
    {
        status = EINVAL;
        goto cleanup;
    }

    stage = "host child ownership";
    if (0 != sigaction(SIGCHLD, NULL, &childAction))
    {
        status = errno;
        goto cleanup;
    }
    if ((SIG_DFL != childAction.sa_handler) || (0 != (childAction.sa_flags & SA_NOCLDWAIT)))
    {
        status = ENOTSUP;
        goto cleanup;
    }

    stage = "deadline";
    if (0 != clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        status = errno;
        goto cleanup;
    }
    if ((int64_t)deadline.tv_sec > INT32_MAX - timeoutMilliseconds / 1000 - 1)
    {
        status = EOVERFLOW;
        goto cleanup;
    }
    deadline.tv_sec += timeoutMilliseconds / 1000;
    deadline.tv_nsec += (long)(timeoutMilliseconds % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        ++deadline.tv_sec;
        deadline.tv_nsec -= 1000000000L;
    }
    snprintf(seconds, sizeof(seconds), "%ld", (long)deadline.tv_sec);
    snprintf(nanoseconds, sizeof(nanoseconds), "%ld", deadline.tv_nsec);

    stage = "pipe";
    if (0 != pipe(descriptors))
    {
        status = errno;
        goto cleanup;
    }
    if ((0 != (status = PreparePipeDescriptor(&descriptors[0]))) ||
        (0 != (status = PreparePipeDescriptor(&descriptors[1]))))
    {
        goto cleanup;
    }
    if (0 != fcntl(descriptors[0], F_SETFL, O_NONBLOCK))
    {
        status = errno;
        goto cleanup;
    }

    stage = "spawn setup";
    if (0 != (status = posix_spawn_file_actions_init(&actions)))
    {
        goto cleanup;
    }
    actionsInitialized = true;
    if ((0 != (status = posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDOUT_FILENO))) ||
        (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[0]))) ||
        (0 != (status = posix_spawn_file_actions_addclose(&actions, descriptors[1]))) ||
        (0 != (status = posix_spawnattr_init(&attributes))))
    {
        goto cleanup;
    }
    attributesInitialized = true;
    sigemptyset(&defaults);
    sigaddset(&defaults, SIGALRM);
    sigaddset(&defaults, SIGPIPE);
    sigemptyset(&mask);
    if ((0 != (status = posix_spawnattr_setsigdefault(&attributes, &defaults))) ||
        (0 != (status = posix_spawnattr_setsigmask(&attributes, &mask))) ||
        (0 != (status = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK))))
    {
        goto cleanup;
    }

    stage = "spawn";
    if (0 != (status = RemainingMilliseconds(&deadline, &remaining)))
    {
        goto cleanup;
    }
    status = posix_spawn(&child, workerPath, &actions, &attributes, arguments, environ);
    if (0 != status)
    {
        child = -1;
        goto cleanup;
    }
    stage = "closing parent writer";
    if (0 != close(descriptors[1]))
    {
        status = errno;
        descriptors[1] = -1;
        goto cleanup;
    }
    descriptors[1] = -1;

    stage = "response";
    for (;;)
    {
        struct pollfd descriptor = {descriptors[0], POLLIN, 0};
        ssize_t size = read(descriptors[0], bytes + received, sizeof(bytes) - received);
        if (size > 0)
        {
            received += (size_t)size;
            if (received > sizeof(reply))
            {
                status = EPROTO;
                goto cleanup;
            }
        }
        else if (0 == size)
        {
            eof = true;
        }
        else if ((EINTR != errno) && (EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
            status = errno;
            goto cleanup;
        }

        if (!reaped)
        {
            pid_t waited = waitpid(child, &childStatus, WNOHANG);
            if (child == waited)
            {
                reaped = true;
            }
            else if ((waited < 0) && (EINTR != errno))
            {
                status = errno;
                // ECHILD means the host reaped it; never signal a potentially reused PID.
                reaped = (ECHILD == status);
                goto cleanup;
            }
        }
        if (reaped && eof)
        {
            break;
        }
        if (0 != (status = RemainingMilliseconds(&deadline, &remaining)))
        {
            goto cleanup;
        }
        if (eof && (remaining > 10))
        {
            remaining = 10;
        }
        if ((poll(eof ? NULL : &descriptor, eof ? 0 : 1, remaining) < 0) && (EINTR != errno))
        {
            status = errno;
            goto cleanup;
        }
    }

    stage = "worker exit";
    if (WIFSIGNALED(childStatus) && (SIGALRM == WTERMSIG(childStatus)))
    {
        status = ETIMEDOUT;
        goto cleanup;
    }
    if (!WIFEXITED(childStatus) || (0 != WEXITSTATUS(childStatus)))
    {
        status = EIO;
        goto cleanup;
    }
    stage = "deadline";
    if (0 != (status = RemainingMilliseconds(&deadline, &remaining)))
    {
        goto cleanup;
    }
    stage = "reply validation";
    if (sizeof(reply) != received)
    {
        status = EPROTO;
        goto cleanup;
    }
    memcpy(&reply, bytes, sizeof(reply));
    status = DecodeReply(&reply, &resolved);

cleanup:
    if ((child > 0) && !reaped)
    {
        pid_t waited;
        if ((0 != kill(child, SIGKILL)) && (ESRCH != errno))
        {
            OsConfigLogInfo(log, "TelemetryResolveHost: Cannot terminate resolver child (errno=%d)", errno);
        }
        do
        {
            waited = waitpid(child, &childStatus, 0);
        } while ((waited < 0) && (EINTR == errno));
        if (waited < 0)
        {
            int waitError = errno;
            OsConfigLogInfo(log, "TelemetryResolveHost: Cannot reap resolver child (errno=%d)", waitError);
            if (0 == status)
            {
                status = waitError;
            }
        }
    }
    if (actionsInitialized)
    {
        int destroyError = posix_spawn_file_actions_destroy(&actions);
        if (0 != destroyError)
        {
            OsConfigLogInfo(log, "TelemetryResolveHost: Cannot release spawn actions (status=%d)", destroyError);
            if (0 == status)
            {
                status = destroyError;
            }
        }
    }
    if (attributesInitialized)
    {
        int destroyError = posix_spawnattr_destroy(&attributes);
        if (0 != destroyError)
        {
            OsConfigLogInfo(log, "TelemetryResolveHost: Cannot release spawn attributes (status=%d)", destroyError);
            if (0 == status)
            {
                status = destroyError;
            }
        }
    }
    for (size_t i = 0; i < ARRAY_SIZE(descriptors); ++i)
    {
        if ((descriptors[i] >= 0) && (0 != close(descriptors[i])))
        {
            int closeError = errno;
            OsConfigLogInfo(log, "TelemetryResolveHost: Cannot close IPC descriptor (errno=%d)", closeError);
            if (0 == status)
            {
                status = closeError;
            }
        }
    }

    if (0 != status)
    {
        OsConfigLogInfo(log, "TelemetryResolveHost: %s failed (status=%d, lookup=%d, child=%d)",
            stage, status, reply.lookupError, childStatus);
    }
    else
    {
        *result = resolved;
        OsConfigLogDebug(log, "TelemetryResolveHost: Resolved %zu addresses", result->count);
    }
    return status;
}
