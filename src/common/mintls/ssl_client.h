// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 *  TLS 1.2 and 1.3 client-side functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_client.h.
 */

#ifndef MBEDTLS_SSL_CLIENT_H
#define MBEDTLS_SSL_CLIENT_H

#include "common.h"

#include "ssl_misc.h"

#include <stddef.h>

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_client_hello(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

#endif /* MBEDTLS_SSL_CLIENT_H */
