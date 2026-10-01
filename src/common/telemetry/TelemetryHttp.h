// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_HTTP_H
#define TELEMETRY_HTTP_H

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
int TelemetryHttpBuildRequest(const char* token, const char* clientVersion,
    int64_t uploadTimeMilliseconds, size_t eventSize, char* headers,
    size_t capacity, size_t* headerSize, OsConfigLogHandle log);

int TelemetryHttpResponseInitialize(TelemetryHttpResponse* response, OsConfigLogHandle log);

// Zero means these bytes were processed, NOT collector acceptance. Require
// complete && acceptance == TelemetryAccepted to report confirmed acceptance.
// endOfStream means a valid transport EOF, not an unauthenticated TLS truncation.
// Preserved control headers must be handled by the invocation owner before its
// next send; the parser neither ignores them nor implements suppression policy.
int TelemetryHttpResponseFeed(TelemetryHttpResponse* response, const void* bytes,
    size_t size, bool endOfStream, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_HTTP_H
