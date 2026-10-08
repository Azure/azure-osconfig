// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef RESOLVER_H
#define RESOLVER_H

#include <Logging.h>
#include <stddef.h>
#include <sys/socket.h>

#define TELEMETRY_MAX_RESOLVED_ADDRESSES 8

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

#ifdef __cplusplus
}
#endif

#endif // RESOLVER_H
