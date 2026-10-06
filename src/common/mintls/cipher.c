// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file cipher.c
 *
 * \brief Generic cipher wrapper for Mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/cipher.c.
 */

#include "common.h"

#include "cipher.h"
#include "cipher_invasive.h"
#include "cipher_wrap.h"
#include "platform_util.h"
#include "error.h"
#include "constant_time.h"
#include "constant_time_internal.h"

#include <stdlib.h>
#include <string.h>

#include "gcm.h"

#include "platform.h"

static int supported_init = 0;

const mbedtls_cipher_base_t *mbedtls_cipher_get_base(
    const mbedtls_cipher_info_t *info)
{
    return mbedtls_cipher_base_lookup_table[info->base_idx];
}

const int *mbedtls_cipher_list(void)
{
    const mbedtls_cipher_definition_t *def;
    int *type;

    if (!supported_init) {
        def = mbedtls_cipher_definitions;
        type = mbedtls_cipher_supported;

        while (def->type != 0) {
            *type++ = (*def++).type;
        }

        *type = 0;

        supported_init = 1;
    }

    return mbedtls_cipher_supported;
}

const mbedtls_cipher_info_t *mbedtls_cipher_info_from_type(
    const mbedtls_cipher_type_t cipher_type)
{
    const mbedtls_cipher_definition_t *def;

    for (def = mbedtls_cipher_definitions; def->info != NULL; def++) {
        if (def->type == cipher_type) {
            return def->info;
        }
    }

    return NULL;
}

const mbedtls_cipher_info_t *mbedtls_cipher_info_from_string(
    const char *cipher_name)
{
    const mbedtls_cipher_definition_t *def;

    if (NULL == cipher_name) {
        return NULL;
    }

    for (def = mbedtls_cipher_definitions; def->info != NULL; def++) {
        if (!strcmp(def->info->name, cipher_name)) {
            return def->info;
        }
    }

    return NULL;
}

const mbedtls_cipher_info_t *mbedtls_cipher_info_from_values(
    const mbedtls_cipher_id_t cipher_id,
    int key_bitlen,
    const mbedtls_cipher_mode_t mode)
{
    const mbedtls_cipher_definition_t *def;

    for (def = mbedtls_cipher_definitions; def->info != NULL; def++) {
        if (mbedtls_cipher_get_base(def->info)->cipher == cipher_id &&
            mbedtls_cipher_info_get_key_bitlen(def->info) == (unsigned) key_bitlen &&
            def->info->mode == mode) {
            return def->info;
        }
    }

    return NULL;
}

void mbedtls_cipher_init(mbedtls_cipher_context_t *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_cipher_context_t));
}

void mbedtls_cipher_free(mbedtls_cipher_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->cipher_ctx) {
        mbedtls_cipher_get_base(ctx->cipher_info)->ctx_free_func(ctx->cipher_ctx);
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_cipher_context_t));
}

int mbedtls_cipher_setup(mbedtls_cipher_context_t *ctx,
                         const mbedtls_cipher_info_t *cipher_info, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    memset(ctx, 0, sizeof(mbedtls_cipher_context_t));

    if (mbedtls_cipher_get_base(cipher_info)->ctx_alloc_func != NULL) {
        ctx->cipher_ctx = mbedtls_cipher_get_base(cipher_info)->ctx_alloc_func();
        if (ctx->cipher_ctx == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_ALLOC_FAILED);
        }
    }

    ctx->cipher_info = cipher_info;

    MINTLS_RETURN(0);
}

int mbedtls_cipher_setkey(mbedtls_cipher_context_t *ctx,
                          const unsigned char *key,
                          int key_bitlen,
                          const mbedtls_operation_t operation, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (operation != MBEDTLS_ENCRYPT && operation != MBEDTLS_DECRYPT) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }
    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }
#if defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    if (MBEDTLS_MODE_ECB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) &&
        MBEDTLS_DECRYPT == operation) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
    }
