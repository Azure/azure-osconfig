/*
 *  CTR_DRBG implementation based on AES-256 (NIST SP 800-90)
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: library/ctr_drbg.c.
 */
/*
 *  The NIST SP 800-90 DRBGs are described in the following publication.
 *
 *  https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-90r.pdf
 */

#include "common.h"

#include "ctr.h"
#include "ctr_drbg.h"
#include "platform_util.h"
#include "error.h"

#include <string.h>

/* Using error translation functions from PSA to MbedTLS */

#include "platform.h"

/*
 * CTR_DRBG context initialization
 */
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_ctr_drbg_context));
    mbedtls_aes_init(&ctx->aes_ctx);
    /* Indicate that the entropy nonce length is not set explicitly.
     * See mbedtls_ctr_drbg_set_nonce_len(). */
    ctx->reseed_counter = -1;

    ctx->reseed_interval = MBEDTLS_CTR_DRBG_RESEED_INTERVAL;
}

/*
 *  This function resets CTR_DRBG context to the state immediately
 *  after initial call of mbedtls_ctr_drbg_init().
 */
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_aes_free(&ctx->aes_ctx);
    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_ctr_drbg_context));
    ctx->reseed_interval = MBEDTLS_CTR_DRBG_RESEED_INTERVAL;
    ctx->reseed_counter = -1;
}

void mbedtls_ctr_drbg_set_prediction_resistance(mbedtls_ctr_drbg_context *ctx,
                                                int resistance)
{
    ctx->prediction_resistance = resistance;
}

void mbedtls_ctr_drbg_set_entropy_len(mbedtls_ctr_drbg_context *ctx,
                                      size_t len)
{
    ctx->entropy_len = len;
}

int mbedtls_ctr_drbg_set_nonce_len(mbedtls_ctr_drbg_context *ctx,
                                   size_t len)
{
    /* If mbedtls_ctr_drbg_seed() has already been called, it's
     * too late. Return the error code that's closest to making sense. */
    if (ctx->f_entropy != NULL) {
        return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    }

    if (len > MBEDTLS_CTR_DRBG_MAX_SEED_INPUT) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }

    /* This shouldn't be an issue because
     * MBEDTLS_CTR_DRBG_MAX_SEED_INPUT < INT_MAX in any sensible
     * configuration, but make sure anyway. */
    if (len > INT_MAX) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }

    /* For backward compatibility with Mbed TLS <= 2.19, store the
     * entropy nonce length in a field that already exists, but isn't
     * used until after the initial seeding. */
    /* Due to the capping of len above, the value fits in an int. */
    ctx->reseed_counter = (int) len;
    return 0;
}

void mbedtls_ctr_drbg_set_reseed_interval(mbedtls_ctr_drbg_context *ctx,
                                          int interval)
{
    ctx->reseed_interval = interval;
}

