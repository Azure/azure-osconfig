// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Public Key abstraction layer: wrapper functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pk_wrap.c.
 */

#include "common.h"

#include "platform_util.h"

#include "pk_wrap.h"
#include "pk_internal.h"
#include "error.h"

/* Even if RSA not activated, for the sake of RSA-alt */
#include "rsa.h"

#include "ecp.h"

#include "ecdsa.h"

#include "platform.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

int rsa_can_do(mbedtls_pk_type_t type)
{
    return type == MBEDTLS_PK_RSA ||
           type == MBEDTLS_PK_RSASSA_PSS;
}

size_t rsa_get_bitlen(mbedtls_pk_context *pk)
{
    const mbedtls_rsa_context *rsa = (const mbedtls_rsa_context *) pk->pk_ctx;
    return mbedtls_rsa_get_bitlen(rsa);
}

int rsa_verify_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                           const unsigned char *hash, size_t hash_len,
                           const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_rsa_context *rsa = (mbedtls_rsa_context *) pk->pk_ctx;
    size_t rsa_len = mbedtls_rsa_get_len(rsa);

#if SIZE_MAX > UINT_MAX
    if (md_alg == MBEDTLS_MD_NONE && UINT_MAX < hash_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }
#endif

    if (sig_len < rsa_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_VERIFY_FAILED);
    }

    if ((ret = mbedtls_rsa_pkcs1_verify(rsa, md_alg,
                                        (unsigned int) hash_len,
                                        hash, sig, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    /* The buffer contains a valid signature followed by extra data.
     * We have a special error code for that so that so that callers can
     * use mbedtls_pk_verify() to check "Does the buffer start with a
     * valid signature?" and not just "Does the buffer contain a valid
     * signature?". */
    if (sig_len > rsa_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_SIG_LEN_MISMATCH);
    }

    MINTLS_RETURN(0);
}

int rsa_sign_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                         const unsigned char *hash, size_t hash_len,
                         unsigned char *sig, size_t sig_size, size_t *sig_len,
                         int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_context *rsa = (mbedtls_rsa_context *) pk->pk_ctx;

#if SIZE_MAX > UINT_MAX
    if (md_alg == MBEDTLS_MD_NONE && UINT_MAX < hash_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }
#endif

    *sig_len = mbedtls_rsa_get_len(rsa);
    if (sig_size < *sig_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BUFFER_TOO_SMALL);
    }

    MINTLS_RETURN(mbedtls_rsa_pkcs1_sign(rsa, f_rng, p_rng,
                                  md_alg, (unsigned int) hash_len,
                                  hash, sig, diagnostics));
}

int rsa_decrypt_wrap(mbedtls_pk_context *pk,
                            const unsigned char *input, size_t ilen,
                            unsigned char *output, size_t *olen, size_t osize,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_context *rsa = (mbedtls_rsa_context *) pk->pk_ctx;

    if (ilen != mbedtls_rsa_get_len(rsa)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(mbedtls_rsa_pkcs1_decrypt(rsa, f_rng, p_rng,
                                     olen, input, output, osize, diagnostics));
}

int rsa_encrypt_wrap(mbedtls_pk_context *pk,
                            const unsigned char *input, size_t ilen,
                            unsigned char *output, size_t *olen, size_t osize,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_context *rsa = (mbedtls_rsa_context *) pk->pk_ctx;
    *olen = mbedtls_rsa_get_len(rsa);

    if (*olen > osize) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE);
    }

    MINTLS_RETURN(mbedtls_rsa_pkcs1_encrypt(rsa, f_rng, p_rng,
                                     ilen, input, output, diagnostics));
}

int rsa_check_pair_wrap(mbedtls_pk_context *pub, mbedtls_pk_context *prv,
                               int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                               void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    (void) f_rng;
    (void) p_rng;
    MINTLS_RETURN(mbedtls_rsa_check_pub_priv((const mbedtls_rsa_context *) pub->pk_ctx,
                                      (const mbedtls_rsa_context *) prv->pk_ctx, diagnostics));
}

void *rsa_alloc_wrap(void)
{
    void *ctx = mbedtls_calloc(1, sizeof(mbedtls_rsa_context));

    if (ctx != NULL) {
        mbedtls_rsa_init((mbedtls_rsa_context *) ctx);
    }

    return ctx;
}

void rsa_free_wrap(void *ctx)
{
    mbedtls_rsa_free((mbedtls_rsa_context *) ctx);
    mbedtls_free(ctx);
}

void rsa_debug(mbedtls_pk_context *pk, mbedtls_pk_debug_item *items)
{
    mbedtls_rsa_context *rsa = (mbedtls_rsa_context *) pk->pk_ctx;

    items->type = MBEDTLS_PK_DEBUG_MPI;
    items->name = "rsa.N";
    items->value = &(rsa->N);

    items++;

    items->type = MBEDTLS_PK_DEBUG_MPI;
    items->name = "rsa.E";
    items->value = &(rsa->E);
}

