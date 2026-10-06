// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file ssl_ciphersuites.c
 *
 * \brief SSL ciphersuites for Mbed TLS
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_ciphersuites.c.
 */

#include "common.h"

#include "platform.h"

#include "ssl_ciphersuites.h"
#include "ssl.h"
#include "ssl_misc.h"

#include <string.h>

/* Preserve the private profile's advertised suite order. */
static const int ciphersuite_preference[] =
{
    MBEDTLS_SSL_CIPHERSUITES,
    0
};

static const mbedtls_ssl_ciphersuite_t ciphersuite_definitions[] =
{

    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256",
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,
      0,
      MBEDTLS_SSL_VERSION_TLS1_2, MBEDTLS_SSL_VERSION_TLS1_2 },
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384, "TLS-ECDHE-ECDSA-WITH-AES-256-GCM-SHA384",
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,
      0,
      MBEDTLS_SSL_VERSION_TLS1_2, MBEDTLS_SSL_VERSION_TLS1_2 },

    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256",
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,
      0,
      MBEDTLS_SSL_VERSION_TLS1_2, MBEDTLS_SSL_VERSION_TLS1_2 },
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, "TLS-ECDHE-RSA-WITH-AES-256-GCM-SHA384",
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,
      0,
      MBEDTLS_SSL_VERSION_TLS1_2, MBEDTLS_SSL_VERSION_TLS1_2 },

    { 0, "",
      MBEDTLS_CIPHER_NONE, MBEDTLS_MD_NONE, MBEDTLS_KEY_EXCHANGE_NONE,
      0, 0, 0 }
};

const int *mbedtls_ssl_list_ciphersuites(void)
{
    return ciphersuite_preference;
}

const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_string(
    const char *ciphersuite_name)
{
    const mbedtls_ssl_ciphersuite_t *cur = ciphersuite_definitions;

    if (NULL == ciphersuite_name) {
        return NULL;
    }

    while (cur->id != 0) {
        if (0 == strcmp(cur->name, ciphersuite_name)) {
            return cur;
        }

        cur++;
    }

    return NULL;
}

const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_id(int ciphersuite)
{
    const mbedtls_ssl_ciphersuite_t *cur = ciphersuite_definitions;

    while (cur->id != 0) {
        if (cur->id == ciphersuite) {
            return cur;
        }

        cur++;
    }

    return NULL;
}

const char *mbedtls_ssl_get_ciphersuite_name(const int ciphersuite_id)
{
    const mbedtls_ssl_ciphersuite_t *cur;

    cur = mbedtls_ssl_ciphersuite_from_id(ciphersuite_id);

    if (cur == NULL) {
        return "unknown";
    }

    return cur->name;
}

int mbedtls_ssl_get_ciphersuite_id(const char *ciphersuite_name)
{
    const mbedtls_ssl_ciphersuite_t *cur;

    cur = mbedtls_ssl_ciphersuite_from_string(ciphersuite_name);

    if (cur == NULL) {
        return 0;
    }

    return cur->id;
}

size_t mbedtls_ssl_ciphersuite_get_cipher_key_bitlen(const mbedtls_ssl_ciphersuite_t *info)
{
    const mbedtls_cipher_info_t * const cipher_info =
        mbedtls_cipher_info_from_type((mbedtls_cipher_type_t) info->cipher);

    return mbedtls_cipher_info_get_key_bitlen(cipher_info);
}

mbedtls_pk_type_t mbedtls_ssl_get_ciphersuite_sig_pk_alg(const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->key_exchange) {
        case MBEDTLS_KEY_EXCHANGE_RSA:
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_RSA_PSK:
            return MBEDTLS_PK_RSA;

        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
            return MBEDTLS_PK_ECDSA;

        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:
            return MBEDTLS_PK_ECKEY;

        default:
            return MBEDTLS_PK_NONE;
    }
}

mbedtls_pk_type_t mbedtls_ssl_get_ciphersuite_sig_alg(const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->key_exchange) {
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
            return MBEDTLS_PK_RSA;

        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
            return MBEDTLS_PK_ECDSA;

        default:
            return MBEDTLS_PK_NONE;
    }
}

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
int mbedtls_ssl_ciphersuite_uses_ec(const mbedtls_ssl_ciphersuite_t *info)
{
    switch (info->key_exchange) {
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
        case MBEDTLS_KEY_EXCHANGE_ECDHE_PSK:
        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:
        case MBEDTLS_KEY_EXCHANGE_ECJPAKE:
            return 1;

        default:
            return 0;
    }
}
#endif /* MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED ||
        * MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED ||
        * MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED*/

