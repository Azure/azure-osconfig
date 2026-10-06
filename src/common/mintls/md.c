// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file md.c
 *
 * \brief Generic message digest wrapper for Mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/md.c.
 */

#include "common.h"

/*
 * Availability of functions in this module is controlled by two
 * feature macros:
 * - MBEDTLS_MD_C enables the whole module;
 * - MBEDTLS_MD_LIGHT enables only functions for hashing and accessing
 * most hash metadata (everything except string names); is it
 * automatically set whenever MBEDTLS_MD_C is defined.
 *
 * In this file, functions from MD_LIGHT are at the top, MD_C at the end.
 *
 * In the future we may want to change the contract of some functions
 * (behaviour with NULL arguments) depending on whether MD_C is defined or
 * only MD_LIGHT. Also, the exact scope of MD_LIGHT might vary.
 *
 * For these reasons, we're keeping MD_LIGHT internal for now.
 */

#include "md.h"
#include "md_wrap.h"
#include "platform_util.h"
#include "error.h"

#include "sha1.h"
#include "sha256.h"
#include "sha512.h"

#include "platform.h"

#include <string.h>

/* See comment above MBEDTLS_MD_MAX_SIZE in md.h */
#if defined(MBEDTLS_PSA_CRYPTO_C) && MBEDTLS_MD_MAX_SIZE < PSA_HASH_MAX_SIZE
#error "Internal error: MBEDTLS_MD_MAX_SIZE < PSA_HASH_MAX_SIZE"
#endif

#define MD_INFO(type, out_size, block_size) type, out_size, block_size,

static const mbedtls_md_info_t mbedtls_sha1_info = {
    MD_INFO(MBEDTLS_MD_SHA1, 20, 64)
};

static const mbedtls_md_info_t mbedtls_sha256_info = {
    MD_INFO(MBEDTLS_MD_SHA256, 32, 64)
};

static const mbedtls_md_info_t mbedtls_sha384_info = {
    MD_INFO(MBEDTLS_MD_SHA384, 48, 128)
};

static const mbedtls_md_info_t mbedtls_sha512_info = {
    MD_INFO(MBEDTLS_MD_SHA512, 64, 128)
};

const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t md_type)
{
    switch (md_type) {
        case MBEDTLS_MD_SHA1:
            return &mbedtls_sha1_info;
        case MBEDTLS_MD_SHA256:
            return &mbedtls_sha256_info;
        case MBEDTLS_MD_SHA384:
            return &mbedtls_sha384_info;
        case MBEDTLS_MD_SHA512:
            return &mbedtls_sha512_info;
        default:
            return NULL;
    }
}

void mbedtls_md_init(mbedtls_md_context_t *ctx)
{
    /* Note: this sets engine (if present) to MBEDTLS_MD_ENGINE_LEGACY */
    memset(ctx, 0, sizeof(mbedtls_md_context_t));
}

void mbedtls_md_free(mbedtls_md_context_t *ctx)
{
    if (ctx == NULL || ctx->md_info == NULL) {
        return;
    }

    if (ctx->md_ctx != NULL) {
        switch (ctx->md_info->type) {
            case MBEDTLS_MD_SHA1:
                mbedtls_sha1_free(ctx->md_ctx);
                break;
            case MBEDTLS_MD_SHA256:
                mbedtls_sha256_free(ctx->md_ctx);
                break;
            case MBEDTLS_MD_SHA384:
                mbedtls_sha512_free(ctx->md_ctx);
                break;
            case MBEDTLS_MD_SHA512:
                mbedtls_sha512_free(ctx->md_ctx);
                break;
            default:
                /* Shouldn't happen */
                break;
        }
        mbedtls_free(ctx->md_ctx);
    }

    if (ctx->hmac_ctx != NULL) {
        mbedtls_zeroize_and_free(ctx->hmac_ctx,
                                 2 * ctx->md_info->block_size);
    }

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_md_context_t));
}

