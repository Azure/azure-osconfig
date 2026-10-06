// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Public Key abstraction layer
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pk.c.
 */

#include "common.h"

#include "pk.h"
#include "pk_wrap.h"
#include "pk_internal.h"

#include "platform_util.h"
#include "error.h"

#include "rsa.h"
#include "rsa_internal.h"
#include "ecp.h"
#include "ecdsa.h"

#include <limits.h>
#include <stdint.h>

#if !defined(PK_EXPORT_KEYS_ON_THE_STACK)
#include "platform.h" // for calloc/free
#endif

/*
 * Initialise a mbedtls_pk_context
 */
void mbedtls_pk_init(mbedtls_pk_context *ctx)
{
    ctx->pk_info = NULL;
    ctx->pk_ctx = NULL;
}

/*
 * Free (the components of) a mbedtls_pk_context
 */
void mbedtls_pk_free(mbedtls_pk_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if ((ctx->pk_info != NULL) && (ctx->pk_info->ctx_free_func != NULL)) {
        ctx->pk_info->ctx_free_func(ctx->pk_ctx);
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_pk_context));
}

/*
 * Get pk_info structure from type
 */
const mbedtls_pk_info_t *mbedtls_pk_info_from_type(mbedtls_pk_type_t pk_type)
{
    switch (pk_type) {
        case MBEDTLS_PK_RSA:
            return &mbedtls_rsa_info;
        case MBEDTLS_PK_ECKEY:
            return &mbedtls_eckey_info;
        case MBEDTLS_PK_ECKEY_DH:
            return &mbedtls_eckeydh_info;
        case MBEDTLS_PK_ECDSA:
            return &mbedtls_ecdsa_info;
        /* MBEDTLS_PK_RSA_ALT omitted on purpose */
        default:
            return NULL;
    }
}

/*
 * Initialise context
 */
int mbedtls_pk_setup(mbedtls_pk_context *ctx, const mbedtls_pk_info_t *info, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (info == NULL || ctx->pk_info != NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if ((info->ctx_alloc_func != NULL) &&
        ((ctx->pk_ctx = info->ctx_alloc_func()) == NULL)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_ALLOC_FAILED);
    }

    ctx->pk_info = info;

    MINTLS_RETURN(0);
}

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/*
 * Initialize an RSA-alt context
 */