int block_cipher_df(unsigned char *output,
                           const unsigned char *data, size_t data_len)
{
    unsigned char buf[MBEDTLS_CTR_DRBG_MAX_SEED_INPUT +
                      MBEDTLS_CTR_DRBG_BLOCKSIZE + 16];
    unsigned char tmp[MBEDTLS_CTR_DRBG_SEEDLEN];
    unsigned char key[MBEDTLS_CTR_DRBG_KEYSIZE];
    unsigned char chain[MBEDTLS_CTR_DRBG_BLOCKSIZE];
    unsigned char *p, *iv;
    int ret = 0;
    mbedtls_aes_context aes_ctx;

    int i, j;
    size_t buf_len, use_len;

    if (data_len > MBEDTLS_CTR_DRBG_MAX_SEED_INPUT) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }

    memset(buf, 0, MBEDTLS_CTR_DRBG_MAX_SEED_INPUT +
           MBEDTLS_CTR_DRBG_BLOCKSIZE + 16);

    /*
     * Construct IV (16 bytes) and S in buffer
     * IV = Counter (in 32-bits) padded to 16 with zeroes
     * S = Length input string (in 32-bits) || Length of output (in 32-bits) ||
     *     data || 0x80
     *     (Total is padded to a multiple of 16-bytes with zeroes)
     */
    p = buf + MBEDTLS_CTR_DRBG_BLOCKSIZE;
    MBEDTLS_PUT_UINT32_BE(data_len, p, 0);
    p += 4 + 3;
    *p++ = MBEDTLS_CTR_DRBG_SEEDLEN;
    memcpy(p, data, data_len);
    p[data_len] = 0x80;

    buf_len = MBEDTLS_CTR_DRBG_BLOCKSIZE + 8 + data_len + 1;

    for (i = 0; i < MBEDTLS_CTR_DRBG_KEYSIZE; i++) {
        key[i] = i;
    }

    mbedtls_aes_init(&aes_ctx);

    if ((ret = mbedtls_aes_setkey_enc(&aes_ctx, key,
                                      MBEDTLS_CTR_DRBG_KEYBITS)) != 0) {
        goto exit;
    }

    /*
     * Reduce data to MBEDTLS_CTR_DRBG_SEEDLEN bytes of data
     */
    for (j = 0; j < MBEDTLS_CTR_DRBG_SEEDLEN; j += MBEDTLS_CTR_DRBG_BLOCKSIZE) {
        p = buf;
        memset(chain, 0, MBEDTLS_CTR_DRBG_BLOCKSIZE);
        use_len = buf_len;

        while (use_len > 0) {
            mbedtls_xor(chain, chain, p, MBEDTLS_CTR_DRBG_BLOCKSIZE);
            p += MBEDTLS_CTR_DRBG_BLOCKSIZE;
            use_len -= (use_len >= MBEDTLS_CTR_DRBG_BLOCKSIZE) ?
                       MBEDTLS_CTR_DRBG_BLOCKSIZE : use_len;

            if ((ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT,
                                             chain, chain)) != 0) {
                goto exit;
            }
        }

        memcpy(tmp + j, chain, MBEDTLS_CTR_DRBG_BLOCKSIZE);

        /*
         * Update IV
         */
        buf[3]++;
    }

    /*
     * Do final encryption with reduced data
     */
    if ((ret = mbedtls_aes_setkey_enc(&aes_ctx, tmp,
                                      MBEDTLS_CTR_DRBG_KEYBITS)) != 0) {
        goto exit;
    }
    iv = tmp + MBEDTLS_CTR_DRBG_KEYSIZE;
    p = output;

    for (j = 0; j < MBEDTLS_CTR_DRBG_SEEDLEN; j += MBEDTLS_CTR_DRBG_BLOCKSIZE) {
        if ((ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT,
                                         iv, iv)) != 0) {
            goto exit;
        }
        memcpy(p, iv, MBEDTLS_CTR_DRBG_BLOCKSIZE);
        p += MBEDTLS_CTR_DRBG_BLOCKSIZE;
    }
exit:
    mbedtls_aes_free(&aes_ctx);
    /*
     * tidy up the stack
     */
    mbedtls_platform_zeroize(buf, sizeof(buf));
    mbedtls_platform_zeroize(tmp, sizeof(tmp));
    mbedtls_platform_zeroize(key, sizeof(key));
    mbedtls_platform_zeroize(chain, sizeof(chain));
    if (0 != ret) {
        /*
         * wipe partial seed from memory
         */
        mbedtls_platform_zeroize(output, MBEDTLS_CTR_DRBG_SEEDLEN);
    }

    return ret;
}

/* CTR_DRBG_Update (SP 800-90A &sect;10.2.1.2)
 * ctr_drbg_update_internal(ctx, provided_data)
 * implements
 * CTR_DRBG_Update(provided_data, Key, V)
 * with inputs and outputs
 *   ctx->aes_ctx = Key
 *   ctx->counter = V
 */
