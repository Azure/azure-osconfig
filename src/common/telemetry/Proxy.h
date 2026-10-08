// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PROXY_H
#define PROXY_H

#include <Logging.h>
#include <stddef.h>
#include <stdint.h>

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

typedef struct TelemetryHttpProxy
{
    char host[254];
    uint16_t port;
    // Contains credentials when present. Only send to the configured proxy.
    char authorization[710];
} TelemetryHttpProxy;

// Initial plain HTTP CONNECT adapter. Other schemes return ENOTSUP, never direct.
// Supports ASCII DNS, bracketed IPv6, explicit ports and percent-decoded Basic
// userinfo. Default HTTP proxy port is 1080, as in the pinned curl configuration.
// Non-root URL paths, encoded hosts, scoped IPv6 and IDNs are not implemented.
// Input must not overlap output, which is cleared on failure. Both APIs log
// status only, not credentials. BuildConnect requires unmodified parser output.
int TelemetryProxyParseHttp(const char* url, TelemetryHttpProxy* proxy, OsConfigLogHandle log);
int TelemetryProxyBuildConnect(const TelemetryHttpProxy* proxy, char* bytes, size_t capacity, size_t* size, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // PROXY_H