#endif

    if ((ctx->cipher_info->flags & MBEDTLS_CIPHER_VARIABLE_KEY_LEN) == 0 &&
        (int) mbedtls_cipher_info_get_key_bitlen(ctx->cipher_info) != key_bitlen) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    ctx->key_bitlen = key_bitlen;
    ctx->operation = operation;

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
    /*
     * For OFB, CFB and CTR mode always use the encryption key schedule
     */
    if (MBEDTLS_ENCRYPT == operation ||
        MBEDTLS_MODE_CFB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_OFB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_CTR == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        MINTLS_RETURN(mbedtls_cipher_get_base(ctx->cipher_info)->setkey_enc_func(ctx->cipher_ctx, key,
                                                                          ctx->key_bitlen, diagnostics));
    }

    if (MBEDTLS_DECRYPT == operation) {
        MINTLS_RETURN(mbedtls_cipher_get_base(ctx->cipher_info)->setkey_dec_func(ctx->cipher_ctx, key,
                                                                          ctx->key_bitlen, diagnostics));
    }
#else
    if (operation == MBEDTLS_ENCRYPT || operation == MBEDTLS_DECRYPT) {
        MINTLS_RETURN(mbedtls_cipher_get_base(ctx->cipher_info)->setkey_enc_func(ctx->cipher_ctx, key,
                                                                          ctx->key_bitlen, diagnostics));
    }
#endif

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
}

int mbedtls_cipher_set_iv(mbedtls_cipher_context_t *ctx,
                          const unsigned char *iv,
                          size_t iv_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t actual_iv_size;

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    /* avoid buffer overflow in ctx->iv */
    if (iv_len > MBEDTLS_MAX_IV_LENGTH) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
    }

    if ((ctx->cipher_info->flags & MBEDTLS_CIPHER_VARIABLE_IV_LEN) != 0) {
        actual_iv_size = iv_len;
    } else {
        actual_iv_size = mbedtls_cipher_info_get_iv_size(ctx->cipher_info);

        /* avoid reading past the end of input buffer */
        if (actual_iv_size > iv_len) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
        }
    }

    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        MINTLS_RETURN(mbedtls_gcm_starts((mbedtls_gcm_context *) ctx->cipher_ctx,
                                  ctx->operation,
                                  iv, iv_len, diagnostics));
    }

    if (actual_iv_size != 0) {
        memcpy(ctx->iv, iv, actual_iv_size);
        ctx->iv_size = actual_iv_size;
    }

    MINTLS_RETURN(0);
}

int mbedtls_cipher_reset(mbedtls_cipher_context_t *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    ctx->unprocessed_len = 0;

    MINTLS_RETURN(0);
}

int mbedtls_cipher_update_ad(mbedtls_cipher_context_t *ctx,
                             const unsigned char *ad, size_t ad_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        MINTLS_RETURN(mbedtls_gcm_update_ad((mbedtls_gcm_context *) ctx->cipher_ctx,
                                     ad, ad_len, diagnostics));
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}

int mbedtls_cipher_update(mbedtls_cipher_context_t *ctx, const unsigned char *input,
                          size_t ilen, unsigned char *output, size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t block_size;

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    *olen = 0;
    block_size = mbedtls_cipher_get_block_size(ctx);
    if (0 == block_size) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_INVALID_CONTEXT);
    }

    if (((mbedtls_cipher_mode_t) ctx->cipher_info->mode) == MBEDTLS_MODE_ECB) {
        if (ilen != block_size) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FULL_BLOCK_EXPECTED);
        }

        *olen = ilen;

        if (0 != (ret = mbedtls_cipher_get_base(ctx->cipher_info)->ecb_func(ctx->cipher_ctx,
                                                                            ctx->operation, input,
                                                                            output, diagnostics))) {
            MINTLS_RETURN(ret);
        }

        MINTLS_RETURN(0);
    }

    if (((mbedtls_cipher_mode_t) ctx->cipher_info->mode) == MBEDTLS_MODE_GCM) {
        MINTLS_RETURN(mbedtls_gcm_update((mbedtls_gcm_context *) ctx->cipher_ctx,
                                  input, ilen,
                                  output, ilen, olen, diagnostics));
    }

    if (input == output &&
        (ctx->unprocessed_len != 0 || ilen % block_size)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}

#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)

/*
 * No padding: don't pad :)
 *
 * There is no add_padding function (check for NULL in mbedtls_cipher_finish)
 * but a trivial get_padding function
 */