const mbedtls_pk_info_t mbedtls_rsa_info = {
    .type = MBEDTLS_PK_RSA,
    .name = "RSA",
    .get_bitlen = rsa_get_bitlen,
    .can_do = rsa_can_do,
    .verify_func = rsa_verify_wrap,
    .sign_func = rsa_sign_wrap,
    .decrypt_func = rsa_decrypt_wrap,
    .encrypt_func = rsa_encrypt_wrap,
    .check_pair_func = rsa_check_pair_wrap,
    .ctx_alloc_func = rsa_alloc_wrap,
    .ctx_free_func = rsa_free_wrap,
    .debug_func = rsa_debug,
};

/*
 * Generic EC key
 */
int eckey_can_do(mbedtls_pk_type_t type)
{
    return type == MBEDTLS_PK_ECKEY ||
           type == MBEDTLS_PK_ECKEY_DH ||
           type == MBEDTLS_PK_ECDSA;
}

size_t eckey_get_bitlen(mbedtls_pk_context *pk)
{
    mbedtls_ecp_keypair *ecp = (mbedtls_ecp_keypair *) pk->pk_ctx;
    return ecp->grp.pbits;
}

int ecdsa_verify_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                             const unsigned char *hash, size_t hash_len,
                             const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    ((void) md_alg);

    ret = mbedtls_ecdsa_read_signature((mbedtls_ecdsa_context *) pk->pk_ctx,
                                       hash, hash_len, sig, sig_len, diagnostics);

    if (ret == MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH) {
        MINTLS_RETURN_CAUSE(MBEDTLS_ERR_PK_SIG_LEN_MISMATCH);
    }

    MINTLS_RETURN(ret);
}

int ecdsa_sign_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                           const unsigned char *hash, size_t hash_len,
                           unsigned char *sig, size_t sig_size, size_t *sig_len,
                           int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecdsa_write_signature((mbedtls_ecdsa_context *) pk->pk_ctx,
                                         md_alg, hash, hash_len,
                                         sig, sig_size, sig_len,
                                         f_rng, p_rng, diagnostics));
}

int eckey_check_pair_wrap(mbedtls_pk_context *pub, mbedtls_pk_context *prv,
                                 int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                 void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_check_pub_priv((const mbedtls_ecp_keypair *) pub->pk_ctx,
                                      (const mbedtls_ecp_keypair *) prv->pk_ctx,
                                      f_rng, p_rng, diagnostics));
}

void *eckey_alloc_wrap(void)
{
    void *ctx = mbedtls_calloc(1, sizeof(mbedtls_ecp_keypair));

    if (ctx != NULL) {
        mbedtls_ecp_keypair_init(ctx);
    }

    return ctx;
}

void eckey_free_wrap(void *ctx)
{
    mbedtls_ecp_keypair_free((mbedtls_ecp_keypair *) ctx);
    mbedtls_free(ctx);
}

void eckey_debug(mbedtls_pk_context *pk, mbedtls_pk_debug_item *items)
{
    mbedtls_ecp_keypair *ecp = (mbedtls_ecp_keypair *) pk->pk_ctx;
    items->type = MBEDTLS_PK_DEBUG_ECP;
    items->name = "eckey.Q";
    items->value = &(ecp->Q);
}

const mbedtls_pk_info_t mbedtls_eckey_info = {
    .type = MBEDTLS_PK_ECKEY,
    .name = "EC",
    .get_bitlen = eckey_get_bitlen,
    .can_do = eckey_can_do,
    .verify_func = ecdsa_verify_wrap,   /* Compatible key structures */
    .sign_func = ecdsa_sign_wrap,   /* Compatible key structures */
    .decrypt_func = NULL,
    .encrypt_func = NULL,
    .check_pair_func = eckey_check_pair_wrap,
    .ctx_alloc_func = eckey_alloc_wrap,
    .ctx_free_func = eckey_free_wrap,
    .debug_func = eckey_debug,
};

/*
 * EC key restricted to ECDH
 */
int eckeydh_can_do(mbedtls_pk_type_t type)
{
    return type == MBEDTLS_PK_ECKEY ||
           type == MBEDTLS_PK_ECKEY_DH;
}

const mbedtls_pk_info_t mbedtls_eckeydh_info = {
    .type = MBEDTLS_PK_ECKEY_DH,
    .name = "EC_DH",
    .get_bitlen = eckey_get_bitlen,         /* Same underlying key structure */
    .can_do = eckeydh_can_do,
    .verify_func = NULL,
    .sign_func = NULL,
    .decrypt_func = NULL,
    .encrypt_func = NULL,
    .check_pair_func = eckey_check_pair_wrap,
    .ctx_alloc_func = eckey_alloc_wrap,   /* Same underlying key structure */
    .ctx_free_func = eckey_free_wrap,    /* Same underlying key structure */
    .debug_func = eckey_debug,            /* Same underlying key structure */
};

