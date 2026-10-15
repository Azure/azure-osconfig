// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ARIA_INTERFACE_H
#define ARIA_INTERFACE_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELEMETRY_ARIA_HOST "mobile.events.data.microsoft.com"
#define TELEMETRY_ARIA_PATH "/OneCollector/1.0/"
#define TELEMETRY_HTTP_HEADER_LIMIT 8192
#define TELEMETRY_HTTP_BODY_LIMIT 8192
#define TELEMETRY_HTTP_LINE_LIMIT 2048
#define TELEMETRY_HTTP_WIRE_LIMIT 32768
#define TELEMETRY_HTTP_CONTROL_LIMIT 16
#define TELEMETRY_HTTP_TOKEN_LIMIT 512
#define TELEMETRY_HTTP_VERSION_LIMIT 100
#define TELEMETRY_PROXY_URL_LIMIT 4096
#define TELEMETRY_PROXY_BYPASS_LIMIT 8192

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum TelemetryAcceptance
{
    TelemetryUnconfirmed,
    TelemetryAccepted,
    TelemetryRejected
} TelemetryAcceptance;

typedef enum TelemetryHttpControlKind
{
    TelemetryRetryAfter,
    TelemetryKillTokens,
    TelemetryKillDuration,
    TelemetryTimeDeltaMillis
} TelemetryHttpControlKind;

typedef struct TelemetryHttpControl
{
    TelemetryHttpControlKind kind;
    size_t offset;
} TelemetryHttpControl;

// Caller-owned incremental parser. Initialize before each response; do not
// modify its fields while feeding it. It owns no socket, queue, or event data.
typedef struct TelemetryHttpResponse
{
    bool complete;
    bool reusable;
    unsigned int status;
    TelemetryAcceptance acceptance;
    size_t bodySize;
    char body[TELEMETRY_HTTP_BODY_LIMIT + 1];
    size_t controlCount;
    TelemetryHttpControl controls[TELEMETRY_HTTP_CONTROL_LIMIT];
    char controlValues[TELEMETRY_HTTP_HEADER_LIMIT + 1];

    // Parser bookkeeping; not an IPC layout or stable public ABI.
    unsigned int state;
    unsigned int minorVersion;
    unsigned int informationalCount;
    unsigned int headerCount;
    size_t headerBytes;
    size_t wireBytes;
    size_t controlBytes;
    size_t remaining;
    size_t contentLength;
    size_t lineSize;
    bool carriageReturn;
    bool hasLength;
    bool chunked;
    bool connectionClose;
    bool connectionKeepAlive;
    char line[TELEMETRY_HTTP_LINE_LIMIT + 1];
} TelemetryHttpResponse;

// Formats only the headers for one uncompressed encoded event. The caller
// writes these bytes followed by exactly eventSize binary bytes. On failure,
// *headerSize is zero and the header buffer is unchanged. No token is logged.
int TelemetryHttpBuildRequest(const char* token, const char* clientVersion, int64_t uploadTimeMilliseconds, size_t eventSize,
    char* headers, size_t capacity, size_t* headerSize, OsConfigLogHandle log);

int TelemetryHttpResponseInitialize(TelemetryHttpResponse* response, OsConfigLogHandle log);

// Zero means these bytes were processed, NOT collector acceptance. Require
// (complete) && (TelemetryAccepted == acceptance) to report confirmed acceptance.
// endOfStream means a valid transport EOF, not an unauthenticated TLS truncation.
// Preserved control headers must be handled by the invocation owner before its
// next send; the parser neither ignores them nor implements suppression policy.
int TelemetryHttpResponseFeed(TelemetryHttpResponse* response, const void* bytes, size_t size, bool endOfStream, OsConfigLogHandle log);

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

typedef struct TelemetryTransport TelemetryTransport;

// Owned process only, serialized calls, SIGPIPE ignored and independent operation
// self-timer armed. Create snapshots inherited routing but performs no networking.
// Initial routes: direct or plain HTTP CONNECT, optionally URL Basic credentials.
// Unsupported proxy configuration fails explicitly without a direct fallback.
int TelemetryTransportCreate(TelemetryTransport** transport, OsConfigLogHandle log);

// One encoded record per request. Reuses a verified connection; never retries
// an event, redirects, or changes route following failure. A stale idle session
// can reconnect before the first request byte; after sending starts only a later
// call may reconnect. The same deadline covers DNS, TCP, CONNECT, TLS and HTTP.
// Zero means a complete parsed response: inspect acceptance separately.
// Retry/kill headers or HTTP429/503 conservatively latch suppression for this
// context; subsequent calls fail ECANCELED. Cross-process suppression is pending.
int TelemetryTransportSend(TelemetryTransport* transport, const char* token, const char* clientVersion, int64_t uploadTimeMilliseconds,
    const void* event, size_t eventSize, int64_t deadline, TelemetryHttpResponse* response, OsConfigLogHandle log);
bool TelemetryTransportSuppressed(const TelemetryTransport* transport);
void TelemetryTransportDestroy(TelemetryTransport** transport, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // ARIA_INTERFACE_H
