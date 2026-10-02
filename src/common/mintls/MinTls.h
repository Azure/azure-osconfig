// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MIN_TLS_H
#define MIN_TLS_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MinTls MinTls;

// Serialized, owned-process-only TLS client. All deadlines are absolute
// CLOCK_MONOTONIC nanoseconds. The caller must bound blocking filesystem/RNG/
// crypto work with its process watchdog. No queues, DNS, TCP connect or threads.
// caFile overrides trust discovery. Otherwise SSL_CERT_FILE or the first
// recognized OS PEM bundle is used. No built-in roots or trust-on-first-use.
int MinTlsCreate(MinTls** tls, const char* caFile, int64_t deadline, OsConfigLogHandle log);

// Borrows a connected nonblocking socket; never closes it. Requires a verified
// chain, certificate dates, and exact DNS/IP identity (DNS wildcards are allowed
// only by the core's X.509 rules). SNI is sent only for DNS identities.
int MinTlsHandshake(MinTls* tls, int descriptor, const char* peer,
    int64_t deadline, OsConfigLogHandle log);
int MinTlsWrite(MinTls* tls, const void* bytes, size_t size,
    int64_t deadline, OsConfigLogHandle log);
// Only a TLS close_notify is EOF; raw socket EOF is an error.
int MinTlsRead(MinTls* tls, void* bytes, size_t capacity, size_t* size,
    bool* endOfStream, int64_t deadline, OsConfigLogHandle log);
void MinTlsDestroy(MinTls** tls, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif
