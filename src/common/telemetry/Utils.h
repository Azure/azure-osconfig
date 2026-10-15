// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_UTILS_H
#define TELEMETRY_UTILS_H

#include <Logging.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <time.h>

#define TELEMETRY_MAX_RESOLVED_ADDRESSES 8
#define TELEMETRY_RESOLVER_PROTOCOL_VERSION 1
#define TELEMETRY_RESOLVER_ADDRESS_LIMIT 8
#define TELEMETRY_RESOLVER_HOST_LIMIT 253

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

#define TelemetryArmDeadline(TIMER, DEADLINE, STATUS) do {\
    timer_t _telemetryArmTimer = (TIMER);\
    int64_t _telemetryArmDeadline = (DEADLINE);\
    int* _telemetryArmStatus = &(STATUS);\
    struct itimerspec _telemetryArmExpiration = {{0, 0}, {0, 0}};\
    int _telemetryArmRemaining = 0;\
    int _telemetryArmResult = 0;\
    if ((_telemetryArmDeadline <= 0) || ((int64_t)(time_t)(_telemetryArmDeadline / 1000000000) != (_telemetryArmDeadline / 1000000000))) {\
        _telemetryArmResult = EINVAL;\
    }\
    else {\
        TelemetryDeadlineRemaining(_telemetryArmDeadline, &_telemetryArmRemaining, _telemetryArmResult);\
        if (0 == _telemetryArmResult) {\
            _telemetryArmExpiration.it_value.tv_sec = (time_t)(_telemetryArmDeadline / 1000000000);\
            _telemetryArmExpiration.it_value.tv_nsec = (long)(_telemetryArmDeadline % 1000000000);\
            _telemetryArmResult = (0 != timer_settime(_telemetryArmTimer, TIMER_ABSTIME, &_telemetryArmExpiration, NULL)) ? ((0 != errno) ? errno : EIO) : 0;\
        }\
    }\
    *_telemetryArmStatus = _telemetryArmResult;\
} while (0)

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TelemetryResolvedHost
{
    struct sockaddr_storage addresses[TELEMETRY_MAX_RESOLVED_ADDRESSES];
    socklen_t lengths[TELEMETRY_MAX_RESOLVED_ADDRESSES];
    size_t count;
} TelemetryResolvedHost;

// workerPath is a trusted absolute path to the matching resolver executable.
// Returns zero or a logged errno value; result is cleared on failure.
// Addresses have port zero; the connection layer supplies the destination port.
// Calls must be serialized. The host must not reap this component's child or
// change SIGCHLD handling during the call. Nondefault SIGCHLD handling is rejected.
// The deadline covers spawning/resolution. Termination is followed by waitpid:
// kernel scheduling/reaping cannot be given an unconditional wall-clock bound.
// Production callers must supply their valid log handle.
int TelemetryResolveHost(const char* workerPath, const char* host, int timeoutMilliseconds, TelemetryResolvedHost* result, OsConfigLogHandle log);

// Fixed-size, native-endian IPC between matching executables on the same host.
typedef struct TelemetryResolverAddress
{
    uint32_t family;
    uint32_t scopeId;
    unsigned char bytes[16];
} TelemetryResolverAddress;

typedef struct TelemetryResolverReply
{
    uint32_t version;
    int32_t error;
    int32_t lookupError;
    uint32_t count;
    TelemetryResolverAddress addresses[TELEMETRY_RESOLVER_ADDRESS_LIMIT];
} TelemetryResolverReply;

// Lookup runs only in an exec'd worker; errors travel in the reply to its parent.
void TelemetryLookupHost(const char* host, TelemetryResolverReply* reply);
int TelemetryDecodeResolverReply(const TelemetryResolverReply* reply, struct TelemetryResolvedHost* result);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_UTILS_H
