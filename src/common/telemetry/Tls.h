// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TLS_H
#define TLS_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TelemetryTls TelemetryTls;

// Worker-only, serialized API; never initialize this provider inside gc_worker.
// The owned process must ignore SIGPIPE and have its self-exit timer armed.
// Provider libraries remain mapped until that process exits; objects do not.
// All deadlines are absolute CLOCK_MONOTONIC nanoseconds, never reset by I/O.
// Requires system OpenSSL, preferred in order 3, 1.1, then 1.0.2. No bundled TLS.
// An initialization, policy, handshake or verification failure never retries
// through an older provider. OpenSSL 1.0.1 and earlier are unsupported.
int TelemetryTlsCreate(TelemetryTls** tls, int64_t deadline, OsConfigLogHandle log);

// Borrows an already connected O_NONBLOCK socket (direct or tunneled). Does not
// discover routing, connect TCP, own/close the descriptor, or bypass a proxy.
// Verifies the OS trust chain and expected DNS/IP identity. Failure clears the
// TLS connection; a later, distinct operation may attach another socket.
int TelemetryTlsHandshake(TelemetryTls* tls, int descriptor, const char* peer, int64_t deadline, OsConfigLogHandle log);

// Writes the complete buffer, continuing identical TLS writes on WANT_READ/
// WANT_WRITE, never replaying a request. Any I/O failure discards the connection.
int TelemetryTlsWrite(TelemetryTls* tls, const void* bytes, size_t size, int64_t deadline, OsConfigLogHandle log);

// A successful read returns bytes or authenticated close_notify, never both.
// An abrupt socket EOF is an error, not a valid close-delimited HTTP boundary.
int TelemetryTlsRead(TelemetryTls* tls, void* bytes, size_t capacity, size_t* size, bool* endOfStream, int64_t deadline, OsConfigLogHandle log);

// Frees connection/context objects without network I/O or closing the borrowed
// descriptor. No shutdown replay or blocking wait for a peer close_notify.
void TelemetryTlsDestroy(TelemetryTls** tls, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // TLS_H
