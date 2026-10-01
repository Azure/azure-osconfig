// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_DEADLINE_H
#define TELEMETRY_DEADLINE_H

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>

static inline int TelemetryMonotonicTime(int64_t* value)
{
    struct timespec now = {0, 0};
    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return (0 != errno) ? errno : EIO;
    }
    if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > (uint64_t)(INT64_MAX / 1000000000 - 1)))
    {
        return EOVERFLOW;
    }
    *value = (int64_t)now.tv_sec * INT64_C(1000000000) + now.tv_nsec;
    return 0;
}

static inline int TelemetryDeadlineRemaining(int64_t deadline, int* milliseconds)
{
    int64_t now = 0;
    int status = TelemetryMonotonicTime(&now);
    if (0 != status) return status;
    if (now >= deadline) return ETIMEDOUT;
    int64_t remaining = (deadline - now) / 1000000 + ((deadline - now) % 1000000 != 0);
    *milliseconds = (remaining > INT_MAX) ? INT_MAX : (int)remaining;
    return 0;
}

#endif // TELEMETRY_DEADLINE_H