int get_no_padding(unsigned char *input, size_t input_len,
                          size_t *data_len, size_t *invalid_padding, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (NULL == input || NULL == data_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    *data_len = input_len;
    *invalid_padding = 0;
    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_CIPHER_MODE_WITH_PADDING */

int mbedtls_cipher_finish_padded(mbedtls_cipher_context_t *ctx,
                                 unsigned char *output, size_t *olen,
                                 size_t *invalid_padding, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    *olen = 0;
    *invalid_padding = 0;

#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)
    /* CBC mode requires padding so we make sure a call to
     * mbedtls_cipher_set_padding_mode has been done successfully. */
    if (MBEDTLS_MODE_CBC == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        if (ctx->get_padding == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
        }
    }
#endif

    if (MBEDTLS_MODE_CFB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_OFB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_CTR == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_CCM_STAR_NO_TAG == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_XTS == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode) ||
        MBEDTLS_MODE_STREAM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        MINTLS_RETURN(0);
    }

    if ((MBEDTLS_CIPHER_CHACHA20          == ((mbedtls_cipher_type_t) ctx->cipher_info->type)) ||
        (MBEDTLS_CIPHER_CHACHA20_POLY1305 == ((mbedtls_cipher_type_t) ctx->cipher_info->type))) {
        MINTLS_RETURN(0);
    }

    if (MBEDTLS_MODE_ECB == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        if (ctx->unprocessed_len != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FULL_BLOCK_EXPECTED);
        }

        MINTLS_RETURN(0);
    }

    ((void) output);

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}

int mbedtls_cipher_finish(mbedtls_cipher_context_t *ctx,
                          unsigned char *output, size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t invalid_padding = 0;
    int ret = mbedtls_cipher_finish_padded(ctx, output, olen,
                                           &invalid_padding, diagnostics);
    if (ret == 0) {
        ret = mbedtls_ct_error_if_else_0(invalid_padding,
                                         MBEDTLS_ERR_CIPHER_INVALID_PADDING);
    }
    MINTLS_RETURN(ret);
}

#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)
int mbedtls_cipher_set_padding_mode(mbedtls_cipher_context_t *ctx,
                                    mbedtls_cipher_padding_t mode, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (NULL == ctx->cipher_info ||
        MBEDTLS_MODE_CBC != ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    switch (mode) {
        case MBEDTLS_PADDING_NONE:
            ctx->add_padding = NULL;
            ctx->get_padding = get_no_padding;
            break;

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
    }

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_CIPHER_MODE_WITH_PADDING */

int mbedtls_cipher_write_tag(mbedtls_cipher_context_t *ctx,
                             unsigned char *tag, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    if (MBEDTLS_ENCRYPT != ctx->operation) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        size_t output_length;
        /* The code here doesn't yet support alternative implementations
         * that can delay up to a block of output. */
        MINTLS_RETURN(mbedtls_gcm_finish((mbedtls_gcm_context *) ctx->cipher_ctx,
                                  NULL, 0, &output_length,
                                  tag, tag_len, diagnostics));
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}

int mbedtls_cipher_check_tag(mbedtls_cipher_context_t *ctx,
                             const unsigned char *tag, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char check_tag[16];
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ctx->cipher_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    if (MBEDTLS_DECRYPT != ctx->operation) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    /* Status to return on a non-authenticated algorithm. */
    ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);

    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        size_t output_length;
        /* The code here doesn't yet support alternative implementations
         * that can delay up to a block of output. */

        if (tag_len > sizeof(check_tag)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
        }

        if (0 != (ret = mbedtls_gcm_finish(
                      (mbedtls_gcm_context *) ctx->cipher_ctx,
                      NULL, 0, &output_length,
                      check_tag, tag_len, diagnostics))) {
            MINTLS_RETURN(ret);
        }

        /* Check the tag in "constant-time" */
        if (mbedtls_ct_memcmp(tag, check_tag, tag_len) != 0) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_CIPHER_AUTH_FAILED);
            goto exit;
        }
    }

exit:
    mbedtls_platform_zeroize(check_tag, tag_len);
    MINTLS_RETURN(ret);
}

/*
 * Packet-oriented wrapper for non-AEAD modes
 */
