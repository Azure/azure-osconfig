// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file cipher_wrap.c
 *
 * \brief Generic cipher wrapper for Mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/cipher_wrap.c.
 */

#include "common.h"

#include "cipher_wrap.h"
#include "error.h"

#include "aes.h"

#include "gcm.h"

#include "platform.h"

enum mbedtls_cipher_base_index {
    MBEDTLS_CIPHER_BASE_INDEX_AES,
#if defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA)
    MBEDTLS_CIPHER_BASE_INDEX_GCM_AES,
#endif
    /* Prevent compile failure due to empty enum */
    MBEDTLS_CIPHER_BASE_PREVENT_EMPTY_ENUM
};

#if defined(MBEDTLS_GCM_C) && \
    (defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA) || \
    defined(MBEDTLS_ARIA_C) || defined(MBEDTLS_CAMELLIA_C))
/* shared by all GCM ciphers */
void *gcm_ctx_alloc(void)
{
    void *ctx = mbedtls_calloc(1, sizeof(mbedtls_gcm_context));

    if (ctx != NULL) {
        mbedtls_gcm_init((mbedtls_gcm_context *) ctx);
    }

    return ctx;
}

void gcm_ctx_free(void *ctx)
{
    mbedtls_gcm_free(ctx);
    mbedtls_free(ctx);
}
#endif /* MBEDTLS_GCM_C */

int aes_crypt_ecb_wrap(void *ctx, mbedtls_operation_t operation,
                              const unsigned char *input, unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_aes_crypt_ecb((mbedtls_aes_context *) ctx, operation, input, output, diagnostics));
}

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
int aes_setkey_dec_wrap(void *ctx, const unsigned char *key,
                               unsigned int key_bitlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_aes_setkey_dec((mbedtls_aes_context *) ctx, key, key_bitlen, diagnostics));
}
#endif

int aes_setkey_enc_wrap(void *ctx, const unsigned char *key,
                               unsigned int key_bitlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_aes_setkey_enc((mbedtls_aes_context *) ctx, key, key_bitlen, diagnostics));
}

void *aes_ctx_alloc(void)
{
    mbedtls_aes_context *aes = mbedtls_calloc(1, sizeof(mbedtls_aes_context));

    if (aes == NULL) {
        return NULL;
    }

    mbedtls_aes_init(aes);

    return aes;
}

void aes_ctx_free(void *ctx)
{
    mbedtls_aes_free((mbedtls_aes_context *) ctx);
    mbedtls_free(ctx);
}

static const mbedtls_cipher_base_t aes_info = {
    MBEDTLS_CIPHER_ID_AES,
    aes_crypt_ecb_wrap,
    aes_setkey_enc_wrap,
#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    aes_setkey_dec_wrap,
#endif
    aes_ctx_alloc,
    aes_ctx_free
};

static const mbedtls_cipher_info_t aes_128_ecb_info = {
    "AES-128-ECB",
    16,
    0 >> MBEDTLS_IV_SIZE_SHIFT,
    128 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_ECB,
    MBEDTLS_CIPHER_AES_128_ECB,
    0,
    MBEDTLS_CIPHER_BASE_INDEX_AES
};

#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
static const mbedtls_cipher_info_t aes_192_ecb_info = {
    "AES-192-ECB",
    16,
    0 >> MBEDTLS_IV_SIZE_SHIFT,
    192 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_ECB,
    MBEDTLS_CIPHER_AES_192_ECB,
    0,
    MBEDTLS_CIPHER_BASE_INDEX_AES
};

static const mbedtls_cipher_info_t aes_256_ecb_info = {
    "AES-256-ECB",
    16,
    0 >> MBEDTLS_IV_SIZE_SHIFT,
    256 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_ECB,
    MBEDTLS_CIPHER_AES_256_ECB,
    0,
    MBEDTLS_CIPHER_BASE_INDEX_AES
};
#endif

int gcm_aes_setkey_wrap(void *ctx, const unsigned char *key,
                               unsigned int key_bitlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_gcm_setkey((mbedtls_gcm_context *) ctx, MBEDTLS_CIPHER_ID_AES,
                              key, key_bitlen, diagnostics));
}