int ctr_drbg_update_internal(mbedtls_ctr_drbg_context *ctx,
                                    const unsigned char data[MBEDTLS_CTR_DRBG_SEEDLEN])
{
    unsigned char tmp[MBEDTLS_CTR_DRBG_SEEDLEN];
    unsigned char *p = tmp;
    int j;
    int ret = 0;

    memset(tmp, 0, MBEDTLS_CTR_DRBG_SEEDLEN);

    for (j = 0; j < MBEDTLS_CTR_DRBG_SEEDLEN; j += MBEDTLS_CTR_DRBG_BLOCKSIZE) {
        /*
         * Increase counter
         */
        mbedtls_ctr_increment_counter(ctx->counter);

        /*
         * Crypt counter block
         */
        if ((ret = mbedtls_aes_crypt_ecb(&ctx->aes_ctx, MBEDTLS_AES_ENCRYPT,
                                         ctx->counter, p)) != 0) {
            goto exit;
        }

        p += MBEDTLS_CTR_DRBG_BLOCKSIZE;
    }

    mbedtls_xor(tmp, tmp, data, MBEDTLS_CTR_DRBG_SEEDLEN);

    /*
     * Update key and counter
     */
    if ((ret = mbedtls_aes_setkey_enc(&ctx->aes_ctx, tmp,
                                      MBEDTLS_CTR_DRBG_KEYBITS)) != 0) {
        goto exit;
    }
    memcpy(ctx->counter, tmp + MBEDTLS_CTR_DRBG_KEYSIZE,
           MBEDTLS_CTR_DRBG_BLOCKSIZE);

exit:
    mbedtls_platform_zeroize(tmp, sizeof(tmp));
    return ret;
}

/* CTR_DRBG_Instantiate with derivation function (SP 800-90A &sect;10.2.1.3.2)
 * mbedtls_ctr_drbg_update(ctx, additional, add_len)
 * implements
 * CTR_DRBG_Instantiate(entropy_input, nonce, personalization_string,
 *                      security_strength) -> initial_working_state
 * with inputs
 *   ctx->counter = all-bits-0
 *   ctx->aes_ctx = context from all-bits-0 key
 *   additional[:add_len] = entropy_input || nonce || personalization_string
 * and with outputs
 *   ctx = initial_working_state
 */
int mbedtls_ctr_drbg_update(mbedtls_ctr_drbg_context *ctx,
                            const unsigned char *additional,
                            size_t add_len)
{
    unsigned char add_input[MBEDTLS_CTR_DRBG_SEEDLEN];
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (add_len == 0) {
        return 0;
    }

    if ((ret = block_cipher_df(add_input, additional, add_len)) != 0) {
        goto exit;
    }
    if ((ret = ctr_drbg_update_internal(ctx, add_input)) != 0) {
        goto exit;
    }

exit:
    mbedtls_platform_zeroize(add_input, sizeof(add_input));
    return ret;
}

/* CTR_DRBG_Reseed with derivation function (SP 800-90A &sect;10.2.1.4.2)
 * mbedtls_ctr_drbg_reseed(ctx, additional, len, nonce_len)
 * implements
 * CTR_DRBG_Reseed(working_state, entropy_input, additional_input)
 *                -> new_working_state
 * with inputs
 *   ctx contains working_state
 *   additional[:len] = additional_input
 * and entropy_input comes from calling ctx->f_entropy
 *                              for (ctx->entropy_len + nonce_len) bytes
 * and with output
 *   ctx contains new_working_state
 */
