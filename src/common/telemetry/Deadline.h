// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef DEADLINE_H
#define DEADLINE_H

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>

static inline int TelemetryMonotonicTime(int64_t* value)
{
    struct timespec now = {0, 0};
    int status = 0;

    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        status = (0 != errno) ? errno : EIO;
    }
    else if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > (uint64_t)((INT64_MAX / 1000000000) - 1)))
    {
        status = EOVERFLOW;
    }
    else
    {
        *value = ((int64_t)now.tv_sec * INT64_C(1000000000)) + now.tv_nsec;
    }

    return status;
}

static inline int TelemetryDeadlineRemaining(int64_t deadline, int* milliseconds)
{
    int64_t now = 0;
    int status = 0;
    int64_t remaining = 0;

    if (0 == (status = TelemetryMonotonicTime(&now)))
    {
        if (now >= deadline)
        {
            status = ETIMEDOUT;
        }
        else
        {
            remaining = ((deadline - now) / 1000000) + (0 != ((deadline - now) % 1000000));
            *milliseconds = (remaining > INT_MAX) ? INT_MAX : (int)remaining;
        }
    }

    return status;
}

static inline int TelemetryArmDeadline(timer_t timer, int64_t deadline)
{
    struct itimerspec expiration = {{0, 0}, {0, 0}};
    int remaining = 0;
    int status = 0;

    if ((deadline <= 0) || ((int64_t)(time_t)(deadline / 1000000000) != (deadline / 1000000000)))
    {
        return EINVAL;
    }

    if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        expiration.it_value.tv_sec = (time_t)(deadline / 1000000000);
        expiration.it_value.tv_nsec = (long)(deadline % 1000000000);
        status = (0 != timer_settime(timer, TIMER_ABSTIME, &expiration, NULL)) ? ((0 != errno) ? errno : EIO) : 0;
    }

    return status;
}

#endif // DEADLINE_H
