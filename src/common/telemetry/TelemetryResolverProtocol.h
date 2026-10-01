// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_RESOLVER_PROTOCOL_H
#define TELEMETRY_RESOLVER_PROTOCOL_H

#include <stdint.h>

#define TELEMETRY_RESOLVER_PROTOCOL_VERSION 1
#define TELEMETRY_RESOLVER_ADDRESS_LIMIT 8
#define TELEMETRY_RESOLVER_HOST_LIMIT 253

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

struct TelemetryResolvedHost;

#ifdef __cplusplus
extern "C"
{
#endif

// Lookup runs only in an exec'd worker; errors travel in the reply to its parent.
void TelemetryLookupHost(const char* host, TelemetryResolverReply* reply);
int TelemetryDecodeResolverReply(const TelemetryResolverReply* reply, struct TelemetryResolvedHost* result);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_RESOLVER_PROTOCOL_H