int mbedtls_pk_setup_rsa_alt(mbedtls_pk_context *ctx, void *key,
                             mbedtls_pk_rsa_alt_decrypt_func decrypt_func,
                             mbedtls_pk_rsa_alt_sign_func sign_func,
                             mbedtls_pk_rsa_alt_key_len_func key_len_func, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_rsa_alt_context *rsa_alt;
    const mbedtls_pk_info_t *info = &mbedtls_rsa_alt_info;

    if (ctx->pk_info != NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if ((ctx->pk_ctx = info->ctx_alloc_func()) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_ALLOC_FAILED);
    }

    ctx->pk_info = info;

    rsa_alt = (mbedtls_rsa_alt_context *) ctx->pk_ctx;

    rsa_alt->key = key;
    rsa_alt->decrypt_func = decrypt_func;
    rsa_alt->sign_func = sign_func;
    rsa_alt->key_len_func = key_len_func;

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/*
 * Tell if a PK can do the operations of the given type
 */
int mbedtls_pk_can_do(const mbedtls_pk_context *ctx, mbedtls_pk_type_t type)
{
    /* A context with null pk_info is not set up yet and can't do anything.
     * For backward compatibility, also accept NULL instead of a context
     * pointer. */
    if (ctx == NULL || ctx->pk_info == NULL) {
        return 0;
    }

    return ctx->pk_info->can_do(type);
}

/*
 * Helper for mbedtls_pk_sign and mbedtls_pk_verify
 */
int pk_hashlen_helper(mbedtls_md_type_t md_alg, size_t *hash_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (*hash_len != 0) {
        MINTLS_RETURN(0);
    }

    *hash_len = mbedtls_md_get_size_from_type(md_alg);

    if (*hash_len == 0) {
        MINTLS_RETURN_ERROR(-1);
    }

    MINTLS_RETURN(0);
}

/*
 * Verify a signature (restartable)
 */
int mbedtls_pk_verify_restartable(mbedtls_pk_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hash_len,
                                  const unsigned char *sig, size_t sig_len,
                                  mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info == NULL ||
        pk_hashlen_helper(md_alg, &hash_len, diagnostics) != 0) {
        MINTLS_RETURN_CAUSE(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    (void) rs_ctx;

    if (ctx->pk_info->verify_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    MINTLS_RETURN(ctx->pk_info->verify_func(ctx, md_alg, hash, hash_len,
                                     sig, sig_len, diagnostics));
}

/*
 * Verify a signature
 */
int mbedtls_pk_verify(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                      const unsigned char *hash, size_t hash_len,
                      const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_pk_verify_restartable(ctx, md_alg, hash, hash_len,
                                         sig, sig_len, NULL, diagnostics));
}

/*
 * Verify a signature with options
 */
int mbedtls_pk_verify_ext(mbedtls_pk_type_t type, const void *options,
                          mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                          const unsigned char *hash, size_t hash_len,
                          const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (!mbedtls_pk_can_do(ctx, type)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    if (type != MBEDTLS_PK_RSASSA_PSS) {
        /* General case: no options */
        if (options != NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
        }

        MINTLS_RETURN(mbedtls_pk_verify(ctx, md_alg, hash, hash_len, sig, sig_len, diagnostics));
    }

    /* Ensure the PK context is of the right type otherwise mbedtls_pk_rsa()
     * below would return a NULL pointer. */
    if (mbedtls_pk_get_type(ctx) != MBEDTLS_PK_RSA) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE);
    }

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_pk_rsassa_pss_options *pss_opts;

#if SIZE_MAX > UINT_MAX
    if (md_alg == MBEDTLS_MD_NONE && UINT_MAX < hash_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }
#endif

    if (options == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    pss_opts = (const mbedtls_pk_rsassa_pss_options *) options;

    {
        if (sig_len < mbedtls_pk_get_len(ctx)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_VERIFY_FAILED);
        }

        ret = mbedtls_rsa_rsassa_pss_verify_ext(mbedtls_pk_rsa(*ctx),
                                                md_alg, (unsigned int) hash_len, hash,
                                                pss_opts->mgf1_hash_id,
                                                pss_opts->expected_salt_len,
                                                sig, diagnostics);
        if (ret != 0) {
            MINTLS_RETURN(ret);
        }

        if (sig_len > mbedtls_pk_get_len(ctx)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_SIG_LEN_MISMATCH);
        }

        MINTLS_RETURN(0);
    }
}

/*
 * Make a signature (restartable)
 */
