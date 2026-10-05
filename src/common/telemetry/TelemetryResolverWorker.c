// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryResolverProtocol.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void SendReplyAndExit(const TelemetryResolverReply* reply)
{
    const unsigned char* bytes = (const unsigned char*)reply;
    size_t remaining = sizeof(*reply);
    ssize_t written = 0;

    while (remaining > 0)
    {
        written = write(STDOUT_FILENO, bytes, remaining);
        if (written > 0)
        {
            bytes += written;
            remaining -= (size_t)written;
        }
        else if ((written < 0) && (EINTR == errno))
        {
            continue;
        }
        else
        {
            // A broken IPC channel is reported by the parent, which owns the log.
            _exit(EXIT_FAILURE);
        }
    }

    _exit(EXIT_SUCCESS);
}

static int ParseNonnegativeLong(const char* text, long* value)
{
    char* end = NULL;

    if ((NULL == text) || (text[0] < '0') || (text[0] > '9'))
    {
        return EINVAL;
    }

    errno = 0;
    *value = strtol(text, &end, 10);
    return ((0 != errno) || ('\0' != *end) || (*value < 0)) ? EINVAL : 0;
}

int main(int argc, char** argv)
{
    TelemetryResolverReply reply = {0};
    struct sigevent notification = {0};
    struct sigaction action = {0};
    struct itimerspec expiration = {0};
    struct timespec now = {0};
    sigset_t timerSignal = {0};
    timer_t timer = 0;
    long seconds = 0;
    long nanoseconds = 0;
    size_t hostLength = (argc > 1) ? strnlen(argv[1], TELEMETRY_RESOLVER_HOST_LIMIT + 1) : 0;

    reply.version = TELEMETRY_RESOLVER_PROTOCOL_VERSION;

    if ((4 != argc) || (0 == hostLength) || (hostLength > TELEMETRY_RESOLVER_HOST_LIMIT) ||
        (0 != ParseNonnegativeLong(argv[2], &seconds)) ||
        (0 != ParseNonnegativeLong(argv[3], &nanoseconds)) || (nanoseconds >= 1000000000L))
    {
        reply.error = EINVAL;
        SendReplyAndExit(&reply);
    }

    expiration.it_value.tv_sec = (time_t)seconds;
    expiration.it_value.tv_nsec = nanoseconds;
    if (((long)expiration.it_value.tv_sec != seconds) || (0 != clock_gettime(CLOCK_MONOTONIC, &now)))
    {
        reply.error = errno ? errno : EINVAL;
        SendReplyAndExit(&reply);
    }

    if ((now.tv_sec > expiration.it_value.tv_sec) ||
        ((now.tv_sec == expiration.it_value.tv_sec) && (now.tv_nsec >= nanoseconds)))
    {
        reply.error = ETIMEDOUT;
        SendReplyAndExit(&reply);
    }

    // This is a fresh executable: timer/signal changes cannot affect the host.
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigemptyset(&timerSignal);
    sigaddset(&timerSignal, SIGALRM);
    if ((0 != sigaction(SIGALRM, &action, NULL)) ||
        (0 != sigprocmask(SIG_UNBLOCK, &timerSignal, NULL)))
    {
        reply.error = errno;
        SendReplyAndExit(&reply);
    }

    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = SIGALRM;
    if ((0 != timer_create(CLOCK_MONOTONIC, &notification, &timer)) ||
        (0 != timer_settime(timer, TIMER_ABSTIME, &expiration, NULL)))
    {
        reply.error = errno;
        SendReplyAndExit(&reply);
    }

    TelemetryLookupHost(argv[1], &reply);

    // Keep the timer armed through IPC; process exit releases it.
    SendReplyAndExit(&reply);
    return EXIT_FAILURE;
}
