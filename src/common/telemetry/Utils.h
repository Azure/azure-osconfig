// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_UTILS_H
#define TELEMETRY_UTILS_H

#include <Logging.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#define TELEMETRY_MAX_RESOLVED_ADDRESSES 8
#define TELEMETRY_RESOLVER_PROTOCOL_VERSION 1
#define TELEMETRY_RESOLVER_ADDRESS_LIMIT 8
#define TELEMETRY_RESOLVER_HOST_LIMIT 253

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