int mbedtls_md_clone(mbedtls_md_context_t *dst,
                     const mbedtls_md_context_t *src, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (dst == NULL || dst->md_info == NULL ||
        src == NULL || src->md_info == NULL ||
        dst->md_info != src->md_info) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    switch (src->md_info->type) {
        case MBEDTLS_MD_SHA1:
            mbedtls_sha1_clone(dst->md_ctx, src->md_ctx);
            break;
        case MBEDTLS_MD_SHA256:
            mbedtls_sha256_clone(dst->md_ctx, src->md_ctx);
            break;
        case MBEDTLS_MD_SHA384:
            mbedtls_sha512_clone(dst->md_ctx, src->md_ctx);
            break;
        case MBEDTLS_MD_SHA512:
            mbedtls_sha512_clone(dst->md_ctx, src->md_ctx);
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(0);
}

#define ALLOC(type)                                                   \
    do {                                                                \
        ctx->md_ctx = mbedtls_calloc(1, sizeof(mbedtls_##type##_context)); \
        if (ctx->md_ctx == NULL)                                       \
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_ALLOC_FAILED);                      \
        mbedtls_##type##_init(ctx->md_ctx);                           \
    }                                                                   \
    while (0)

int mbedtls_md_setup(mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }
    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    ctx->md_info = md_info;
    ctx->md_ctx = NULL;
    ctx->hmac_ctx = NULL;

    switch (md_info->type) {
        case MBEDTLS_MD_SHA1:
            ALLOC(sha1);
            break;
        case MBEDTLS_MD_SHA256:
            ALLOC(sha256);
            break;
        case MBEDTLS_MD_SHA384:
            ALLOC(sha512);
            break;
        case MBEDTLS_MD_SHA512:
            ALLOC(sha512);
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    if (hmac != 0) {
        ctx->hmac_ctx = mbedtls_calloc(2, md_info->block_size);
        if (ctx->hmac_ctx == NULL) {
            mbedtls_md_free(ctx);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_ALLOC_FAILED);
        }
    }

    MINTLS_RETURN(0);
}
#undef ALLOC

int mbedtls_md_starts(mbedtls_md_context_t *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx == NULL || ctx->md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    switch (ctx->md_info->type) {
        case MBEDTLS_MD_SHA1:
            MINTLS_RETURN(mbedtls_sha1_starts(ctx->md_ctx));
        case MBEDTLS_MD_SHA256:
            MINTLS_RETURN(mbedtls_sha256_starts(ctx->md_ctx, 0, diagnostics));
        case MBEDTLS_MD_SHA384:
            MINTLS_RETURN(mbedtls_sha512_starts(ctx->md_ctx, 1, diagnostics));
        case MBEDTLS_MD_SHA512:
            MINTLS_RETURN(mbedtls_sha512_starts(ctx->md_ctx, 0, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }
}

int mbedtls_md_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx == NULL || ctx->md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    switch (ctx->md_info->type) {
        case MBEDTLS_MD_SHA1:
            MINTLS_RETURN(mbedtls_sha1_update(ctx->md_ctx, input, ilen, diagnostics));
        case MBEDTLS_MD_SHA256:
            MINTLS_RETURN(mbedtls_sha256_update(ctx->md_ctx, input, ilen, diagnostics));
        case MBEDTLS_MD_SHA384:
            MINTLS_RETURN(mbedtls_sha512_update(ctx->md_ctx, input, ilen, diagnostics));
        case MBEDTLS_MD_SHA512:
            MINTLS_RETURN(mbedtls_sha512_update(ctx->md_ctx, input, ilen, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }
}

int mbedtls_md_finish(mbedtls_md_context_t *ctx, unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx == NULL || ctx->md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    switch (ctx->md_info->type) {
        case MBEDTLS_MD_SHA1:
            MINTLS_RETURN(mbedtls_sha1_finish(ctx->md_ctx, output, diagnostics));
        case MBEDTLS_MD_SHA256:
            MINTLS_RETURN(mbedtls_sha256_finish(ctx->md_ctx, output, diagnostics));
        case MBEDTLS_MD_SHA384:
            MINTLS_RETURN(mbedtls_sha512_finish(ctx->md_ctx, output, diagnostics));
        case MBEDTLS_MD_SHA512:
            MINTLS_RETURN(mbedtls_sha512_finish(ctx->md_ctx, output, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }
}

int mbedtls_md(const mbedtls_md_info_t *md_info, const unsigned char *input, size_t ilen,
               unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    switch (md_info->type) {
        case MBEDTLS_MD_SHA1:
            MINTLS_RETURN(mbedtls_sha1(input, ilen, output, diagnostics));
        case MBEDTLS_MD_SHA256:
            MINTLS_RETURN(mbedtls_sha256(input, ilen, output, 0, diagnostics));
        case MBEDTLS_MD_SHA384:
            MINTLS_RETURN(mbedtls_sha512(input, ilen, output, 1, diagnostics));
        case MBEDTLS_MD_SHA512:
            MINTLS_RETURN(mbedtls_sha512(input, ilen, output, 0, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }
}

unsigned char mbedtls_md_get_size(const mbedtls_md_info_t *md_info)
{
    if (md_info == NULL) {
        return 0;
    }

    return md_info->size;
}

mbedtls_md_type_t mbedtls_md_get_type(const mbedtls_md_info_t *md_info)
{
    if (md_info == NULL) {
        return MBEDTLS_MD_NONE;
    }

    return md_info->type;
}

/************************************************************************
 * Functions above this separator are part of MBEDTLS_MD_LIGHT,         *
 * functions below are only available when MBEDTLS_MD_C is set.         *
 ************************************************************************/

/*
 * Reminder: update profiles in x509_crt.c when adding a new hash!
 */
static const int supported_digests[] = {

    MBEDTLS_MD_SHA512,

    MBEDTLS_MD_SHA384,

    MBEDTLS_MD_SHA256,

    MBEDTLS_MD_SHA1,

    MBEDTLS_MD_NONE
};

const int *mbedtls_md_list(void)
{
    return supported_digests;
}

typedef struct {
    const char *md_name;
    mbedtls_md_type_t md_type;
} md_name_entry;

static const md_name_entry md_names[] = {
    { "SHA1", MBEDTLS_MD_SHA1 },
    { "SHA", MBEDTLS_MD_SHA1 }, // compatibility fallback
    { "SHA256", MBEDTLS_MD_SHA256 },
    { "SHA384", MBEDTLS_MD_SHA384 },
    { "SHA512", MBEDTLS_MD_SHA512 },
    { NULL, MBEDTLS_MD_NONE },
};

const mbedtls_md_info_t *mbedtls_md_info_from_string(const char *md_name)
{
    if (NULL == md_name) {
        return NULL;
    }

    const md_name_entry *entry = md_names;
    while (entry->md_name != NULL &&
           strcmp(entry->md_name, md_name) != 0) {
        ++entry;
    }

    return mbedtls_md_info_from_type(entry->md_type);
}

const char *mbedtls_md_get_name(const mbedtls_md_info_t *md_info)
{
    if (md_info == NULL) {
        return NULL;
    }

    const md_name_entry *entry = md_names;
    while (entry->md_type != MBEDTLS_MD_NONE &&
           entry->md_type != md_info->type) {
        ++entry;
    }

    return entry->md_name;
}

const mbedtls_md_info_t *mbedtls_md_info_from_ctx(
    const mbedtls_md_context_t *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return ctx->MBEDTLS_PRIVATE(md_info);
}

int mbedtls_md_hmac_starts(mbedtls_md_context_t *ctx, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char sum[MBEDTLS_MD_MAX_SIZE];
    unsigned char *ipad, *opad;

    if (ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    if (keylen > (size_t) ctx->md_info->block_size) {
        if ((ret = mbedtls_md_starts(ctx, diagnostics)) != 0) {
            goto cleanup;
        }
        if ((ret = mbedtls_md_update(ctx, key, keylen, diagnostics)) != 0) {
            goto cleanup;
        }
        if ((ret = mbedtls_md_finish(ctx, sum, diagnostics)) != 0) {
            goto cleanup;
        }

        keylen = ctx->md_info->size;
        key = sum;
    }

    ipad = (unsigned char *) ctx->hmac_ctx;
    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

    memset(ipad, 0x36, ctx->md_info->block_size);
    memset(opad, 0x5C, ctx->md_info->block_size);

    mbedtls_xor(ipad, ipad, key, keylen);
    mbedtls_xor(opad, opad, key, keylen);

    if ((ret = mbedtls_md_starts(ctx, diagnostics)) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_md_update(ctx, ipad,
                                 ctx->md_info->block_size, diagnostics)) != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_platform_zeroize(sum, sizeof(sum));

    MINTLS_RETURN(ret);
}

int mbedtls_md_hmac_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(mbedtls_md_update(ctx, input, ilen, diagnostics));
}

int mbedtls_md_hmac_finish(mbedtls_md_context_t *ctx, unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char tmp[MBEDTLS_MD_MAX_SIZE];
    unsigned char *opad;

    if (ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

    if ((ret = mbedtls_md_finish(ctx, tmp, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    if ((ret = mbedtls_md_starts(ctx, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    if ((ret = mbedtls_md_update(ctx, opad,
                                 ctx->md_info->block_size, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    if ((ret = mbedtls_md_update(ctx, tmp,
                                 ctx->md_info->size, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    MINTLS_RETURN(mbedtls_md_finish(ctx, output, diagnostics));
}

int mbedtls_md_hmac_reset(mbedtls_md_context_t *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *ipad;

    if (ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    ipad = (unsigned char *) ctx->hmac_ctx;

    if ((ret = mbedtls_md_starts(ctx, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    MINTLS_RETURN(mbedtls_md_update(ctx, ipad, ctx->md_info->block_size, diagnostics));
}

int mbedtls_md_hmac(const mbedtls_md_info_t *md_info,
                    const unsigned char *key, size_t keylen,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_md_context_t ctx;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    }

    mbedtls_md_init(&ctx);

    if ((ret = mbedtls_md_setup(&ctx, md_info, 1, diagnostics)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_md_hmac_starts(&ctx, key, keylen, diagnostics)) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_md_hmac_update(&ctx, input, ilen, diagnostics)) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_md_hmac_finish(&ctx, output, diagnostics)) != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_md_free(&ctx);

    MINTLS_RETURN(ret);
}

