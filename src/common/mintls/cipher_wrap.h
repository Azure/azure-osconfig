// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file cipher_wrap.h
 *
 * \brief Cipher wrappers.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/cipher_wrap.h.
 */
#ifndef MBEDTLS_CIPHER_WRAP_H
#define MBEDTLS_CIPHER_WRAP_H

#include "build_info.h"

#include "cipher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Support for GCM either through Mbed TLS SW implementation or PSA */
#define MBEDTLS_CIPHER_HAVE_GCM_VIA_LEGACY_OR_USE_PSA

#define MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA

#if defined(MBEDTLS_CIPHER_HAVE_GCM_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CCM_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CCM_STAR_NO_TAG_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_CIPHER_HAVE_CHACHAPOLY_VIA_LEGACY_OR_USE_PSA)
#define MBEDTLS_CIPHER_HAVE_SOME_AEAD_VIA_LEGACY_OR_USE_PSA
#endif

/**
 * Base cipher information. The non-mode specific functions and values.
 */
struct mbedtls_cipher_base_t {
    /** Base Cipher type (e.g. MBEDTLS_CIPHER_ID_AES) */
    mbedtls_cipher_id_t cipher;

    /** Encrypt using ECB */
    int (*ecb_func)(void *ctx, mbedtls_operation_t mode,
                    const unsigned char *input, unsigned char *output, MinTlsDiagnostics* diagnostics);

    /** Set key for encryption purposes */
    int (*setkey_enc_func)(void *ctx, const unsigned char *key,
                           unsigned int key_bitlen, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    /** Set key for decryption purposes */
    int (*setkey_dec_func)(void *ctx, const unsigned char *key,
                           unsigned int key_bitlen, MinTlsDiagnostics* diagnostics);
#endif

    /** Allocate a new context */
    void * (*ctx_alloc_func)(void);

    /** Free the given context */
    void (*ctx_free_func)(void *ctx);

};

typedef struct {
    mbedtls_cipher_type_t type;
    const mbedtls_cipher_info_t *info;
} mbedtls_cipher_definition_t;

extern const mbedtls_cipher_definition_t mbedtls_cipher_definitions[];

extern int mbedtls_cipher_supported[];

extern const mbedtls_cipher_base_t * const mbedtls_cipher_base_lookup_table[];

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CIPHER_WRAP_H */
