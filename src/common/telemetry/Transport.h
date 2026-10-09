// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Http.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TelemetryTransport TelemetryTransport;

// Owned process only, serialized calls, SIGPIPE ignored and independent operation
// self-timer armed. Create snapshots inherited routing but performs no networking.
// Initial routes: direct or plain HTTP CONNECT, optionally URL Basic credentials.
// Unsupported proxy configuration fails explicitly without a direct fallback.
int TelemetryTransportCreate(TelemetryTransport** transport, OsConfigLogHandle log);

// One encoded record per request. Reuses a verified connection; never retries
// an event, redirects, or changes route following failure. Only a later call may
// reconnect. The same absolute deadline covers DNS, TCP, CONNECT, TLS and HTTP.
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

#endif
