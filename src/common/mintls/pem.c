// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Privacy Enhanced Mail (PEM) decoding
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pem.c.
 */

#include "common.h"

#include "pem.h"
#include "base64.h"
#include "aes.h"
#include "md.h"
#include "cipher.h"
#include "platform_util.h"
#include "error.h"

#include <string.h>

#include "platform.h"

void mbedtls_pem_init(mbedtls_pem_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_pem_context));
}

#if defined(PEM_RFC1421)
/*
 * Read a 16-byte hex string and convert it to binary
 */
int pem_get_iv(const unsigned char *s, unsigned char *iv,
                      size_t iv_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t i, j, k;

    memset(iv, 0, iv_len);

    for (i = 0; i < iv_len * 2; i++, s++) {
        if (*s >= '0' && *s <= '9') {
            j = *s - '0';
        } else
        if (*s >= 'A' && *s <= 'F') {
            j = *s - '7';
        } else
        if (*s >= 'a' && *s <= 'f') {
            j = *s - 'W';
        } else {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_ENC_IV);
        }

        k = ((i & 1) != 0) ? j : j << 4;

        iv[i >> 1] = (unsigned char) (iv[i >> 1] | k);
    }

    MINTLS_RETURN(0);
}

int pem_pbkdf1(unsigned char *key, size_t keylen,
                      unsigned char *iv,
                      const unsigned char *pwd, size_t pwdlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_md_context_t md5_ctx;
    const mbedtls_md_info_t *md5_info;
    unsigned char md5sum[16];
    size_t use_len;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    mbedtls_md_init(&md5_ctx);

    /* Prepare the context. (setup() errors gracefully on NULL info.) */
    md5_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    if ((ret = mbedtls_md_setup(&md5_ctx, md5_info, 0, diagnostics)) != 0) {
        goto exit;
    }

    /*
     * key[ 0..15] = MD5(pwd || IV)
     */
    if ((ret = mbedtls_md_starts(&md5_ctx, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md5_ctx, pwd, pwdlen, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md5_ctx, iv,  8, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_finish(&md5_ctx, md5sum, diagnostics)) != 0) {
        goto exit;
    }

    if (keylen <= 16) {
        memcpy(key, md5sum, keylen);
        goto exit;
    }

    memcpy(key, md5sum, 16);

    /*
     * key[16..23] = MD5(key[ 0..15] || pwd || IV])
     */
    if ((ret = mbedtls_md_starts(&md5_ctx, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md5_ctx, md5sum, 16, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md5_ctx, pwd, pwdlen, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md5_ctx, iv, 8, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_finish(&md5_ctx, md5sum, diagnostics)) != 0) {
        goto exit;
    }

    use_len = 16;
    if (keylen < 32) {
        use_len = keylen - 16;
    }

    memcpy(key + 16, md5sum, use_len);

exit:
    mbedtls_md_free(&md5_ctx);
    mbedtls_platform_zeroize(md5sum, 16);

    MINTLS_RETURN(ret);
}

/*
 * Decrypt with AES-XXX-CBC, using PBKDF1 for key derivation
 */
int pem_aes_decrypt(unsigned char aes_iv[16], unsigned int keylen,
                           unsigned char *buf, size_t buflen,
                           const unsigned char *pwd, size_t pwdlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_aes_context aes_ctx;
    unsigned char aes_key[32];
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    mbedtls_aes_init(&aes_ctx);

    if ((ret = pem_pbkdf1(aes_key, keylen, aes_iv, pwd, pwdlen, diagnostics)) != 0) {
        goto exit;
    }

    if ((ret = mbedtls_aes_setkey_dec(&aes_ctx, aes_key, keylen * 8, diagnostics)) != 0) {
        goto exit;
    }
    ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, buflen,
                                aes_iv, buf, buf);

exit:
    mbedtls_aes_free(&aes_ctx);
    mbedtls_platform_zeroize(aes_key, keylen);

    MINTLS_RETURN(ret);
}

int pem_check_pkcs_padding(unsigned char *input, size_t input_len, size_t *data_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* input_len > 0 is not guaranteed by mbedtls_pem_read_buffer(). */
    if (input_len < 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_DATA);
    }
    size_t pad_len = input[input_len - 1];
    size_t i;

    if (pad_len > input_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_PASSWORD_MISMATCH);
    }

    *data_len = input_len - pad_len;

    for (i = *data_len; i < input_len; i++) {
        if (input[i] != pad_len) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_PASSWORD_MISMATCH);
        }
    }

    MINTLS_RETURN(0);
}

#endif /* PEM_RFC1421 */