int mbedtls_ctr_drbg_reseed_internal(mbedtls_ctr_drbg_context *ctx,
                                            const unsigned char *additional,
                                            size_t len,
                                            size_t nonce_len)
{
    unsigned char seed[MBEDTLS_CTR_DRBG_MAX_SEED_INPUT];
    size_t seedlen = 0;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ctx->entropy_len > MBEDTLS_CTR_DRBG_MAX_SEED_INPUT) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }
    if (nonce_len > MBEDTLS_CTR_DRBG_MAX_SEED_INPUT - ctx->entropy_len) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }
    if (len > MBEDTLS_CTR_DRBG_MAX_SEED_INPUT - ctx->entropy_len - nonce_len) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }

    memset(seed, 0, MBEDTLS_CTR_DRBG_MAX_SEED_INPUT);

    /* Gather entropy_len bytes of entropy to seed state. */
    if (0 != ctx->f_entropy(ctx->p_entropy, seed, ctx->entropy_len)) {
        return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    }
    seedlen += ctx->entropy_len;

    /* Gather entropy for a nonce if requested. */
    if (nonce_len != 0) {
        if (0 != ctx->f_entropy(ctx->p_entropy, seed + seedlen, nonce_len)) {
            return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
        }
        seedlen += nonce_len;
    }

    /* Add additional data if provided. */
    if (additional != NULL && len != 0) {
        memcpy(seed + seedlen, additional, len);
        seedlen += len;
    }

    /* Reduce to 384 bits. */
    if ((ret = block_cipher_df(seed, seed, seedlen)) != 0) {
        goto exit;
    }

    /* Update state. */
    if ((ret = ctr_drbg_update_internal(ctx, seed)) != 0) {
        goto exit;
    }
    ctx->reseed_counter = 0;

exit:
    mbedtls_platform_zeroize(seed, sizeof(seed));
    return ret;
}

int mbedtls_ctr_drbg_reseed(mbedtls_ctr_drbg_context *ctx,
                            const unsigned char *additional, size_t len)
{
    return mbedtls_ctr_drbg_reseed_internal(ctx, additional, len, 0);
}

/* Return a "good" nonce length for CTR_DRBG. The chosen nonce length
 * is sufficient to achieve the maximum security strength given the key
 * size and entropy length. If there is enough entropy in the initial
 * call to the entropy function to serve as both the entropy input and
 * the nonce, don't make a second call to get a nonce. */
size_t good_nonce_len(size_t entropy_len)
{
    if (entropy_len >= MBEDTLS_CTR_DRBG_KEYSIZE * 3 / 2) {
        return 0;
    } else {
        return (entropy_len + 1) / 2;
    }
}

/* CTR_DRBG_Instantiate with derivation function (SP 800-90A &sect;10.2.1.3.2)
 * mbedtls_ctr_drbg_seed(ctx, f_entropy, p_entropy, custom, len)
 * implements
 * CTR_DRBG_Instantiate(entropy_input, nonce, personalization_string,
 *                      security_strength) -> initial_working_state
 * with inputs
 *   custom[:len] = nonce || personalization_string
 * where entropy_input comes from f_entropy for ctx->entropy_len bytes
 * and with outputs
 *   ctx = initial_working_state
 */
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                          int (*f_entropy)(void *, unsigned char *, size_t),
                          void *p_entropy,
                          const unsigned char *custom,
                          size_t len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char key[MBEDTLS_CTR_DRBG_KEYSIZE];
    size_t nonce_len;

    memset(key, 0, MBEDTLS_CTR_DRBG_KEYSIZE);

    /* The mutex is initialized iff f_entropy is set. */

    ctx->f_entropy = f_entropy;
    ctx->p_entropy = p_entropy;

    if (ctx->entropy_len == 0) {
        ctx->entropy_len = MBEDTLS_CTR_DRBG_ENTROPY_LEN;
    }
    /* ctx->reseed_counter contains the desired amount of entropy to
     * grab for a nonce (see mbedtls_ctr_drbg_set_nonce_len()).
     * If it's -1, indicating that the entropy nonce length was not set
     * explicitly, use a sufficiently large nonce for security. */
    nonce_len = (ctx->reseed_counter >= 0 ?
                 (size_t) ctx->reseed_counter :
                 good_nonce_len(ctx->entropy_len));

    /* Initialize with an empty key. */
    if ((ret = mbedtls_aes_setkey_enc(&ctx->aes_ctx, key,
                                      MBEDTLS_CTR_DRBG_KEYBITS)) != 0) {
        return ret;
    }

    /* Do the initial seeding. */
    if ((ret = mbedtls_ctr_drbg_reseed_internal(ctx, custom, len,
                                                nonce_len)) != 0) {
        return ret;
    }
    return 0;
}