int mbedtls_pk_sign_restartable(mbedtls_pk_context *ctx,
                                mbedtls_md_type_t md_alg,
                                const unsigned char *hash, size_t hash_len,
                                unsigned char *sig, size_t sig_size, size_t *sig_len,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng,
                                mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info == NULL || pk_hashlen_helper(md_alg, &hash_len, diagnostics) != 0) {
        MINTLS_RETURN_CAUSE(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    (void) rs_ctx;

    if (ctx->pk_info->sign_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    MINTLS_RETURN(ctx->pk_info->sign_func(ctx, md_alg,
                                   hash, hash_len,
                                   sig, sig_size, sig_len,
                                   f_rng, p_rng, diagnostics));
}

/*
 * Make a signature
 */
int mbedtls_pk_sign(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                    const unsigned char *hash, size_t hash_len,
                    unsigned char *sig, size_t sig_size, size_t *sig_len,
                    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_pk_sign_restartable(ctx, md_alg, hash, hash_len,
                                       sig, sig_size, sig_len,
                                       f_rng, p_rng, NULL, diagnostics));
}

/*
 * Make a signature given a signature type.
 */
int mbedtls_pk_sign_ext(mbedtls_pk_type_t pk_type,
                        mbedtls_pk_context *ctx,
                        mbedtls_md_type_t md_alg,
                        const unsigned char *hash, size_t hash_len,
                        unsigned char *sig, size_t sig_size, size_t *sig_len,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (!mbedtls_pk_can_do(ctx, pk_type)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    if (pk_type != MBEDTLS_PK_RSASSA_PSS) {
        MINTLS_RETURN(mbedtls_pk_sign(ctx, md_alg, hash, hash_len,
                               sig, sig_size, sig_len, f_rng, p_rng, diagnostics));
    }

    if (sig_size < mbedtls_pk_get_len(ctx)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BUFFER_TOO_SMALL);
    }

    if (pk_hashlen_helper(md_alg, &hash_len, diagnostics) != 0) {
        MINTLS_RETURN_CAUSE(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    mbedtls_rsa_context *const rsa_ctx = mbedtls_pk_rsa(*ctx);

    const int ret = mbedtls_rsa_rsassa_pss_sign_no_mode_check(rsa_ctx, f_rng, p_rng, md_alg,
                                                              (unsigned int) hash_len, hash, sig, diagnostics);
    if (ret == 0) {
        *sig_len = rsa_ctx->len;
    }
    MINTLS_RETURN(ret);

}

/*
 * Decrypt message
 */
int mbedtls_pk_decrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info->decrypt_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    MINTLS_RETURN(ctx->pk_info->decrypt_func(ctx, input, ilen,
                                      output, olen, osize, f_rng, p_rng, diagnostics));
}

/*
 * Encrypt message
 */
int mbedtls_pk_encrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info->encrypt_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    MINTLS_RETURN(ctx->pk_info->encrypt_func(ctx, input, ilen,
                                      output, olen, osize, f_rng, p_rng, diagnostics));
}

/*
 * Check public-private key pair
 */
int mbedtls_pk_check_pair(const mbedtls_pk_context *pub,
                          const mbedtls_pk_context *prv,
                          int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                          void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (pub->pk_info == NULL ||
        prv->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (prv->pk_info->check_pair_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE);
    }

    if (prv->pk_info->type == MBEDTLS_PK_RSA_ALT) {
        if (pub->pk_info->type != MBEDTLS_PK_RSA) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
        }
    } else {
        if ((prv->pk_info->type != MBEDTLS_PK_OPAQUE) &&
            (pub->pk_info != prv->pk_info)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
        }
    }

    MINTLS_RETURN(prv->pk_info->check_pair_func((mbedtls_pk_context *) pub,
                                         (mbedtls_pk_context *) prv,
                                         f_rng, p_rng, diagnostics));
}

/*
 * Get key size in bits
 */
size_t mbedtls_pk_get_bitlen(const mbedtls_pk_context *ctx)
{
    /* For backward compatibility, accept NULL or a context that
     * isn't set up yet, and return a fake value that should be safe. */
    if (ctx == NULL || ctx->pk_info == NULL) {
        return 0;
    }

    return ctx->pk_info->get_bitlen((mbedtls_pk_context *) ctx);
}

/*
 * Export debug information
 */
int mbedtls_pk_debug(const mbedtls_pk_context *ctx, mbedtls_pk_debug_item *items, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->pk_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_BAD_INPUT_DATA);
    }

    if (ctx->pk_info->debug_func == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_TYPE_MISMATCH);
    }

    ctx->pk_info->debug_func((mbedtls_pk_context *) ctx, items);
    MINTLS_RETURN(0);
}

/*
 * Access the PK type name
 */
const char *mbedtls_pk_get_name(const mbedtls_pk_context *ctx)
{
    if (ctx == NULL || ctx->pk_info == NULL) {
        return "invalid PK";
    }

    return ctx->pk_info->name;
}

/*
 * Access the PK type
 */
mbedtls_pk_type_t mbedtls_pk_get_type(const mbedtls_pk_context *ctx)
{
    if (ctx == NULL || ctx->pk_info == NULL) {
        return MBEDTLS_PK_NONE;
    }

    return ctx->pk_info->type;
}