int mbedtls_cipher_crypt(mbedtls_cipher_context_t *ctx,
                         const unsigned char *iv, size_t iv_len,
                         const unsigned char *input, size_t ilen,
                         unsigned char *output, size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t finish_olen;

    if ((ret = mbedtls_cipher_set_iv(ctx, iv, iv_len, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_cipher_reset(ctx, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_cipher_update(ctx, input, ilen,
                                     output, olen, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    size_t invalid_padding = 0;
    if ((ret = mbedtls_cipher_finish_padded(ctx, output + *olen,
                                            &finish_olen,
                                            &invalid_padding, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    *olen += finish_olen;

    ret = mbedtls_ct_error_if_else_0(invalid_padding,
                                     MBEDTLS_ERR_CIPHER_INVALID_PADDING);
    MINTLS_RETURN(ret);
}

#if defined(MBEDTLS_CIPHER_MODE_AEAD)
/*
 * Packet-oriented encryption for AEAD modes: internal function used by
 * mbedtls_cipher_auth_encrypt_ext().
 */
int mbedtls_cipher_aead_encrypt(mbedtls_cipher_context_t *ctx,
                                       const unsigned char *iv, size_t iv_len,
                                       const unsigned char *ad, size_t ad_len,
                                       const unsigned char *input, size_t ilen,
                                       unsigned char *output, size_t *olen,
                                       unsigned char *tag, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        *olen = ilen;
        MINTLS_RETURN(mbedtls_gcm_crypt_and_tag(ctx->cipher_ctx, MBEDTLS_GCM_ENCRYPT,
                                         ilen, iv, iv_len, ad, ad_len,
                                         input, output, tag_len, tag, diagnostics));
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}

/*
 * Packet-oriented encryption for AEAD modes: internal function used by
 * mbedtls_cipher_auth_encrypt_ext().
 */
int mbedtls_cipher_aead_decrypt(mbedtls_cipher_context_t *ctx,
                                       const unsigned char *iv, size_t iv_len,
                                       const unsigned char *ad, size_t ad_len,
                                       const unsigned char *input, size_t ilen,
                                       unsigned char *output, size_t *olen,
                                       const unsigned char *tag, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


    if (MBEDTLS_MODE_GCM == ((mbedtls_cipher_mode_t) ctx->cipher_info->mode)) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        *olen = ilen;
        ret = mbedtls_gcm_auth_decrypt(ctx->cipher_ctx, ilen,
                                       iv, iv_len, ad, ad_len,
                                       tag, tag_len, input, output, diagnostics);

        if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_CIPHER_AUTH_FAILED);
        }

        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
}
#endif /* MBEDTLS_CIPHER_MODE_AEAD */

#if defined(MBEDTLS_CIPHER_MODE_AEAD) || defined(MBEDTLS_NIST_KW_C)
/*
 * Packet-oriented encryption for AEAD/NIST_KW: public function.
 */
int mbedtls_cipher_auth_encrypt_ext(mbedtls_cipher_context_t *ctx,
                                    const unsigned char *iv, size_t iv_len,
                                    const unsigned char *ad, size_t ad_len,
                                    const unsigned char *input, size_t ilen,
                                    unsigned char *output, size_t output_len,
                                    size_t *olen, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


#if defined(MBEDTLS_CIPHER_MODE_AEAD)
    /* AEAD case: check length before passing on to shared function */
    if (output_len < ilen + tag_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    int ret = mbedtls_cipher_aead_encrypt(ctx, iv, iv_len, ad, ad_len,
                                          input, ilen, output, olen,
                                          output + ilen, tag_len, diagnostics);
    *olen += tag_len;
    MINTLS_RETURN(ret);
#else
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
#endif /* MBEDTLS_CIPHER_MODE_AEAD */
}

/*
 * Packet-oriented decryption for AEAD/NIST_KW: public function.
 */
int mbedtls_cipher_auth_decrypt_ext(mbedtls_cipher_context_t *ctx,
                                    const unsigned char *iv, size_t iv_len,
                                    const unsigned char *ad, size_t ad_len,
                                    const unsigned char *input, size_t ilen,
                                    unsigned char *output, size_t output_len,
                                    size_t *olen, size_t tag_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


#if defined(MBEDTLS_CIPHER_MODE_AEAD)
    /* AEAD case: check length before passing on to shared function */
    if (ilen < tag_len || output_len < ilen - tag_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(mbedtls_cipher_aead_decrypt(ctx, iv, iv_len, ad, ad_len,
                                       input, ilen - tag_len, output, olen,
                                       input + ilen - tag_len, tag_len, diagnostics));
#else
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE);
#endif /* MBEDTLS_CIPHER_MODE_AEAD */
}
#endif /* MBEDTLS_CIPHER_MODE_AEAD || MBEDTLS_NIST_KW_C */