/* CTR_DRBG_Generate with derivation function (SP 800-90A &sect;10.2.1.5.2)
 * mbedtls_ctr_drbg_random_with_add(ctx, output, output_len, additional, add_len)
 * implements
 * CTR_DRBG_Reseed(working_state, entropy_input, additional[:add_len])
 *                -> working_state_after_reseed
 *                if required, then
 * CTR_DRBG_Generate(working_state_after_reseed,
 *                   requested_number_of_bits, additional_input)
 *                -> status, returned_bits, new_working_state
 * with inputs
 *   ctx contains working_state
 *   requested_number_of_bits = 8 * output_len
 *   additional[:add_len] = additional_input
 * and entropy_input comes from calling ctx->f_entropy
 * and with outputs
 *   status = SUCCESS (this function does the reseed internally)
 *   returned_bits = output[:output_len]
 *   ctx contains new_working_state
 */
int mbedtls_ctr_drbg_random_with_add(void *p_rng,
                                     unsigned char *output, size_t output_len,
                                     const unsigned char *additional, size_t add_len)
{
    int ret = 0;
    mbedtls_ctr_drbg_context *ctx = (mbedtls_ctr_drbg_context *) p_rng;
    unsigned char *p = output;
    struct {
        unsigned char add_input[MBEDTLS_CTR_DRBG_SEEDLEN];
        unsigned char tmp[MBEDTLS_CTR_DRBG_BLOCKSIZE];
    } locals;
    size_t use_len;

    if (output_len > MBEDTLS_CTR_DRBG_MAX_REQUEST) {
        return MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG;
    }

    if (add_len > MBEDTLS_CTR_DRBG_MAX_INPUT) {
        return MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG;
    }

    memset(locals.add_input, 0, MBEDTLS_CTR_DRBG_SEEDLEN);

    if (ctx->reseed_counter >= ctx->reseed_interval ||
        ctx->prediction_resistance) {
        if ((ret = mbedtls_ctr_drbg_reseed(ctx, additional, add_len)) != 0) {
            return ret;
        }
        add_len = 0;
    }

    if (add_len > 0) {
        if ((ret = block_cipher_df(locals.add_input, additional, add_len)) != 0) {
            goto exit;
        }
        if ((ret = ctr_drbg_update_internal(ctx, locals.add_input)) != 0) {
            goto exit;
        }
    }

    while (output_len > 0) {
        /*
         * Increase counter (treat it as a 128-bit big-endian integer).
         */
        mbedtls_ctr_increment_counter(ctx->counter);

        /*
         * Crypt counter block
         */
        if ((ret = mbedtls_aes_crypt_ecb(&ctx->aes_ctx, MBEDTLS_AES_ENCRYPT,
                                         ctx->counter, locals.tmp)) != 0) {
            goto exit;
        }

        use_len = (output_len > MBEDTLS_CTR_DRBG_BLOCKSIZE)
            ? MBEDTLS_CTR_DRBG_BLOCKSIZE : output_len;
        /*
         * Copy random block to destination
         */
        memcpy(p, locals.tmp, use_len);
        p += use_len;
        output_len -= use_len;
    }

    if ((ret = ctr_drbg_update_internal(ctx, locals.add_input)) != 0) {
        goto exit;
    }

    ctx->reseed_counter++;

exit:
    mbedtls_platform_zeroize(&locals, sizeof(locals));
    return ret;
}

int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output,
                            size_t output_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ctr_drbg_context *ctx = (mbedtls_ctr_drbg_context *) p_rng;

    ret = mbedtls_ctr_drbg_random_with_add(ctx, output, output_len, NULL, 0);

    return ret;
}