int mbedtls_pem_read_buffer(mbedtls_pem_context *ctx, const char *header, const char *footer,
                            const unsigned char *data, const unsigned char *pwd,
                            size_t pwdlen, size_t *use_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, enc;
    size_t len;
    unsigned char *buf;
    const unsigned char *s1, *s2, *end;
#if defined(PEM_RFC1421)
    unsigned char pem_iv[16];
    mbedtls_cipher_type_t enc_alg = MBEDTLS_CIPHER_NONE;
#else
    ((void) pwd);
    ((void) pwdlen);
#endif /* PEM_RFC1421 */

    if (ctx == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_BAD_INPUT_DATA);
    }

    s1 = (unsigned char *) strstr((const char *) data, header);

    if (s1 == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    }

    s2 = (unsigned char *) strstr((const char *) data, footer);

    if (s2 == NULL || s2 <= s1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    }

    s1 += strlen(header);
    if (*s1 == ' ') {
        s1++;
    }
    if (*s1 == '\r') {
        s1++;
    }
    if (*s1 == '\n') {
        s1++;
    } else {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    }

    end = s2;
    end += strlen(footer);
    if (*end == ' ') {
        end++;
    }
    if (*end == '\r') {
        end++;
    }
    if (*end == '\n') {
        end++;
    }
    *use_len = (size_t) (end - data);

    enc = 0;

    if (s2 - s1 >= 22 && memcmp(s1, "Proc-Type: 4,ENCRYPTED", 22) == 0) {
#if defined(PEM_RFC1421)
        enc++;

        s1 += 22;
        if (*s1 == '\r') {
            s1++;
        }
        if (*s1 == '\n') {
            s1++;
        } else {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_DATA);
        }

        if (s2 - s1 >= 14 && memcmp(s1, "DEK-Info: AES-", 14) == 0) {
            if (s2 - s1 < 22) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_UNKNOWN_ENC_ALG);
            } else if (memcmp(s1, "DEK-Info: AES-128-CBC,", 22) == 0) {
                enc_alg = MBEDTLS_CIPHER_AES_128_CBC;
            } else if (memcmp(s1, "DEK-Info: AES-192-CBC,", 22) == 0) {
                enc_alg = MBEDTLS_CIPHER_AES_192_CBC;
            } else if (memcmp(s1, "DEK-Info: AES-256-CBC,", 22) == 0) {
                enc_alg = MBEDTLS_CIPHER_AES_256_CBC;
            } else {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_UNKNOWN_ENC_ALG);
            }

            s1 += 22;
            if (s2 - s1 < 32 || pem_get_iv(s1, pem_iv, 16, diagnostics) != 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_ENC_IV);
            }

            s1 += 32;
        }

        if (enc_alg == MBEDTLS_CIPHER_NONE) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_UNKNOWN_ENC_ALG);
        }

        if (*s1 == '\r') {
            s1++;
        }
        if (*s1 == '\n') {
            s1++;
        } else {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_DATA);
        }
#else
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_FEATURE_UNAVAILABLE);
#endif /* PEM_RFC1421 */
    }

    if (s1 >= s2) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_INVALID_DATA);
    }

    ret = mbedtls_base64_decode(NULL, 0, &len, s1, (size_t) (s2 - s1), diagnostics);

    if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PEM_INVALID_DATA, ret));
    }

    if (len == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_BAD_INPUT_DATA);
    }

    if ((buf = mbedtls_calloc(1, len)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_ALLOC_FAILED);
    }

    if ((ret = mbedtls_base64_decode(buf, len, &len, s1, (size_t) (s2 - s1), diagnostics)) != 0) {
        mbedtls_zeroize_and_free(buf, len);
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PEM_INVALID_DATA, ret));
    }

    if (enc != 0) {
#if defined(PEM_RFC1421)
        if (pwd == NULL) {
            mbedtls_zeroize_and_free(buf, len);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_PASSWORD_REQUIRED);
        }

        ret = 0;

        if (enc_alg == MBEDTLS_CIPHER_AES_128_CBC) {
            ret = pem_aes_decrypt(pem_iv, 16, buf, len, pwd, pwdlen, diagnostics);
        } else if (enc_alg == MBEDTLS_CIPHER_AES_192_CBC) {
            ret = pem_aes_decrypt(pem_iv, 24, buf, len, pwd, pwdlen, diagnostics);
        } else if (enc_alg == MBEDTLS_CIPHER_AES_256_CBC) {
            ret = pem_aes_decrypt(pem_iv, 32, buf, len, pwd, pwdlen, diagnostics);
        }

        if (ret != 0) {
            mbedtls_zeroize_and_free(buf, len);
            MINTLS_RETURN(ret);
        }

        /* Check PKCS padding and update data length based on padding info.
         * This can be used to detect invalid padding data and password
         * mismatches. */
        size_t unpadded_len;
        ret = pem_check_pkcs_padding(buf, len, &unpadded_len, diagnostics);
        if (ret != 0) {
            mbedtls_zeroize_and_free(buf, len);
            MINTLS_RETURN(ret);
        }
        len = unpadded_len;
#else
        mbedtls_zeroize_and_free(buf, len);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PEM_FEATURE_UNAVAILABLE);
#endif /* PEM_RFC1421 */
    }

    ctx->buf = buf;
    ctx->buflen = len;

    MINTLS_RETURN(0);
}

void mbedtls_pem_free(mbedtls_pem_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->buf != NULL) {
        mbedtls_zeroize_and_free(ctx->buf, ctx->buflen);
    }
    mbedtls_free(ctx->info);

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_pem_context));
}

