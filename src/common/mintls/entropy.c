/*
 *  Entropy accumulator implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: library/entropy.c.
 */

#include "common.h"

#include "entropy.h"
#include "entropy_poll.h"
#include "platform_util.h"
#include "error.h"

#include <string.h>

#include "platform.h"

#define ENTROPY_MAX_LOOP    256     /**< Maximum amount to loop before error */

void mbedtls_entropy_init(mbedtls_entropy_context *ctx)
{
    ctx->source_count = 0;
    memset(ctx->source, 0, sizeof(ctx->source));

    ctx->accumulator_started = 0;
    mbedtls_md_init(&ctx->accumulator);

    /* Reminder: Update ENTROPY_HAVE_STRONG in the test files
     *           when adding more strong entropy sources here. */

#if !defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
    mbedtls_entropy_add_source(ctx, mbedtls_platform_entropy_poll, NULL,
                               MBEDTLS_ENTROPY_MIN_PLATFORM,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);
#endif /* MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES */
}

void mbedtls_entropy_free(mbedtls_entropy_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* If the context was already free, don't call free() again.
     * This is important for mutexes which don't allow double-free. */
    if (ctx->accumulator_started == -1) {
        return;
    }

    mbedtls_md_free(&ctx->accumulator);
    ctx->source_count = 0;
    mbedtls_platform_zeroize(ctx->source, sizeof(ctx->source));
    ctx->accumulator_started = -1;
}

int mbedtls_entropy_add_source(mbedtls_entropy_context *ctx,
                               mbedtls_entropy_f_source_ptr f_source, void *p_source,
                               size_t threshold, int strong)
{
    int idx, ret = 0;

    idx = ctx->source_count;
    if (idx >= MBEDTLS_ENTROPY_MAX_SOURCES) {
        ret = MBEDTLS_ERR_ENTROPY_MAX_SOURCES;
        goto exit;
    }

    ctx->source[idx].f_source  = f_source;
    ctx->source[idx].p_source  = p_source;
    ctx->source[idx].threshold = threshold;
    ctx->source[idx].strong    = strong;

    ctx->source_count++;

exit:

    return ret;
}

/*
 * Entropy accumulator update
 */
int entropy_update(mbedtls_entropy_context *ctx, unsigned char source_id,
                          const unsigned char *data, size_t len)
{
    unsigned char header[2];
    unsigned char tmp[MBEDTLS_ENTROPY_BLOCK_SIZE];
    size_t use_len = len;
    const unsigned char *p = data;
    int ret = 0;

    if (use_len > MBEDTLS_ENTROPY_BLOCK_SIZE) {
        if ((ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_ENTROPY_MD),
                              data, len, tmp)) != 0) {
            goto cleanup;
        }
        p = tmp;
        use_len = MBEDTLS_ENTROPY_BLOCK_SIZE;
    }

    header[0] = source_id;
    header[1] = use_len & 0xFF;

    /*
     * Start the accumulator if this has not already happened. Note that
     * it is sufficient to start the accumulator here only because all calls to
     * gather entropy eventually execute this code.
     */
    if (ctx->accumulator_started == 0) {
        ret = mbedtls_md_setup(&ctx->accumulator,
                               mbedtls_md_info_from_type(MBEDTLS_ENTROPY_MD), 0);
        if (ret != 0) {
            goto cleanup;
        }
        ret = mbedtls_md_starts(&ctx->accumulator);
        if (ret != 0) {
            goto cleanup;
        }
        ctx->accumulator_started = 1;
    }
    if ((ret = mbedtls_md_update(&ctx->accumulator, header, 2)) != 0) {
        goto cleanup;
    }
    ret = mbedtls_md_update(&ctx->accumulator, p, use_len);

cleanup:
    mbedtls_platform_zeroize(tmp, sizeof(tmp));

    return ret;
}

int mbedtls_entropy_update_manual(mbedtls_entropy_context *ctx,
                                  const unsigned char *data, size_t len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    ret = entropy_update(ctx, MBEDTLS_ENTROPY_SOURCE_MANUAL, data, len);

    return ret;
}

/*
 * Run through the different sources to add entropy to our accumulator
 */