int ecdsa_can_do(mbedtls_pk_type_t type)
{
    return type == MBEDTLS_PK_ECDSA;
}

const mbedtls_pk_info_t mbedtls_ecdsa_info = {
    .type = MBEDTLS_PK_ECDSA,
    .name = "ECDSA",
    .get_bitlen = eckey_get_bitlen,     /* Compatible key structures */
    .can_do = ecdsa_can_do,
    .verify_func = ecdsa_verify_wrap,   /* Compatible key structures */
    .sign_func = ecdsa_sign_wrap,   /* Compatible key structures */
    .decrypt_func = NULL,
    .encrypt_func = NULL,
    .check_pair_func = eckey_check_pair_wrap,   /* Compatible key structures */
    .ctx_alloc_func = eckey_alloc_wrap,   /* Compatible key structures */
    .ctx_free_func = eckey_free_wrap,   /* Compatible key structures */
    .debug_func = eckey_debug,        /* Compatible key structures */
};

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/*
 * Support for alternative RSA-private implementations
 */

int rsa_alt_can_do(mbedtls_pk_type_t type)
{
    return type == MBEDTLS_PK_RSA;
}

size_t rsa_alt_get_bitlen(mbedtls_pk_context *pk)
{
    const mbedtls_rsa_alt_context *rsa_alt = pk->pk_ctx;

    return 8 * rsa_alt->key_len_func(rsa_alt->key);
}

int rsa_alt_sign_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                             const unsigned char *hash, size_t hash_len,
                             unsigned char *sig, size_t sig_size, size_t *sig_len,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_alt_context *rsa_alt = pk->pk_ctx;

#if SIZE_MAX > UINT_MAX
    if (UINT_MAX < hash_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }
#endif

    *sig_len = rsa_alt->key_len_func(rsa_alt->key);
    if (*sig_len > MBEDTLS_PK_SIGNATURE_MAX_SIZE) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }
    if (*sig_len > sig_size) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BUFFER_TOO_SMALL);
    }

    MINTLS_RETURN(rsa_alt->sign_func(rsa_alt->key, f_rng, p_rng,
                              md_alg, (unsigned int) hash_len, hash, sig, diagnostics));
}

int rsa_alt_decrypt_wrap(mbedtls_pk_context *pk,
                                const unsigned char *input, size_t ilen,
                                unsigned char *output, size_t *olen, size_t osize,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_alt_context *rsa_alt = pk->pk_ctx;

    ((void) f_rng);
    ((void) p_rng);

    if (ilen != rsa_alt->key_len_func(rsa_alt->key)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(rsa_alt->decrypt_func(rsa_alt->key,
                                 olen, input, output, osize, diagnostics));
}

int rsa_alt_check_pair(mbedtls_pk_context *pub, mbedtls_pk_context *prv,
                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                              void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char hash[32];
    size_t sig_len = 0;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (rsa_alt_get_bitlen(prv) != rsa_get_bitlen(pub)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    size_t sig_size = (rsa_get_bitlen(pub) + 7) / 8;
    unsigned char *sig = mbedtls_calloc(1, sig_size);
    if (sig == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_ALLOC_FAILED);
    }

    memset(hash, 0x2a, sizeof(hash));

    if ((ret = rsa_alt_sign_wrap(prv, MBEDTLS_MD_NONE,
                                 hash, sizeof(hash),
                                 sig, sig_size, &sig_len,
                                 f_rng, p_rng, diagnostics)) != 0) {
        goto cleanup;
    }

    if (rsa_verify_wrap(pub, MBEDTLS_MD_NONE,
                        hash, sizeof(hash), sig, sig_len, diagnostics) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

cleanup:
    mbedtls_zeroize_and_free(sig, sig_size);
    MINTLS_RETURN(ret);
}

void *rsa_alt_alloc_wrap(void)
{
    void *ctx = mbedtls_calloc(1, sizeof(mbedtls_rsa_alt_context));

    if (ctx != NULL) {
        memset(ctx, 0, sizeof(mbedtls_rsa_alt_context));
    }

    return ctx;
}

void rsa_alt_free_wrap(void *ctx)
{
    mbedtls_zeroize_and_free(ctx, sizeof(mbedtls_rsa_alt_context));
}

const mbedtls_pk_info_t mbedtls_rsa_alt_info = {
    .type = MBEDTLS_PK_RSA_ALT,
    .name = "RSA-alt",
    .get_bitlen = rsa_alt_get_bitlen,
    .can_do = rsa_alt_can_do,
    .verify_func = NULL,
    .sign_func = rsa_alt_sign_wrap,
    .decrypt_func = rsa_alt_decrypt_wrap,
    .encrypt_func = NULL,
    .check_pair_func = rsa_alt_check_pair,
    .ctx_alloc_func = rsa_alt_alloc_wrap,
    .ctx_free_func = rsa_alt_free_wrap,
    .debug_func = NULL,
};
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */
