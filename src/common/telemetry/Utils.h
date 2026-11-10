// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_UTILS_H
#define TELEMETRY_UTILS_H

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>

// STATUS is an int lvalue. Arguments are evaluated once; callers diagnose failures.
#define TelemetryMonotonicTime(VALUE, STATUS) do {\
    int64_t* _telemetryTimeValue = (VALUE);\
    int* _telemetryTimeStatus = &(STATUS);\
    struct timespec _telemetryTimeNow = {0, 0};\
    int _telemetryTimeResult = 0;\
    if (0 != clock_gettime(CLOCK_MONOTONIC, &_telemetryTimeNow)) {\
        _telemetryTimeResult = (0 != errno) ? errno : EIO;\
    }\
    else if ((_telemetryTimeNow.tv_sec < 0) || ((uint64_t)_telemetryTimeNow.tv_sec > (uint64_t)((INT64_MAX / 1000000000) - 1))) {\
        _telemetryTimeResult = EOVERFLOW;\
    }\
    else {\
        *_telemetryTimeValue = ((int64_t)_telemetryTimeNow.tv_sec * INT64_C(1000000000)) + _telemetryTimeNow.tv_nsec;\
    }\
    *_telemetryTimeStatus = _telemetryTimeResult;\
} while (0)

#define TelemetryDeadlineRemaining(DEADLINE, MILLISECONDS, STATUS) do {\
    int64_t _telemetryRemainingDeadline = (DEADLINE);\
    int* _telemetryRemainingOutput = (MILLISECONDS);\
    int* _telemetryRemainingStatus = &(STATUS);\
    int64_t _telemetryRemainingNow = 0;\
    int64_t _telemetryRemainingValue = 0;\
    int _telemetryRemainingResult = 0;\
    TelemetryMonotonicTime(&_telemetryRemainingNow, _telemetryRemainingResult);\
    if (0 == _telemetryRemainingResult) {\
        if (_telemetryRemainingNow >= _telemetryRemainingDeadline) {\
            _telemetryRemainingResult = ETIMEDOUT;\
        }\
        else {\
            _telemetryRemainingValue = ((_telemetryRemainingDeadline - _telemetryRemainingNow) / 1000000) +\
                (0 != ((_telemetryRemainingDeadline - _telemetryRemainingNow) % 1000000));\
            *_telemetryRemainingOutput = (_telemetryRemainingValue > INT_MAX) ? INT_MAX : (int)_telemetryRemainingValue;\
        }\
    }\
    *_telemetryRemainingStatus = _telemetryRemainingResult;\
} while (0)

#endif // TELEMETRY_UTILS_H