#if defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA)
static const mbedtls_cipher_base_t gcm_aes_info = {
    MBEDTLS_CIPHER_ID_AES,
    NULL,
    gcm_aes_setkey_wrap,
#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    gcm_aes_setkey_wrap,
#endif
    gcm_ctx_alloc,
    gcm_ctx_free,
};
#endif /* MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA */

#if defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA)
static const mbedtls_cipher_info_t aes_128_gcm_info = {
    "AES-128-GCM",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    128 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_GCM,
    MBEDTLS_CIPHER_AES_128_GCM,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_GCM_AES
};

#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
static const mbedtls_cipher_info_t aes_192_gcm_info = {
    "AES-192-GCM",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    192 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_GCM,
    MBEDTLS_CIPHER_AES_192_GCM,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_GCM_AES
};

static const mbedtls_cipher_info_t aes_256_gcm_info = {
    "AES-256-GCM",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    256 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_GCM,
    MBEDTLS_CIPHER_AES_256_GCM,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_GCM_AES
};
#endif
#endif /* MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA */

#if defined(MBEDTLS_CIPHER_HAVE_CCM_STAR_NO_TAG_AES_VIA_LEGACY_OR_USE_PSA)
static const mbedtls_cipher_info_t aes_128_ccm_star_no_tag_info = {
    "AES-128-CCM*-NO-TAG",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    128 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_AES_128_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_CCM_AES
};

#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
static const mbedtls_cipher_info_t aes_192_ccm_star_no_tag_info = {
    "AES-192-CCM*-NO-TAG",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    192 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_AES_192_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_CCM_AES
};

static const mbedtls_cipher_info_t aes_256_ccm_star_no_tag_info = {
    "AES-256-CCM*-NO-TAG",
    16,
    12 >> MBEDTLS_IV_SIZE_SHIFT,
    256 >> MBEDTLS_KEY_BITLEN_SHIFT,
    MBEDTLS_MODE_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_AES_256_CCM_STAR_NO_TAG,
    MBEDTLS_CIPHER_VARIABLE_IV_LEN,
    MBEDTLS_CIPHER_BASE_INDEX_CCM_AES
};
#endif
#endif /* MBEDTLS_CIPHER_HAVE_CCM_STAR_NO_TAG_AES_VIA_LEGACY_OR_USE_PSA */

const mbedtls_cipher_definition_t mbedtls_cipher_definitions[] =
{
    { MBEDTLS_CIPHER_AES_128_ECB,          &aes_128_ecb_info },
#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
    { MBEDTLS_CIPHER_AES_192_ECB,          &aes_192_ecb_info },
    { MBEDTLS_CIPHER_AES_256_ECB,          &aes_256_ecb_info },
#endif
#if defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA)
    { MBEDTLS_CIPHER_AES_128_GCM,          &aes_128_gcm_info },
#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
    { MBEDTLS_CIPHER_AES_192_GCM,          &aes_192_gcm_info },
    { MBEDTLS_CIPHER_AES_256_GCM,          &aes_256_gcm_info },
#endif
#endif
#if defined(MBEDTLS_CIPHER_HAVE_CCM_STAR_NO_TAG_AES_VIA_LEGACY_OR_USE_PSA)
    { MBEDTLS_CIPHER_AES_128_CCM_STAR_NO_TAG,          &aes_128_ccm_star_no_tag_info },
#if !defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
    { MBEDTLS_CIPHER_AES_192_CCM_STAR_NO_TAG,          &aes_192_ccm_star_no_tag_info },
    { MBEDTLS_CIPHER_AES_256_CCM_STAR_NO_TAG,          &aes_256_ccm_star_no_tag_info },
#endif
#endif

    { MBEDTLS_CIPHER_NONE, NULL }
};

#define NUM_CIPHERS (sizeof(mbedtls_cipher_definitions) /      \
                     sizeof(mbedtls_cipher_definitions[0]))
int mbedtls_cipher_supported[NUM_CIPHERS];

const mbedtls_cipher_base_t * const mbedtls_cipher_base_lookup_table[] = {
    [MBEDTLS_CIPHER_BASE_INDEX_AES] = &aes_info,
#if defined(MBEDTLS_CIPHER_HAVE_GCM_AES_VIA_LEGACY_OR_USE_PSA)
    [MBEDTLS_CIPHER_BASE_INDEX_GCM_AES] = &gcm_aes_info,
#endif
};