int entropy_gather_internal(mbedtls_entropy_context *ctx)
{
    int ret = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    int i;
    int have_one_strong = 0;
    unsigned char buf[MBEDTLS_ENTROPY_MAX_GATHER];
    size_t olen;

    if (ctx->source_count == 0) {
        return MBEDTLS_ERR_ENTROPY_NO_SOURCES_DEFINED;
    }

    /*
     * Run through our entropy sources
     */
    for (i = 0; i < ctx->source_count; i++) {
        if (ctx->source[i].strong == MBEDTLS_ENTROPY_SOURCE_STRONG) {
            have_one_strong = 1;
        }

        olen = 0;
        if ((ret = ctx->source[i].f_source(ctx->source[i].p_source,
                                           buf, MBEDTLS_ENTROPY_MAX_GATHER, &olen)) != 0) {
            goto cleanup;
        }

        /*
         * Add if we actually gathered something
         */
        if (olen > 0) {
            if ((ret = entropy_update(ctx, (unsigned char) i,
                                      buf, olen)) != 0) {
                return ret;
            }
            ctx->source[i].size += olen;
        }
    }

    if (have_one_strong == 0) {
        ret = MBEDTLS_ERR_ENTROPY_NO_STRONG_SOURCE;
    }

cleanup:
    mbedtls_platform_zeroize(buf, sizeof(buf));

    return ret;
}

/*
 * Thread-safe wrapper for entropy_gather_internal()
 */
int mbedtls_entropy_gather(mbedtls_entropy_context *ctx)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    ret = entropy_gather_internal(ctx);

    return ret;
}

int mbedtls_entropy_func(void *data, unsigned char *output, size_t len)
{
    int ret, count = 0, i, thresholds_reached;
    size_t strong_size;
    mbedtls_entropy_context *ctx = (mbedtls_entropy_context *) data;
    unsigned char buf[MBEDTLS_ENTROPY_BLOCK_SIZE];

    if (len > MBEDTLS_ENTROPY_BLOCK_SIZE) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    /*
     * Always gather extra entropy before a call
     */
    do {
        if (count++ > ENTROPY_MAX_LOOP) {
            ret = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
            goto exit;
        }

        if ((ret = entropy_gather_internal(ctx)) != 0) {
            goto exit;
        }

        thresholds_reached = 1;
        strong_size = 0;
        for (i = 0; i < ctx->source_count; i++) {
            if (ctx->source[i].size < ctx->source[i].threshold) {
                thresholds_reached = 0;
            }
            if (ctx->source[i].strong == MBEDTLS_ENTROPY_SOURCE_STRONG) {
                strong_size += ctx->source[i].size;
            }
        }
    } while (!thresholds_reached || strong_size < MBEDTLS_ENTROPY_BLOCK_SIZE);

    memset(buf, 0, MBEDTLS_ENTROPY_BLOCK_SIZE);

    /*
     * Note that at this stage it is assumed that the accumulator was started
     * in a previous call to entropy_update(). If this is not guaranteed, the
     * code below will fail.
     */
    if ((ret = mbedtls_md_finish(&ctx->accumulator, buf)) != 0) {
        goto exit;
    }

    /*
     * Reset accumulator and counters and recycle existing entropy
     */
    mbedtls_md_free(&ctx->accumulator);
    mbedtls_md_init(&ctx->accumulator);
    ret = mbedtls_md_setup(&ctx->accumulator,
                           mbedtls_md_info_from_type(MBEDTLS_ENTROPY_MD), 0);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_starts(&ctx->accumulator);
    if (ret != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&ctx->accumulator, buf,
                                 MBEDTLS_ENTROPY_BLOCK_SIZE)) != 0) {
        goto exit;
    }

    /*
     * Perform second hashing on entropy
     */
    if ((ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_ENTROPY_MD),
                          buf, MBEDTLS_ENTROPY_BLOCK_SIZE, buf)) != 0) {
        goto exit;
    }

    for (i = 0; i < ctx->source_count; i++) {
        ctx->source[i].size = 0;
    }

    memcpy(output, buf, len);

    ret = 0;

exit:
    mbedtls_platform_zeroize(buf, sizeof(buf));

    return ret;
}

