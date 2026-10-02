// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_PROXY_H
#define TELEMETRY_PROXY_H

#include <Logging.h>

#define TELEMETRY_PROXY_URL_LIMIT 4096
#define TELEMETRY_PROXY_BYPASS_LIMIT 8192

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum TelemetryProxyKind
{
    TelemetryProxyUnresolved,
    TelemetryProxyDirect,
    TelemetryProxyConfigured
} TelemetryProxyKind;

typedef struct TelemetryProxySelection
{
    TelemetryProxyKind kind;
    char url[TELEMETRY_PROXY_URL_LIMIT + 1];
} TelemetryProxySelection;

// Worker-only, serialized discovery for the fixed HTTPS Aria destination.
// Uses the inherited environment without changing it or performing DNS/network I/O.
// A configured result is an owned copy, NOT a parsed/validated proxy endpoint.
// It can contain credentials: never log or send it to the collector.
// The transport must validate and honor it, or fail; never fall back to direct.
// On error, selection is unresolved and its URL is cleared. Callers must check
// status and kind before connecting, and discard the selection at worker exit.
int TelemetryProxyDiscover(TelemetryProxySelection* selection, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_PROXY_H
