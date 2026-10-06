// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Generic SSL/TLS messaging layer functions
 *  (record layer + retransmission state machine)
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_msg.c.
 */
/*
 *  http://www.ietf.org/rfc/rfc2246.txt
 *  http://www.ietf.org/rfc/rfc4346.txt
 */

#include "common.h"

#include "platform.h"

#include "ssl.h"
#include "ssl_misc.h"
#include "debug_internal.h"
#include "ssl_debug_helpers.h"
#include "error.h"
#include "platform_util.h"
#include "version.h"
#include "constant_time_internal.h"
#include "constant_time.h"

#include <limits.h>
#include <string.h>

#include "oid.h"

uint32_t ssl_get_hs_total_len(mbedtls_ssl_context const *ssl);

/*
 * Start a timer.
 * Passing millisecs = 0 cancels a running timer.
 */
void mbedtls_ssl_set_timer(mbedtls_ssl_context *ssl, uint32_t millisecs)
{
    if (ssl->f_set_timer == NULL) {
        return;
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("set_timer to %d ms", (int) millisecs));
    ssl->f_set_timer(ssl->p_timer, millisecs / 4, millisecs);
}

/*
 * Return -1 is timer is expired, 0 if it isn't.
 */
int mbedtls_ssl_check_timer(mbedtls_ssl_context *ssl)
{
    if (ssl->f_get_timer == NULL) {
        return 0;
    }

    if (ssl->f_get_timer(ssl->p_timer) == 2) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("timer expired"));
        return -1;
    }

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_record_header(mbedtls_ssl_context const *ssl,
                                   unsigned char *buf,
                                   size_t len,
                                   mbedtls_record *rec, MinTlsDiagnostics* diagnostics);

int mbedtls_ssl_check_record(mbedtls_ssl_context const *ssl,
                             unsigned char *buf,
                             size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    MBEDTLS_SSL_DEBUG_MSG(3, ("=> mbedtls_ssl_check_record"));
    MBEDTLS_SSL_DEBUG_BUF(3, "record buffer", buf, buflen);

    /* We don't support record checking in TLS because
     * there doesn't seem to be a usecase for it.
     */
    if (ssl->conf->transport == MBEDTLS_SSL_TRANSPORT_STREAM) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
        goto exit;
    }

exit:
    /* On success, we have decrypted the buffer in-place, so make
     * sure we don't leak any plaintext data. */
    mbedtls_platform_zeroize(buf, buflen);

    /* For the purpose of this API, treat messages with unexpected CID
     * as well as such from future epochs as unexpected. */
    if (ret == MBEDTLS_ERR_SSL_UNEXPECTED_CID ||
        ret == MBEDTLS_ERR_SSL_EARLY_MESSAGE) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_UNEXPECTED_RECORD);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("<= mbedtls_ssl_check_record"));
    MINTLS_RETURN(ret);
}

#define SSL_DONT_FORCE_FLUSH 0
#define SSL_FORCE_FLUSH      1

/*
 * Encryption/decryption functions
 */

/* The size of the `add_data` structure depends on various
 * factors, namely
 *
 * 1) CID functionality disabled
 *
 * additional_data =
 *    8:                    seq_num +
 *    1:                       type +
 *    2:                    version +
 *    2:  length of inner plaintext +
 *
 * size = 13 bytes
 *
 * 2) CID functionality based on RFC 9146 enabled
 *
 * size = 8 + 1 + 1 + 1 + 2 + 2 + 6 + 2 + CID-length
 *      = 23 + CID-length
 *
 * 3) CID functionality based on legacy CID version
    according to draft-ietf-tls-dtls-connection-id-05
 *  https://tools.ietf.org/html/draft-ietf-tls-dtls-connection-id-05
 *
 * size = 13 + 1 + CID-length
 *
 * More information about the CID usage:
 *
 * Per Section 5.3 of draft-ietf-tls-dtls-connection-id-05 the
 * size of the additional data structure is calculated as:
 *
 * additional_data =
 *    8:                    seq_num +
 *    1:                  tls12_cid +
 *    2:     DTLSCipherText.version +
 *    n:                        cid +
 *    1:                 cid_length +
 *    2: length_of_DTLSInnerPlaintext
 *
 * Per RFC 9146 the size of the add_data structure is calculated as:
 *
 * additional_data =
 *    8:        seq_num_placeholder +
 *    1:                  tls12_cid +
 *    1:                 cid_length +
 *    1:                  tls12_cid +
 *    2:     DTLSCiphertext.version +
 *    2:                      epoch +
 *    6:            sequence_number +
 *    n:                        cid +
 *    2: length_of_DTLSInnerPlaintext
 *
 */
void ssl_extract_add_data_from_record(unsigned char *add_data,
                                             size_t *add_data_len,
                                             mbedtls_record *rec,
                                             mbedtls_ssl_protocol_version
                                             tls_version,
                                             size_t taglen)
{
    /* Several types of ciphers have been defined for use with TLS and DTLS,
     * and the MAC calculations for those ciphers differ slightly. Further
     * variants were added when the CID functionality was added with RFC 9146.
     * This implementations also considers the use of a legacy version of the
     * CID specification published in draft-ietf-tls-dtls-connection-id-05,
     * which is used in deployments.
     *
     * We will distinguish between the non-CID and the CID cases below.
     *
     * --- Non-CID cases ---
     *
     * Quoting RFC 5246 (TLS 1.2):
     *
     *    additional_data = seq_num + TLSCompressed.type +
     *                      TLSCompressed.version + TLSCompressed.length;
     *
     * For TLS 1.3, the record sequence number is dropped from the AAD
     * and encoded within the nonce of the AEAD operation instead.
     * Moreover, the additional data involves the length of the TLS
     * ciphertext, not the TLS plaintext as in earlier versions.
     * Quoting RFC 8446 (TLS 1.3):
     *
     *      additional_data = TLSCiphertext.opaque_type ||
     *                        TLSCiphertext.legacy_record_version ||
     *                        TLSCiphertext.length
     *
     * We pass the tag length to this function in order to compute the
     * ciphertext length from the inner plaintext length rec->data_len via
     *
     *     TLSCiphertext.length = TLSInnerPlaintext.length + taglen.
     *
     * --- CID cases ---
     *
     * RFC 9146 uses a common pattern when constructing the data
     * passed into a MAC / AEAD cipher.
     *
     * Data concatenation for MACs used with block ciphers with
     * Encrypt-then-MAC Processing (with CID):
     *
     *  data = seq_num_placeholder +
     *         tls12_cid +
     *         cid_length +
     *         tls12_cid +
     *         DTLSCiphertext.version +
     *         epoch +
     *         sequence_number +
     *         cid +
     *         DTLSCiphertext.length +
     *         IV +
     *         ENC(content + padding + padding_length)
     *
     * Data concatenation for MACs used with block ciphers (with CID):
     *
     *  data =  seq_num_placeholder +
     *          tls12_cid +
     *          cid_length +
     *          tls12_cid +
     *          DTLSCiphertext.version +
     *          epoch +
     *          sequence_number +
     *          cid +
     *          length_of_DTLSInnerPlaintext +
     *          DTLSInnerPlaintext.content +
     *          DTLSInnerPlaintext.real_type +
     *          DTLSInnerPlaintext.zeros
     *
     * AEAD ciphers use the following additional data calculation (with CIDs):
     *
     *     additional_data = seq_num_placeholder +
     *                tls12_cid +
     *                cid_length +
     *                tls12_cid +
     *                DTLSCiphertext.version +
     *                epoch +
     *                sequence_number +
     *                cid +
     *                length_of_DTLSInnerPlaintext
     *
     * Section 5.3 of draft-ietf-tls-dtls-connection-id-05 (for legacy CID use)
     * defines the additional data calculation as follows:
     *
     *     additional_data = seq_num +
     *                tls12_cid +
     *                DTLSCipherText.version +
     *                cid +
     *                cid_length +
     *                length_of_DTLSInnerPlaintext
     */

    unsigned char *cur = add_data;
    size_t ad_len_field = rec->data_len;

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID) && \
    MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 0
    const unsigned char seq_num_placeholder[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
#endif

    {
        ((void) tls_version);
        ((void) taglen);

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID) && \
        MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 0
        if (rec->cid_len != 0) {
            // seq_num_placeholder
            memcpy(cur, seq_num_placeholder, sizeof(seq_num_placeholder));
            cur += sizeof(seq_num_placeholder);

            // tls12_cid type
            *cur = rec->type;
            cur++;

            // cid_length
            *cur = rec->cid_len;
            cur++;
        } else
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID */
        {
            // epoch + sequence number
            memcpy(cur, rec->ctr, sizeof(rec->ctr));
            cur += sizeof(rec->ctr);
        }
    }

    // type
    *cur = rec->type;
    cur++;

    // version
    memcpy(cur, rec->ver, sizeof(rec->ver));
    cur += sizeof(rec->ver);

#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID) && \
    MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 1

    if (rec->cid_len != 0) {
        // CID
        memcpy(cur, rec->cid, rec->cid_len);
        cur += rec->cid_len;

        // cid_length
        *cur = rec->cid_len;
        cur++;

        // length of inner plaintext
        MBEDTLS_PUT_UINT16_BE(ad_len_field, cur, 0);
        cur += 2;
    } else
#elif defined(MBEDTLS_SSL_DTLS_CONNECTION_ID) && \
    MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 0

    if (rec->cid_len != 0) {
        // epoch + sequence number
        memcpy(cur, rec->ctr, sizeof(rec->ctr));
        cur += sizeof(rec->ctr);

        // CID
        memcpy(cur, rec->cid, rec->cid_len);
        cur += rec->cid_len;

        // length of inner plaintext
        MBEDTLS_PUT_UINT16_BE(ad_len_field, cur, 0);
        cur += 2;
    } else
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID */
    {
        MBEDTLS_PUT_UINT16_BE(ad_len_field, cur, 0);
        cur += 2;
    }

    *add_data_len = (size_t) (cur - add_data);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_transform_aead_dynamic_iv_is_explicit(
    mbedtls_ssl_transform const *transform)
{
    return transform->ivlen != transform->fixed_ivlen;
}

/* Compute IV := ( fixed_iv || 0 ) XOR ( 0 || dynamic_IV )
 *
 * Concretely, this occurs in two variants:
 *
 * a) Fixed and dynamic IV lengths add up to total IV length, giving
 *       IV = fixed_iv || dynamic_iv
 *
 *    This variant is used in TLS 1.2 when used with GCM or CCM.
 *
 * b) Fixed IV lengths matches total IV length, giving
 *       IV = fixed_iv XOR ( 0 || dynamic_iv )
 *
 *    This variant occurs in TLS 1.3 and for TLS 1.2 when using ChaChaPoly.
 *
 * See also the documentation of mbedtls_ssl_transform.
 *
 * This function has the precondition that
 *
 *     dst_iv_len >= max( fixed_iv_len, dynamic_iv_len )
 *
 * which has to be ensured by the caller. If this precondition
 * violated, the behavior of this function is undefined.
 */
void ssl_build_record_nonce(unsigned char *dst_iv,
                                   size_t dst_iv_len,
                                   unsigned char const *fixed_iv,
                                   size_t fixed_iv_len,
                                   unsigned char const *dynamic_iv,
                                   size_t dynamic_iv_len)
{
    /* Start with Fixed IV || 0 */
    memset(dst_iv, 0, dst_iv_len);
    memcpy(dst_iv, fixed_iv, fixed_iv_len);

    dst_iv += dst_iv_len - dynamic_iv_len;
    mbedtls_xor(dst_iv, dst_iv, dynamic_iv, dynamic_iv_len);
}

int mbedtls_ssl_encrypt_buf(mbedtls_ssl_context *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_ssl_mode_t ssl_mode;
    int auth_done = 0;
    unsigned char *data;
    /* For an explanation of the additional data length see
     * the description of ssl_extract_add_data_from_record().
     */
    unsigned char add_data[13];
    size_t add_data_len;
    size_t post_avail;

    /* The SSL context is only used for debugging purposes! */
    ssl = NULL; /* make sure we don't use it except for debug */
    ((void) ssl);

    /* The PRNG is used for dynamic IV generation that's used
     * for CBC transformations in TLS 1.2. */
    ((void) f_rng);
    ((void) p_rng);

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> encrypt buf"));

    if (transform == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("no transform provided to encrypt_buf"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }
    if (rec == NULL
        || rec->buf == NULL
        || rec->buf_len < rec->data_offset
        || rec->buf_len - rec->data_offset < rec->data_len
        ) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad record structure provided to encrypt_buf"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    ssl_mode = mbedtls_ssl_get_mode_from_transform(transform);

    data = rec->buf + rec->data_offset;
    post_avail = rec->buf_len - (rec->data_len + rec->data_offset);
    MBEDTLS_SSL_DEBUG_BUF(4, "before encrypt: output payload",
                          data, rec->data_len);

    if (rec->data_len > MBEDTLS_SSL_OUT_CONTENT_LEN) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Record content %" MBEDTLS_PRINTF_SIZET
                                  " too large, maximum %" MBEDTLS_PRINTF_SIZET,
                                  rec->data_len,
                                  (size_t) MBEDTLS_SSL_OUT_CONTENT_LEN));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    /* The following two code paths implement the (D)TLSInnerPlaintext
     * structure present in TLS 1.3 and DTLS 1.2 + CID.
     *
     * See ssl_build_inner_plaintext() for more information.
     *
     * Note that this changes `rec->data_len`, and hence
     * `post_avail` needs to be recalculated afterwards.
     *
     * Note also that the two code paths cannot occur simultaneously
     * since they apply to different versions of the protocol. There
     * is hence no risk of double-addition of the inner plaintext.
     */

    post_avail = rec->buf_len - (rec->data_len + rec->data_offset);

    /*
     * Add MAC before if needed
     */

    /*
     * Encrypt
     */

    if (ssl_mode == MBEDTLS_SSL_MODE_AEAD) {
        unsigned char iv[12];
        unsigned char *dynamic_iv;
        size_t dynamic_iv_len;
        int dynamic_iv_is_explicit =
            ssl_transform_aead_dynamic_iv_is_explicit(transform);
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        /* Check that there's space for the authentication tag. */
        if (post_avail < transform->taglen) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("Buffer provided for encrypted record not large enough"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL);
        }

        /*
         * Build nonce for AEAD encryption.
         *
         * Note: In the case of CCM and GCM in TLS 1.2, the dynamic
         *       part of the IV is prepended to the ciphertext and
         *       can be chosen freely - in particular, it need not
         *       agree with the record sequence number.
         *       However, since ChaChaPoly as well as all AEAD modes
         *       in TLS 1.3 use the record sequence number as the
         *       dynamic part of the nonce, we uniformly use the
         *       record sequence number here in all cases.
         */
        dynamic_iv     = rec->ctr;
        dynamic_iv_len = sizeof(rec->ctr);

        ssl_build_record_nonce(iv, sizeof(iv),
                               transform->iv_enc,
                               transform->fixed_ivlen,
                               dynamic_iv,
                               dynamic_iv_len);

        /*
         * Build additional data for AEAD encryption.
         * This depends on the TLS version.
         */
        ssl_extract_add_data_from_record(add_data, &add_data_len, rec,
                                         transform->tls_version,
                                         transform->taglen);

        MBEDTLS_SSL_DEBUG_BUF(4, "IV used (internal)",
                              iv, transform->ivlen);
        MBEDTLS_SSL_DEBUG_BUF(4, "IV used (transmitted)",
                              dynamic_iv,
                              dynamic_iv_is_explicit ? dynamic_iv_len : 0);
        MBEDTLS_SSL_DEBUG_BUF(4, "additional data used for AEAD",
                              add_data, add_data_len);
        MBEDTLS_SSL_DEBUG_MSG(3, ("before encrypt: msglen = %" MBEDTLS_PRINTF_SIZET ", "
                                                                                    "including 0 bytes of padding",
                                  rec->data_len));

        /*
         * Encrypt and authenticate
         */
        if ((ret = mbedtls_cipher_auth_encrypt_ext(&transform->cipher_ctx_enc,
                                                   iv, transform->ivlen,
                                                   add_data, add_data_len,
                                                   data, rec->data_len, /* src */
                                                   data, rec->buf_len - (size_t) (data - rec->buf), /* dst */
                                                   &rec->data_len,
                                                   transform->taglen, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_auth_encrypt_ext", ret);
            MINTLS_RETURN(ret);
        }

        MBEDTLS_SSL_DEBUG_BUF(4, "after encrypt: tag",
                              data + rec->data_len - transform->taglen,
                              transform->taglen);
        /* Account for authentication tag. */
        post_avail -= transform->taglen;

        /*
         * Prefix record content with dynamic IV in case it is explicit.
         */
        if (dynamic_iv_is_explicit != 0) {
            if (rec->data_offset < dynamic_iv_len) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("Buffer provided for encrypted record not large enough"));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL);
            }

            memcpy(data - dynamic_iv_len, dynamic_iv, dynamic_iv_len);
            rec->data_offset -= dynamic_iv_len;
            rec->data_len    += dynamic_iv_len;
        }

        auth_done++;
    } else
    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    /* Make extra sure authentication was performed, exactly once */
    if (auth_done != 1) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= encrypt buf"));

    MINTLS_RETURN(0);
}

int mbedtls_ssl_decrypt_buf(mbedtls_ssl_context const *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t olen;
    mbedtls_ssl_mode_t ssl_mode;
    int ret;

    int auth_done = 0;
    unsigned char *data;
    /* For an explanation of the additional data length see
     * the description of ssl_extract_add_data_from_record().
     */
    unsigned char add_data[13];
    size_t add_data_len;

    ssl = NULL; /* make sure we don't use it except for debug */
    ((void) ssl);

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> decrypt buf"));
    if (rec == NULL                     ||
        rec->buf == NULL                ||
        rec->buf_len < rec->data_offset ||
        rec->buf_len - rec->data_offset < rec->data_len) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad record structure provided to decrypt_buf"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    data = rec->buf + rec->data_offset;
    ssl_mode = mbedtls_ssl_get_mode_from_transform(transform);

    if (ssl_mode == MBEDTLS_SSL_MODE_AEAD) {
        unsigned char iv[12];
        unsigned char *dynamic_iv;
        size_t dynamic_iv_len;

        /*
         * Extract dynamic part of nonce for AEAD decryption.
         *
         * Note: In the case of CCM and GCM in TLS 1.2, the dynamic
         *       part of the IV is prepended to the ciphertext and
         *       can be chosen freely - in particular, it need not
         *       agree with the record sequence number.
         */
        dynamic_iv_len = sizeof(rec->ctr);
        if (ssl_transform_aead_dynamic_iv_is_explicit(transform) == 1) {
            if (rec->data_len < dynamic_iv_len) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("msglen (%" MBEDTLS_PRINTF_SIZET
                                          " ) < explicit_iv_len (%" MBEDTLS_PRINTF_SIZET ") ",
                                          rec->data_len,
                                          dynamic_iv_len));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_MAC);
            }
            dynamic_iv = data;

            data += dynamic_iv_len;
            rec->data_offset += dynamic_iv_len;
            rec->data_len    -= dynamic_iv_len;
        } else {
            dynamic_iv = rec->ctr;
        }

        /* Check that there's space for the authentication tag. */
        if (rec->data_len < transform->taglen) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("msglen (%" MBEDTLS_PRINTF_SIZET
                                      ") < taglen (%" MBEDTLS_PRINTF_SIZET ") ",
                                      rec->data_len,
                                      transform->taglen));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_MAC);
        }
        rec->data_len -= transform->taglen;

        /*
         * Prepare nonce from dynamic and static parts.
         */
        ssl_build_record_nonce(iv, sizeof(iv),
                               transform->iv_dec,
                               transform->fixed_ivlen,
                               dynamic_iv,
                               dynamic_iv_len);

        /*
         * Build additional data for AEAD encryption.
         * This depends on the TLS version.
         */
        ssl_extract_add_data_from_record(add_data, &add_data_len, rec,
                                         transform->tls_version,
                                         transform->taglen);
        MBEDTLS_SSL_DEBUG_BUF(4, "additional data used for AEAD",
                              add_data, add_data_len);

        /* Because of the check above, we know that there are
         * explicit_iv_len Bytes preceding data, and taglen
         * bytes following data + data_len. This justifies
         * the debug message and the invocation of
         * mbedtls_cipher_auth_decrypt_ext() below. */

        MBEDTLS_SSL_DEBUG_BUF(4, "IV used", iv, transform->ivlen);
        MBEDTLS_SSL_DEBUG_BUF(4, "TAG used", data + rec->data_len,
                              transform->taglen);

        /*
         * Decrypt and authenticate
         */
        if ((ret = mbedtls_cipher_auth_decrypt_ext
                       (&transform->cipher_ctx_dec,
                       iv, transform->ivlen,
                       add_data, add_data_len,
                       data, rec->data_len + transform->taglen, /* src */
                       data, rec->buf_len - (size_t) (data - rec->buf), &olen, /* dst */
                       transform->taglen, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_auth_decrypt_ext", ret);

            if (ret == MBEDTLS_ERR_CIPHER_AUTH_FAILED) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_MAC);
            }

            MINTLS_RETURN(ret);
        }

        auth_done++;

        /* Double-check that AEAD decryption doesn't change content length. */
        if (olen != rec->data_len) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }
    } else
    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

#if defined(MBEDTLS_SSL_DEBUG_ALL)
    MBEDTLS_SSL_DEBUG_BUF(4, "raw buffer after decryption",
                          data, rec->data_len);
#endif

    /*
     * Authenticate if not done yet.
     * Compute the MAC regardless of the padding result (RFC4346, CBCTIME).
     */

    /* Make extra sure authentication was performed, exactly once */
    if (auth_done != 1) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= decrypt buf"));

    MINTLS_RETURN(0);
}

#undef MAC_NONE
#undef MAC_PLAINTEXT
#undef MAC_CIPHERTEXT

/*
 * Fill the input message buffer by appending data to it.
 * The amount of data already fetched is in ssl->in_left.
 *
 * If we return 0, is it guaranteed that (at least) nb_want bytes are
 * available (from this read and/or a previous one). Otherwise, an error code
 * is returned (possibly EOF or WANT_READ).
 *
 * With stream transport (TLS) on success ssl->in_left == nb_want, but
 * with datagram transport (DTLS) on success ssl->in_left >= nb_want,
 * since we always read a whole datagram at once.
 *
 * For DTLS, it is up to the caller to set ssl->next_record_offset when
 * they're done reading a record.
 */
int mbedtls_ssl_fetch_input(mbedtls_ssl_context *ssl, size_t nb_want, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;
    size_t in_buf_len = MBEDTLS_SSL_IN_BUFFER_LEN;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> fetch input"));

    if (ssl->f_recv == NULL && ssl->f_recv_timeout == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Bad usage of mbedtls_ssl_set_bio() "));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (nb_want > in_buf_len - (size_t) (ssl->in_hdr - ssl->in_buf)) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("requesting more data than fits"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    {
        MBEDTLS_SSL_DEBUG_MSG(2, ("in_left: %" MBEDTLS_PRINTF_SIZET
                                  ", nb_want: %" MBEDTLS_PRINTF_SIZET,
                                  ssl->in_left, nb_want));

        while (ssl->in_left < nb_want) {
            len = nb_want - ssl->in_left;

            if (mbedtls_ssl_check_timer(ssl) != 0) {
                ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_TIMEOUT);
            } else {
                if (ssl->f_recv_timeout != NULL) {
                    ret = ssl->f_recv_timeout(ssl->p_bio,
                                              ssl->in_hdr + ssl->in_left, len,
                                              ssl->conf->read_timeout, diagnostics);
                } else {
                    ret = ssl->f_recv(ssl->p_bio,
                                      ssl->in_hdr + ssl->in_left, len, diagnostics);
                }
            }

            MBEDTLS_SSL_DEBUG_MSG(2, ("in_left: %" MBEDTLS_PRINTF_SIZET
                                      ", nb_want: %" MBEDTLS_PRINTF_SIZET,
                                      ssl->in_left, nb_want));
            MBEDTLS_SSL_DEBUG_RET(2, "ssl->f_recv(_timeout)", ret);

            if (ret == 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_CONN_EOF);
            }

            if (ret < 0) {
                MINTLS_RETURN(ret);
            }

            if ((size_t) ret > len) {
                MBEDTLS_SSL_DEBUG_MSG(1,
                                      ("f_recv returned %d bytes but only %" MBEDTLS_PRINTF_SIZET
                                       " were requested",
                                       ret, len));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
            }

            ssl->in_left += ret;
        }
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= fetch input"));

    MINTLS_RETURN(0);
}

/*
 * Flush any data not yet written
 */
int mbedtls_ssl_flush_output(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *buf;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> flush output"));

    if (ssl->f_send == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Bad usage of mbedtls_ssl_set_bio() "));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    /* Avoid incrementing counter if data is flushed */
    if (ssl->out_left == 0) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= flush output"));
        MINTLS_RETURN(0);
    }

    while (ssl->out_left > 0) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("message length: %" MBEDTLS_PRINTF_SIZET
                                  ", out_left: %" MBEDTLS_PRINTF_SIZET,
                                  mbedtls_ssl_out_hdr_len(ssl) + ssl->out_msglen, ssl->out_left));

        buf = ssl->out_hdr - ssl->out_left;
        ret = ssl->f_send(ssl->p_bio, buf, ssl->out_left, diagnostics);

        MBEDTLS_SSL_DEBUG_RET(2, "ssl->f_send", ret);

        if (ret <= 0) {
            MINTLS_RETURN(ret);
        }

        if ((size_t) ret > ssl->out_left) {
            MBEDTLS_SSL_DEBUG_MSG(1,
                                  ("f_send returned %d bytes but only %" MBEDTLS_PRINTF_SIZET
                                   " bytes were sent",
                                   ret, ssl->out_left));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }

        ssl->out_left -= ret;
    }

    {
        ssl->out_hdr = ssl->out_buf + 8;
    }
    mbedtls_ssl_update_out_pointers(ssl, ssl->transform_out);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= flush output"));

    MINTLS_RETURN(0);
}

/*
 * Functions to handle the DTLS retransmission state machine
 */

/*
 * Handshake layer functions
 */
int mbedtls_ssl_start_handshake_msg(mbedtls_ssl_context *ssl, unsigned char hs_type,
                                    unsigned char **buf, size_t *buf_len)
{
    /*
     * Reserve 4 bytes for handshake header. ( Section 4,RFC 8446 )
     *    ...
     *    HandshakeType msg_type;
     *    uint24 length;
     *    ...
     */
    *buf = ssl->out_msg + 4;
    *buf_len = MBEDTLS_SSL_OUT_CONTENT_LEN - 4;

    ssl->out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = hs_type;

    return 0;
}

/*
 * Write (DTLS: or queue) current handshake (including CCS) message.
 *
 *  - fill in handshake headers
 *  - update handshake checksum
 *  - DTLS: save message for resending
 *  - then pass to the record layer
 *
 * DTLS: except for HelloRequest, messages are only queued, and will only be
 * actually sent when calling flight_transmit() or resend().
 *
 * Inputs:
 *  - ssl->out_msglen: 4 + actual handshake message len
 *      (4 is the size of handshake headers for TLS)
 *  - ssl->out_msg[0]: the handshake type (ClientHello, ServerHello, etc)
 *  - ssl->out_msg + 4: the handshake message body
 *
 * Outputs, ie state before passing to flight_append() or write_record():
 *   - ssl->out_msglen: the length of the record contents
 *      (including handshake headers but excluding record headers)
 *   - ssl->out_msg: the record contents (handshake headers + content)
 */
int mbedtls_ssl_write_handshake_msg_ext(mbedtls_ssl_context *ssl,
                                        int update_checksum,
                                        int force_flush, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const size_t hs_len = ssl->out_msglen - 4;
    const unsigned char hs_type = ssl->out_msg[0];

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write handshake message"));

    /*
     * Sanity checks
     */
    if (ssl->out_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE          &&
        ssl->out_msgtype != MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    /* Whenever we send anything different from a
     * HelloRequest we should be in a handshake - double check. */
    if (!(ssl->out_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE &&
          hs_type          == MBEDTLS_SSL_HS_HELLO_REQUEST) &&
        ssl->handshake == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    /* Double-check that we did not exceed the bounds
     * of the outgoing record buffer.
     * This should never fail as the various message
     * writing functions must obey the bounds of the
     * outgoing record buffer, but better be safe.
     *
     * Note: We deliberately do not check for the MTU or MFL here.
     */
    if (ssl->out_msglen > MBEDTLS_SSL_OUT_CONTENT_LEN) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Record too large: "
                                  "size %" MBEDTLS_PRINTF_SIZET
                                  ", maximum %" MBEDTLS_PRINTF_SIZET,
                                  ssl->out_msglen,
                                  (size_t) MBEDTLS_SSL_OUT_CONTENT_LEN));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    /*
     * Fill handshake headers
     */
    if (ssl->out_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE) {
        ssl->out_msg[1] = MBEDTLS_BYTE_2(hs_len);
        ssl->out_msg[2] = MBEDTLS_BYTE_1(hs_len);
        ssl->out_msg[3] = MBEDTLS_BYTE_0(hs_len);

        /*
         * DTLS has additional fields in the Handshake layer,
         * between the length field and the actual payload:
         *      uint16 message_seq;
         *      uint24 fragment_offset;
         *      uint24 fragment_length;
         */

        /* Update running hashes of handshake messages seen */
        if (hs_type != MBEDTLS_SSL_HS_HELLO_REQUEST && update_checksum != 0) {
            ret = ssl->handshake->update_checksum(ssl, ssl->out_msg,
                                                  ssl->out_msglen, diagnostics);
            if (ret != 0) {
                MBEDTLS_SSL_DEBUG_RET(1, "update_checksum", ret);
                MINTLS_RETURN(ret);
            }
        }
    }

    /* Either send now, or just save to be sent (and resent) later */
    {
        if ((ret = mbedtls_ssl_write_record(ssl, force_flush, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "ssl_write_record", ret);
            MINTLS_RETURN(ret);
        }
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write handshake message"));

    MINTLS_RETURN(0);
}

int mbedtls_ssl_finish_handshake_msg(mbedtls_ssl_context *ssl,
                                     size_t buf_len, size_t msg_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t msg_with_header_len;
    ((void) buf_len);

    /* Add reserved 4 bytes for handshake header */
    msg_with_header_len = msg_len + 4;
    ssl->out_msglen = msg_with_header_len;
    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_write_handshake_msg_ext(ssl, 0, 0, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Record layer functions
 */

/*
 * Write current record.
 *
 * Uses:
 *  - ssl->out_msgtype: type of the message (AppData, Handshake, Alert, CCS)
 *  - ssl->out_msglen: length of the record content (excl headers)
 *  - ssl->out_msg: record content
 */
int mbedtls_ssl_write_record(mbedtls_ssl_context *ssl, int force_flush, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, done = 0;
    size_t len = ssl->out_msglen;
    int flush = force_flush;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write record"));

    if (!done) {
        unsigned i;
        size_t protected_record_size;
        size_t out_buf_len = MBEDTLS_SSL_OUT_BUFFER_LEN;
        /* Skip writing the record content type to after the encryption,
         * as it may change when using the CID extension. */
        mbedtls_ssl_protocol_version tls_ver = ssl->tls_version;
        mbedtls_ssl_write_version(ssl->out_hdr + 1, ssl->conf->transport,
                                  tls_ver);

        memcpy(ssl->out_ctr, ssl->cur_out_ctr, MBEDTLS_SSL_SEQUENCE_NUMBER_LEN);
        MBEDTLS_PUT_UINT16_BE(len, ssl->out_len, 0);

        if (ssl->transform_out != NULL) {
            mbedtls_record rec;

            rec.buf         = ssl->out_iv;
            rec.buf_len     = out_buf_len - (size_t) (ssl->out_iv - ssl->out_buf);
            rec.data_len    = ssl->out_msglen;
            rec.data_offset = (size_t) (ssl->out_msg - rec.buf);

            memcpy(&rec.ctr[0], ssl->out_ctr, sizeof(rec.ctr));
            mbedtls_ssl_write_version(rec.ver, ssl->conf->transport, tls_ver);
            rec.type = ssl->out_msgtype;

            if ((ret = mbedtls_ssl_encrypt_buf(ssl, ssl->transform_out, &rec,
                                               ssl->conf->f_rng, ssl->conf->p_rng, diagnostics)) != 0) {
                MBEDTLS_SSL_DEBUG_RET(1, "ssl_encrypt_buf", ret);
                MINTLS_RETURN(ret);
            }

            if (rec.data_offset != 0) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
            }

            /* Update the record content type and CID. */
            ssl->out_msgtype = rec.type;
            ssl->out_msglen = len = rec.data_len;
            MBEDTLS_PUT_UINT16_BE(rec.data_len, ssl->out_len, 0);
        }

        protected_record_size = len + mbedtls_ssl_out_hdr_len(ssl);

        /* Now write the potentially updated record content type. */
        ssl->out_hdr[0] = (unsigned char) ssl->out_msgtype;

        MBEDTLS_SSL_DEBUG_MSG(3, ("output record: msgtype = %u, "
                                  "version = [%u:%u], msglen = %" MBEDTLS_PRINTF_SIZET,
                                  ssl->out_hdr[0], ssl->out_hdr[1],
                                  ssl->out_hdr[2], len));

        MBEDTLS_SSL_DEBUG_BUF(4, "output record sent to network",
                              ssl->out_hdr, protected_record_size);

        ssl->out_left += protected_record_size;
        ssl->out_hdr  += protected_record_size;
        mbedtls_ssl_update_out_pointers(ssl, ssl->transform_out);

        for (i = 8; i > mbedtls_ssl_ep_len(ssl); i--) {
            if (++ssl->cur_out_ctr[i - 1] != 0) {
                break;
            }
        }

        /* The loop goes to its end if the counter is wrapping */
        if (i == mbedtls_ssl_ep_len(ssl)) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("outgoing message counter would wrap"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_COUNTER_WRAPPING);
        }
    }

    if ((flush == SSL_FORCE_FLUSH) &&
        (ret = mbedtls_ssl_flush_output(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_flush_output", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write record"));

    MINTLS_RETURN(0);
}

uint32_t ssl_get_hs_total_len(mbedtls_ssl_context const *ssl)
{
    return MBEDTLS_GET_UINT24_BE(ssl->in_msg, 1);
}

int mbedtls_ssl_prepare_handshake_record(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* Set handshake record length if this is the first fragment.
     * Do it unconditionally for DTLS, for which handshake fragmentation
     * is handled completely differently. */
    if (mbedtls_ssl_get_in_hsfraglen(ssl) == 0) {
        /* The handshake message must at least include the header.
         * We may not have the full message yet in case of fragmentation.
         * To simplify the code, we insist on having the header (and in
         * particular the handshake message length) in the first
         * fragment. */
        if (ssl->in_msglen < mbedtls_ssl_hs_hdr_len(ssl)) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("handshake message too short: %" MBEDTLS_PRINTF_SIZET,
                                      ssl->in_msglen));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
        }

        ssl->in_hslen = mbedtls_ssl_hs_hdr_len(ssl) + ssl_get_hs_total_len(ssl);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("handshake message: msglen ="
                              " %" MBEDTLS_PRINTF_SIZET ", type = %u, hslen = %"
                              MBEDTLS_PRINTF_SIZET,
                              ssl->in_msglen, ssl->in_msg[0], ssl->in_hslen));

    if (ssl->transform_in != NULL) {
        MBEDTLS_SSL_DEBUG_MSG(4, ("decrypted handshake message:"
                                  " iv-buf=%d hdr-buf=%d hdr-buf=%d",
                                  (int) (ssl->in_iv - ssl->in_buf),
                                  (int) (ssl->in_hdr - ssl->in_buf),
                                  (int) (ssl->in_msg - ssl->in_buf)));
    }

    {
        unsigned char *const reassembled_record_start =
            ssl->in_buf + MBEDTLS_SSL_SEQUENCE_NUMBER_LEN;
        unsigned char *const payload_start =
            reassembled_record_start + mbedtls_ssl_in_hdr_len(ssl);
        unsigned in_hsfraglen = mbedtls_ssl_get_in_hsfraglen(ssl);
        unsigned char *payload_end = payload_start + in_hsfraglen;
        /* How many more bytes we want to have a complete handshake message. */
        const size_t hs_remain = ssl->in_hslen - in_hsfraglen;
        /* How many bytes of the current record are part of the first
         * handshake message. There may be more handshake messages (possibly
         * incomplete) in the same record; if so, we leave them after the
         * current record, and ssl_consume_current_message() will take
         * care of consuming the next handshake message. */
        const size_t hs_this_fragment_len =
            ssl->in_msglen > hs_remain ? hs_remain : ssl->in_msglen;
        (void) hs_this_fragment_len;

        MBEDTLS_SSL_DEBUG_MSG(3,
                              ("%s handshake fragment: %" MBEDTLS_PRINTF_SIZET
                               ", %u..%u of %" MBEDTLS_PRINTF_SIZET,
                               (in_hsfraglen != 0 ? "subsequent" :
                                hs_this_fragment_len == ssl->in_hslen ? "sole" :
                                "initial"),
                               ssl->in_msglen,
                               in_hsfraglen,
                               in_hsfraglen + (unsigned) hs_this_fragment_len,
                               ssl->in_hslen));

        /* Move the received handshake fragment to have the whole message
         * (at least the part received so far) in a single segment at a
         * known offset in the input buffer.
         * - When receiving a non-initial handshake fragment, append it to
         *   the initial segment.
         * - Even the initial handshake fragment is moved, if it was
         *   encrypted with an explicit IV: decryption leaves the payload
         *   after the explicit IV, but here we move it to start where the
         *   IV was.
         */
        size_t const in_buf_len = MBEDTLS_SSL_IN_BUFFER_LEN;
        if (payload_end + ssl->in_msglen > ssl->in_buf + in_buf_len) {
            MBEDTLS_SSL_DEBUG_MSG(1,
                                  ("Shouldn't happen: no room to move handshake fragment %"
                                   MBEDTLS_PRINTF_SIZET " from %p to %p (buf=%p len=%"
                                   MBEDTLS_PRINTF_SIZET ")",
                                   ssl->in_msglen,
                                   (void *) ssl->in_msg, (void *) payload_end,
                                   (void *) ssl->in_buf, in_buf_len));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
        }
        memmove(payload_end, ssl->in_msg, ssl->in_msglen);

        in_hsfraglen += (unsigned) ssl->in_msglen;
        payload_end += ssl->in_msglen;

        if (in_hsfraglen < ssl->in_hslen) {
            MBEDTLS_SSL_DEBUG_MSG(3, ("Prepare: waiting for more handshake fragments "
                                      "%u/%" MBEDTLS_PRINTF_SIZET,
                                      in_hsfraglen, ssl->in_hslen));
            ssl->in_hdr = payload_end;
            ssl->in_msglen = 0;
            if (mbedtls_ssl_set_in_hsfraglen(ssl, in_hsfraglen) != 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
            }
            mbedtls_ssl_update_in_pointers(ssl);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_CONTINUE_PROCESSING);
        } else {
            ssl->in_msglen = in_hsfraglen;
            if (mbedtls_ssl_set_in_hsfraglen(ssl, 0) != 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
            }
            ssl->in_hdr = reassembled_record_start;
            mbedtls_ssl_update_in_pointers(ssl);

            /* Update the record length in the fully reassembled record */
            if (ssl->in_msglen > 0xffff) {
                MBEDTLS_SSL_DEBUG_MSG(1,
                                      ("Shouldn't happen: in_msglen=%"
                                       MBEDTLS_PRINTF_SIZET " > 0xffff",
                                       ssl->in_msglen));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
            }
            MBEDTLS_PUT_UINT16_BE(ssl->in_msglen, ssl->in_len, 0);

            size_t record_len = mbedtls_ssl_in_hdr_len(ssl) + ssl->in_msglen;
            (void) record_len;
            MBEDTLS_SSL_DEBUG_BUF(4, "reassembled record",
                                  ssl->in_hdr, record_len);
            if (ssl->in_hslen < ssl->in_msglen) {
                MBEDTLS_SSL_DEBUG_MSG(3,
                                      ("More handshake messages in the record: "
                                       "%" MBEDTLS_PRINTF_SIZET " + %" MBEDTLS_PRINTF_SIZET,
                                       ssl->in_hslen,
                                       ssl->in_msglen - ssl->in_hslen));
            }
        }
    }

    MINTLS_RETURN(0);
}

int mbedtls_ssl_update_handshake_status(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ssl_handshake_params * const hs = ssl->handshake;

    if (mbedtls_ssl_is_handshake_over(ssl) == 0 && hs != NULL) {
        ret = ssl->handshake->update_checksum(ssl, ssl->in_msg, ssl->in_hslen, diagnostics);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "update_checksum", ret);
            MINTLS_RETURN(ret);
        }
    }

    /* Handshake message is complete, increment counter */
    MINTLS_RETURN(0);
}

/*
 * DTLS anti-replay: RFC 6347 4.1.2.6
 *
 * in_window is a field of bits numbered from 0 (lsb) to 63 (msb).
 * Bit n is set iff record number in_window_top - n has been seen.
 *
 * Usually, in_window_top is the last record number seen and the lsb of
 * in_window is set. The only exception is the initial state (record number 0
 * not seen yet).
 */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_check_record_type(uint8_t record_type, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (record_type != MBEDTLS_SSL_MSG_HANDSHAKE &&
        record_type != MBEDTLS_SSL_MSG_ALERT &&
        record_type != MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC &&
        record_type != MBEDTLS_SSL_MSG_APPLICATION_DATA) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
    }

    MINTLS_RETURN(0);
}

/*
 * ContentType type;
 * ProtocolVersion version;
 * uint16 epoch;            // DTLS only
 * uint48 sequence_number;  // DTLS only
 * uint16 length;
 *
 * Return 0 if header looks sane (and, for DTLS, the record is expected)
 * MBEDTLS_ERR_SSL_INVALID_RECORD if the header looks bad,
 * MBEDTLS_ERR_SSL_UNEXPECTED_RECORD (DTLS only) if sane but unexpected.
 *
 * With DTLS, mbedtls_ssl_read_record() will:
 * 1. proceed with the record if this function returns 0
 * 2. drop only the current record if this function returns UNEXPECTED_RECORD
 * 3. return CLIENT_RECONNECT if this function return that value
 * 4. drop the whole datagram if this function returns anything else.
 * Point 2 is needed when the peer is resending, and we have already received
 * the first record from a datagram but are still waiting for the others.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_record_header(mbedtls_ssl_context const *ssl,
                                   unsigned char *buf,
                                   size_t len,
                                   mbedtls_record *rec, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_ssl_protocol_version tls_version;

    size_t const rec_hdr_type_offset    = 0;
    size_t const rec_hdr_type_len       = 1;

    size_t const rec_hdr_version_offset = rec_hdr_type_offset +
                                          rec_hdr_type_len;
    size_t const rec_hdr_version_len    = 2;

    size_t const rec_hdr_ctr_len        = 8;

    size_t       rec_hdr_len_offset; /* To be determined */
    size_t const rec_hdr_len_len    = 2;

    /*
     * Check minimum lengths for record header.
     */

    {
        rec_hdr_len_offset = rec_hdr_version_offset + rec_hdr_version_len;
    }

    if (len < rec_hdr_len_offset + rec_hdr_len_len) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              (
                                  "datagram of length %u too small to hold DTLS record header of length %u",
                                  (unsigned) len,
                                  (unsigned) (rec_hdr_len_offset + rec_hdr_len_len)));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
    }

    /*
     * Parse and validate record content type
     */

    rec->type = buf[rec_hdr_type_offset];

    /* Check record content type */
    {
        if (ssl_check_record_type(rec->type, diagnostics)) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("unknown record type %u",
                                      (unsigned) rec->type));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
        }
    }

    /*
     * Parse and validate record version
     */
    rec->ver[0] = buf[rec_hdr_version_offset + 0];
    rec->ver[1] = buf[rec_hdr_version_offset + 1];
    tls_version = (mbedtls_ssl_protocol_version) mbedtls_ssl_read_version(
        buf + rec_hdr_version_offset,
        ssl->conf->transport);

    if (tls_version > ssl->conf->max_tls_version) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("TLS version mismatch: got %u, expected max %u",
                                  (unsigned) tls_version,
                                  (unsigned) ssl->conf->max_tls_version));

        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
    }
    /*
     * Parse/Copy record sequence number.
     */

    {
        /* Copy implicit record sequence number from SSL context structure. */
        memcpy(&rec->ctr[0], ssl->in_ctr, rec_hdr_ctr_len);
    }

    /*
     * Parse record length.
     */

    rec->data_offset = rec_hdr_len_offset + rec_hdr_len_len;
    rec->data_len    = MBEDTLS_GET_UINT16_BE(buf, rec_hdr_len_offset);
    MBEDTLS_SSL_DEBUG_BUF(4, "input record header", buf, rec->data_offset);

    MBEDTLS_SSL_DEBUG_MSG(3, ("input record: msgtype = %u, "
                              "version = [0x%x], msglen = %" MBEDTLS_PRINTF_SIZET,
                              rec->type, (unsigned) tls_version, rec->data_len));

    rec->buf     = buf;
    rec->buf_len = rec->data_offset + rec->data_len;

    if (rec->data_len == 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("rejecting empty record"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
    }

    /*
     * DTLS-related tests.
     * Check epoch before checking length constraint because
     * the latter varies with the epoch. E.g., if a ChangeCipherSpec
     * message gets duplicated before the corresponding Finished message,
     * the second ChangeCipherSpec should be discarded because it belongs
     * to an old epoch, but not because its length is shorter than
     * the minimum record length for packets using the new record transform.
     * Note that these two kinds of failures are handled differently,
     * as an unexpected record is silently skipped but an invalid
     * record leads to the entire datagram being dropped.
     */

    MINTLS_RETURN(0);
}

/*
 * If applicable, decrypt record content
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_prepare_record_content(mbedtls_ssl_context *ssl,
                                      mbedtls_record *rec, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, done = 0;

    MBEDTLS_SSL_DEBUG_BUF(4, "input record from network",
                          rec->buf, rec->buf_len);

    /*
     * In TLS 1.3, always treat ChangeCipherSpec records
     * as unencrypted. The only thing we do with them is
     * check the length and content and ignore them.
     */

    if (!done && ssl->transform_in != NULL) {
        unsigned char const old_msg_type = rec->type;

        if ((ret = mbedtls_ssl_decrypt_buf(ssl, ssl->transform_in,
                                           rec, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "ssl_decrypt_buf", ret);

            /*
             * The decryption of the record failed, no reason to ignore it,
             * return in error with the decryption error code.
             */
            MINTLS_RETURN(ret);
        }

        if (old_msg_type != rec->type) {
            MBEDTLS_SSL_DEBUG_MSG(4, ("record type after decrypt (before %d): %d",
                                      old_msg_type, rec->type));
        }

        MBEDTLS_SSL_DEBUG_BUF(4, "input payload after decrypt",
                              rec->buf + rec->data_offset, rec->data_len);

        if (rec->data_len == 0) {
            if (ssl->tls_version == MBEDTLS_SSL_VERSION_TLS1_2
                && rec->type != MBEDTLS_SSL_MSG_APPLICATION_DATA) {
                /* TLS v1.2 explicitly disallows zero-length messages which are not application data */
                MBEDTLS_SSL_DEBUG_MSG(1, ("invalid zero-length message type: %d", ssl->in_msgtype));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
            }

            ssl->nb_zero++;

            /*
             * Three or more empty messages may be a DoS attack
             * (excessive CPU consumption).
             */
            if (ssl->nb_zero > 3) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("received four consecutive empty "
                                          "messages, possible DoS attack"));
                /* Treat the records as if they were not properly authenticated,
                 * thereby failing the connection if we see more than allowed
                 * by the configured bad MAC threshold. */
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_MAC);
            }
        } else {
            ssl->nb_zero = 0;
        }

        {
            unsigned i;
            for (i = MBEDTLS_SSL_SEQUENCE_NUMBER_LEN;
                 i > mbedtls_ssl_ep_len(ssl); i--) {
                if (++ssl->in_ctr[i - 1] != 0) {
                    break;
                }
            }

            /* The loop goes to its end iff the counter is wrapping */
            if (i == mbedtls_ssl_ep_len(ssl)) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("incoming message counter would wrap"));
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_COUNTER_WRAPPING);
            }
        }

    }

    /* Check actual (decrypted) record content length against
     * configured maximum. */
    if (rec->data_len > MBEDTLS_SSL_IN_CONTENT_LEN) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad message length"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
    }

    MINTLS_RETURN(0);
}

/*
 * Read a record.
 *
 * Silently ignore non-fatal alert (and for DTLS, invalid records as well,
 * RFC 6347 4.1.2.7) and continue reading until a valid record is found.
 *
 */

/* Helper functions for mbedtls_ssl_read_record(). */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_consume_current_message(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_get_next_record(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_record_is_in_progress(mbedtls_ssl_context *ssl);

int mbedtls_ssl_read_record(mbedtls_ssl_context *ssl,
                            unsigned update_hs_digest, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> read record"));

    if (ssl->keep_current_message == 0) {
        do {

            ret = ssl_consume_current_message(ssl, diagnostics);
            if (ret != 0) {
                MINTLS_RETURN(ret);
            }

            if (ssl_record_is_in_progress(ssl) == 0) {
                int dtls_have_buffered = 0;
                if (dtls_have_buffered == 0) {
                    ret = ssl_get_next_record(ssl, diagnostics);
                    if (ret == MBEDTLS_ERR_SSL_CONTINUE_PROCESSING) {
                        continue;
                    }

                    if (ret != 0) {
                        MBEDTLS_SSL_DEBUG_RET(1, ("ssl_get_next_record"), ret);
                        MINTLS_RETURN(ret);
                    }
                }
            }

            ret = mbedtls_ssl_handle_message_type(ssl, diagnostics);

        } while (MBEDTLS_ERR_SSL_NON_FATAL           == ret  ||
                 MBEDTLS_ERR_SSL_CONTINUE_PROCESSING == ret);

        if (0 != ret) {
            MBEDTLS_SSL_DEBUG_RET(1, ("mbedtls_ssl_handle_message_type"), ret);
            MINTLS_RETURN(ret);
        }

        if (ssl->in_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE &&
            update_hs_digest == 1) {
            ret = mbedtls_ssl_update_handshake_status(ssl, diagnostics);
            if (0 != ret) {
                MBEDTLS_SSL_DEBUG_RET(1, ("mbedtls_ssl_update_handshake_status"), ret);
                MINTLS_RETURN(ret);
            }
        }
    } else {
        MBEDTLS_SSL_DEBUG_MSG(2, ("reuse previously read message"));
        ssl->keep_current_message = 0;
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= read record"));

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_consume_current_message(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /*
     * Consume last content-layer message and potentially
     * update in_msglen which keeps track of the contents'
     * consumption state.
     *
     * (1) Handshake messages:
     *     Remove last handshake message, move content
     *     and adapt in_msglen.
     *
     * (2) Alert messages:
     *     Consume whole record content, in_msglen = 0.
     *
     * (3) Change cipher spec:
     *     Consume whole record content, in_msglen = 0.
     *
     * (4) Application data:
     *     Don't do anything - the record layer provides
     *     the application data as a stream transport
     *     and consumes through mbedtls_ssl_read only.
     *
     */

    /* Case (1): Handshake messages */
    if (ssl->in_hslen != 0) {
        /* Hard assertion to be sure that no application data
         * is in flight, as corrupting ssl->in_msglen during
         * ssl->in_offt != NULL is fatal. */
        if (ssl->in_offt != NULL) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }

        if (mbedtls_ssl_get_in_hsfraglen(ssl) != 0) {
            /* Not all handshake fragments have arrived, do not consume. */
            MBEDTLS_SSL_DEBUG_MSG(3, ("Consume: waiting for more handshake fragments "
                                      "%u/%" MBEDTLS_PRINTF_SIZET,
                                      mbedtls_ssl_get_in_hsfraglen(ssl), ssl->in_hslen));
            MINTLS_RETURN(0);
        }

        /*
         * Get next Handshake message in the current record
         */

        /* Notes:
         * (1) in_hslen is not necessarily the size of the
         *     current handshake content: If DTLS handshake
         *     fragmentation is used, that's the fragment
         *     size instead. Using the total handshake message
         *     size here is faulty and should be changed at
         *     some point.
         * (2) While it doesn't seem to cause problems, one
         *     has to be very careful not to assume that in_hslen
         *     is always <= in_msglen in a sensible communication.
         *     Again, it's wrong for DTLS handshake fragmentation.
         *     The following check is therefore mandatory, and
         *     should not be treated as a silently corrected assertion.
         *     Additionally, ssl->in_hslen might be arbitrarily out of
         *     bounds after handling a DTLS message with an unexpected
         *     sequence number, see mbedtls_ssl_prepare_handshake_record.
         */
        if (ssl->in_hslen < ssl->in_msglen) {
            ssl->in_msglen -= ssl->in_hslen;
            memmove(ssl->in_msg, ssl->in_msg + ssl->in_hslen,
                    ssl->in_msglen);
            MBEDTLS_PUT_UINT16_BE(ssl->in_msglen, ssl->in_len, 0);

            MBEDTLS_SSL_DEBUG_BUF(4, "remaining content in record",
                                  ssl->in_msg, ssl->in_msglen);
        } else {
            ssl->in_msglen = 0;
        }

        ssl->in_hslen   = 0;
    }
    /* Case (4): Application data */
    else if (ssl->in_offt != NULL) {
        MINTLS_RETURN(0);
    }
    /* Everything else (CCS & Alerts) */
    else {
        ssl->in_msglen = 0;
    }

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_record_is_in_progress(mbedtls_ssl_context *ssl)
{
    if (ssl->in_msglen > 0) {
        return 1;
    }

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_get_next_record(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_record rec;

    /* Ensure that we have enough space available for the default form
     * of TLS / DTLS record headers (5 Bytes for TLS, 13 Bytes for DTLS,
     * with no space for CIDs counted in). */
    ret = mbedtls_ssl_fetch_input(ssl, mbedtls_ssl_in_hdr_len(ssl), diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_fetch_input", ret);
        MINTLS_RETURN(ret);
    }

    ret = ssl_parse_record_header(ssl, ssl->in_hdr, ssl->in_left, &rec, diagnostics);
    if (ret != 0) {
        {
            MINTLS_RETURN(ret);
        }
    }

    {
        /*
         * Fetch record contents from underlying transport.
         */
        ret = mbedtls_ssl_fetch_input(ssl, rec.buf_len, diagnostics);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_fetch_input", ret);
            MINTLS_RETURN(ret);
        }

        ssl->in_left = 0;
    }

    /*
     * Decrypt record contents.
     */

    if ((ret = ssl_prepare_record_content(ssl, &rec, diagnostics)) != 0) {
        {
            /* Error out (and send alert) on invalid records */
#if defined(MBEDTLS_SSL_ALL_ALERT_MESSAGES)
            if (ret == MBEDTLS_ERR_SSL_INVALID_MAC) {
                mbedtls_ssl_send_alert_message(ssl,
                                               MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                               MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC, diagnostics);
            }
#endif
            MINTLS_RETURN(ret);
        }
    }

    /* Reset in pointers to default state for TLS/DTLS records,
     * assuming no CID and no offset between record content and
     * record plaintext. */
    mbedtls_ssl_update_in_pointers(ssl);
    ssl->in_iv  = ssl->in_len + 2;

    /* The record content type may change during decryption,
     * so re-read it. */
    ssl->in_msgtype = rec.type;

    ssl->in_msg    = rec.buf + rec.data_offset;
    ssl->in_msglen = rec.data_len;

    MINTLS_RETURN(0);
}

int mbedtls_ssl_handle_message_type(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* If we're in the middle of a fragmented TLS handshake message,
     * we don't accept any other message type. For TLS 1.3, the spec forbids
     * interleaving other message types between handshake fragments. For TLS
     * 1.2, the spec does not forbid it but we do. */
    if (mbedtls_ssl_get_in_hsfraglen(ssl) != 0 &&
        ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("non-handshake message in the middle"
                                  " of a fragmented handshake message"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    /*
     * Handle particular types of records
     */
    if (ssl->in_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE) {
        if ((ret = mbedtls_ssl_prepare_handshake_record(ssl, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }
    }

    if (ssl->in_msgtype == MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC) {
        if (ssl->in_msglen != 1) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("invalid CCS message, len: %" MBEDTLS_PRINTF_SIZET,
                                      ssl->in_msglen));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
        }

        if (ssl->in_msg[0] != 1) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("invalid CCS message, content: %02x",
                                      ssl->in_msg[0]));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
        }

    }

    if (ssl->in_msgtype == MBEDTLS_SSL_MSG_ALERT) {
        if (ssl->in_msglen != 2) {
            /* Note: Standard allows for more than one 2 byte alert
               to be packed in a single message, but Mbed TLS doesn't
               currently support this. */
            MBEDTLS_SSL_DEBUG_MSG(1, ("invalid alert message, len: %" MBEDTLS_PRINTF_SIZET,
                                      ssl->in_msglen));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INVALID_RECORD);
        }

        MBEDTLS_SSL_DEBUG_MSG(2, ("got an alert message, type: [%u:%u]",
                                  ssl->in_msg[0], ssl->in_msg[1]));

        /*
         * Ignore non-fatal alerts, except close_notify and no_renegotiation
         */
        if (ssl->in_msg[0] == MBEDTLS_SSL_ALERT_LEVEL_FATAL) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("is a fatal alert message (msg %d)",
                                      ssl->in_msg[1]));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE);
        }

        if (ssl->in_msg[0] == MBEDTLS_SSL_ALERT_LEVEL_WARNING &&
            ssl->in_msg[1] == MBEDTLS_SSL_ALERT_MSG_CLOSE_NOTIFY) {
            MBEDTLS_SSL_DEBUG_MSG(2, ("is a close notify message"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY);
        }

#if defined(MBEDTLS_SSL_RENEGOTIATION_ENABLED)
        if (ssl->in_msg[0] == MBEDTLS_SSL_ALERT_LEVEL_WARNING &&
            ssl->in_msg[1] == MBEDTLS_SSL_ALERT_MSG_NO_RENEGOTIATION) {
            MBEDTLS_SSL_DEBUG_MSG(2, ("is a no renegotiation alert"));
            /* Will be handled when trying to parse ServerHello */
            MINTLS_RETURN(0);
        }
#endif
        /* Silently ignore: fetch new message */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_NON_FATAL);
    }

    MINTLS_RETURN(0);
}

int mbedtls_ssl_send_fatal_handshake_failure(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ssl_send_alert_message(ssl,
                                          MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                          MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics));
}

int mbedtls_ssl_send_alert_message(mbedtls_ssl_context *ssl,
                                   unsigned char level,
                                   unsigned char message, MinTlsDiagnostics* diagnostics)
{
    // A failed alert is secondary to the error that prompted it.
    MinTlsDiagnostics alertDiagnostics = {diagnostics->log, NULL};
    diagnostics = &alertDiagnostics;
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ssl == NULL || ssl->conf == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (ssl->out_left != 0) {
        MINTLS_RETURN(mbedtls_ssl_flush_output(ssl, diagnostics));
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> send alert message"));
    MBEDTLS_SSL_DEBUG_MSG(3, ("send alert level=%u message=%u", level, message));

    ssl->out_msgtype = MBEDTLS_SSL_MSG_ALERT;
    ssl->out_msglen = 2;
    ssl->out_msg[0] = level;
    ssl->out_msg[1] = message;

    if ((ret = mbedtls_ssl_write_record(ssl, SSL_FORCE_FLUSH, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_record", ret);
        MINTLS_RETURN(ret);
    }
    MBEDTLS_SSL_DEBUG_MSG(2, ("<= send alert message"));

    MINTLS_RETURN(0);
}

int mbedtls_ssl_write_change_cipher_spec(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write change cipher spec"));

    ssl->out_msgtype = MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC;
    ssl->out_msglen  = 1;
    ssl->out_msg[0]  = 1;

    mbedtls_ssl_handshake_increment_state(ssl);

    if ((ret = mbedtls_ssl_write_handshake_msg(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_handshake_msg", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write change cipher spec"));

    MINTLS_RETURN(0);
}

int mbedtls_ssl_parse_change_cipher_spec(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse change cipher spec"));

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        MINTLS_RETURN(ret);
    }

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad change cipher spec message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    /* CCS records are only accepted if they have length 1 and content '1',
     * so we don't need to check this here. */

    /*
     * Switch to our negotiated transform and session parameters for inbound
     * data.
     */
    MBEDTLS_SSL_DEBUG_MSG(3, ("switching to new transform spec for inbound data"));
    ssl->transform_in = ssl->transform_negotiate;
    ssl->session_in = ssl->session_negotiate;

    memset(ssl->in_ctr, 0, MBEDTLS_SSL_SEQUENCE_NUMBER_LEN);

    mbedtls_ssl_update_in_pointers(ssl);

    mbedtls_ssl_handshake_increment_state(ssl);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse change cipher spec"));

    MINTLS_RETURN(0);
}

/* Once ssl->out_hdr as the address of the beginning of the
 * next outgoing record is set, deduce the other pointers.
 *
 * Note: For TLS, we save the implicit record sequence number
 *       (entering MAC computation) in the 8 bytes before ssl->out_hdr,
 *       and the caller has to make sure there's space for this.
 */

size_t ssl_transform_get_explicit_iv_len(
    mbedtls_ssl_transform const *transform)
{
    return transform->ivlen - transform->fixed_ivlen;
}

void mbedtls_ssl_update_out_pointers(mbedtls_ssl_context *ssl,
                                     mbedtls_ssl_transform *transform)
{
    {
        ssl->out_len = ssl->out_hdr + 3;
        ssl->out_iv  = ssl->out_hdr + 5;
    }

    ssl->out_msg = ssl->out_iv;
    /* Adjust out_msg to make space for explicit IV, if used. */
    if (transform != NULL) {
        ssl->out_msg += ssl_transform_get_explicit_iv_len(transform);
    }
}

/* Once ssl->in_hdr as the address of the beginning of the
 * next incoming record is set, deduce the other pointers.
 *
 * Note: For TLS, we save the implicit record sequence number
 *       (entering MAC computation) in the 8 bytes before ssl->in_hdr,
 *       and the caller has to make sure there's space for this.
 */

void mbedtls_ssl_update_in_pointers(mbedtls_ssl_context *ssl)
{
    /* This function sets the pointers to match the case
     * of unprotected TLS/DTLS records, with both  ssl->in_iv
     * and ssl->in_msg pointing to the beginning of the record
     * content.
     *
     * When decrypting a protected record, ssl->in_msg
     * will be shifted to point to the beginning of the
     * record plaintext.
     */

    {
        ssl->in_ctr = ssl->in_buf;
        ssl->in_len = ssl->in_hdr + 3;
        ssl->in_iv  = ssl->in_hdr + 5;
    }

    /* This will be adjusted at record decryption time. */
    ssl->in_msg = ssl->in_iv;
}

/*
 * Setup an SSL context
 */

void mbedtls_ssl_reset_in_pointers(mbedtls_ssl_context *ssl)
{
    {
        ssl->in_hdr = ssl->in_buf + MBEDTLS_SSL_SEQUENCE_NUMBER_LEN;
    }

    /* Derive other internal pointers. */
    mbedtls_ssl_update_in_pointers(ssl);
}

void mbedtls_ssl_reset_out_pointers(mbedtls_ssl_context *ssl)
{
    /* Set the incoming and outgoing record pointers. */
    {
        ssl->out_ctr = ssl->out_buf;
        ssl->out_hdr = ssl->out_buf + MBEDTLS_SSL_SEQUENCE_NUMBER_LEN;
    }
    /* Derive other internal pointers. */
    mbedtls_ssl_update_out_pointers(ssl, NULL /* no transform enabled */);
}

/*
 * SSL get accessors
 */
size_t mbedtls_ssl_get_bytes_avail(const mbedtls_ssl_context *ssl)
{
    return ssl->in_offt == NULL ? 0 : ssl->in_msglen;
}

int mbedtls_ssl_check_pending(const mbedtls_ssl_context *ssl)
{
    /*
     * Case A: We're currently holding back
     * a message for further processing.
     */

    if (ssl->keep_current_message == 1) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("ssl_check_pending: record held back for processing"));
        return 1;
    }

    /*
     * Case B: Further records are pending in the current datagram.
     */

    /*
     * Case C: A handshake message is being processed.
     */

    if (ssl->in_hslen > 0 && ssl->in_hslen < ssl->in_msglen) {
        MBEDTLS_SSL_DEBUG_MSG(3,
                              ("ssl_check_pending: more handshake messages within current record"));
        return 1;
    }

    /*
     * Case D: An application data message is being processed
     */
    if (ssl->in_offt != NULL) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("ssl_check_pending: application data record is being processed"));
        return 1;
    }

    /*
     * In all other cases, the rest of the message can be dropped.
     * As in ssl_get_next_record, this needs to be adapted if
     * we implement support for multiple alerts in single records.
     */

    MBEDTLS_SSL_DEBUG_MSG(3, ("ssl_check_pending: nothing pending"));
    return 0;
}

int mbedtls_ssl_get_record_expansion(const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t transform_expansion = 0;
    const mbedtls_ssl_transform *transform = ssl->transform_out;
    unsigned block_size;

    size_t out_hdr_len = mbedtls_ssl_out_hdr_len(ssl);

    if (transform == NULL) {
        MINTLS_RETURN((int) out_hdr_len);
    }

    switch (mbedtls_cipher_get_cipher_mode(&transform->cipher_ctx_enc)) {
        case MBEDTLS_MODE_GCM:
        case MBEDTLS_MODE_CCM:
        case MBEDTLS_MODE_CHACHAPOLY:
        case MBEDTLS_MODE_STREAM:
            transform_expansion = transform->minlen;
            break;

        case MBEDTLS_MODE_CBC:

            block_size = mbedtls_cipher_get_block_size(
                &transform->cipher_ctx_enc);

            /* Expansion due to the addition of the MAC. */
            transform_expansion += transform->maclen;

            /* Expansion due to the addition of CBC padding;
             * Theoretically up to 256 bytes, but we never use
             * more than the block size of the underlying cipher. */
            transform_expansion += block_size;

            /* For TLS 1.2 or higher, an explicit IV is added
             * after the record header. */
            transform_expansion += block_size;

            break;

        default:
            MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MINTLS_RETURN((int) (out_hdr_len + transform_expansion));
}

/* This function is called from mbedtls_ssl_read() when a handshake message is
 * received after the initial handshake. In this context, handshake messages
 * may only be sent for the purpose of initiating renegotiations.
 *
 * This function is introduced as a separate helper since the handling
 * of post-handshake handshake messages changes significantly in TLS 1.3,
 * and having a helper function allows to distinguish between TLS <= 1.2 and
 * TLS 1.3 in the future without bloating the logic of mbedtls_ssl_read().
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_tls12_handle_hs_message_post_handshake(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /*
     * - For client-side, expect SERVER_HELLO_REQUEST.
     * - For server-side, expect CLIENT_HELLO.
     * - Fail (TLS) or silently drop record (DTLS) in other cases.
     */

    if (ssl->conf->endpoint == MBEDTLS_SSL_IS_CLIENT &&
        (ssl->in_msg[0] != MBEDTLS_SSL_HS_HELLO_REQUEST ||
         ssl->in_hslen  != mbedtls_ssl_hs_hdr_len(ssl))) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("handshake received (not HelloRequest)"));

        /* With DTLS, drop the packet (probably from last handshake) */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    {
        /*
         * Refuse renegotiation
         */

        MBEDTLS_SSL_DEBUG_MSG(3, ("refusing renegotiation, sending alert"));

        if ((ret = mbedtls_ssl_send_alert_message(ssl,
                                                  MBEDTLS_SSL_ALERT_LEVEL_WARNING,
                                                  MBEDTLS_SSL_ALERT_MSG_NO_RENEGOTIATION, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }
    }

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_handle_hs_message_post_handshake(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* Check protocol version and dispatch accordingly. */

    if (ssl->tls_version <= MBEDTLS_SSL_VERSION_TLS1_2) {
        MINTLS_RETURN(ssl_tls12_handle_hs_message_post_handshake(ssl, diagnostics));
    }

    /* Should never happen */
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}

/*
 * brief          Read at most 'len' application data bytes from the input
 *                buffer.
 *
 * param ssl      SSL context:
 *                - First byte of application data not read yet in the input
 *                  buffer located at address `in_offt`.
 *                - The number of bytes of data not read yet is `in_msglen`.
 * param buf      buffer that will hold the data
 * param len      maximum number of bytes to read
 *
 * note           The function updates the fields `in_offt` and `in_msglen`
 *                according to the number of bytes read.
 *
 * return         The number of bytes read.
 */
int ssl_read_application_data(
    mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
    size_t n = (len < ssl->in_msglen) ? len : ssl->in_msglen;

    if (len != 0) {
        memcpy(buf, ssl->in_offt, n);
        ssl->in_msglen -= n;
    }

    /* Zeroising the plaintext buffer to erase unused application data
       from the memory. */
    mbedtls_platform_zeroize(ssl->in_offt, n);

    if (ssl->in_msglen == 0) {
        /* all bytes consumed */
        ssl->in_offt = NULL;
        ssl->keep_current_message = 0;
    } else {
        /* more data available */
        ssl->in_offt += n;
    }

    return (int) n;
}

/*
 * Receive application data decrypted from the SSL layer
 */
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ssl == NULL || ssl->conf == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> read"));

    /*
     * Check if renegotiation is necessary and/or handshake is
     * in process. If yes, perform/continue, and fall through
     * if an unexpected packet is received while the client
     * is waiting for the ServerHello.
     *
     * (There is no equivalent to the last condition on
     *  the server-side as it is not treated as within
     *  a handshake while waiting for the ClientHello
     *  after a renegotiation request.)
     */

    if (ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        ret = mbedtls_ssl_handshake(ssl, diagnostics);
        if (ret != MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO &&
            ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_handshake", ret);
            MINTLS_RETURN(ret);
        }
    }

    /* Loop as long as no application data record is available */
    while (ssl->in_offt == NULL) {
        /* Start timer if not already running */
        if (ssl->f_get_timer != NULL &&
            ssl->f_get_timer(ssl->p_timer) == -1) {
            mbedtls_ssl_set_timer(ssl, ssl->conf->read_timeout);
        }

        if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
            if (ret == MBEDTLS_ERR_SSL_CONN_EOF) {
                MINTLS_RETURN(0);
            }

            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
            MINTLS_RETURN(ret);
        }

        if (ssl->in_msglen  == 0 &&
            ssl->in_msgtype == MBEDTLS_SSL_MSG_APPLICATION_DATA) {
            /*
             * OpenSSL sends empty messages to randomize the IV
             */
            if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
                if (ret == MBEDTLS_ERR_SSL_CONN_EOF) {
                    MINTLS_RETURN(0);
                }

                MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
                MINTLS_RETURN(ret);
            }
        }

        if (ssl->in_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE) {
            ret = ssl_handle_hs_message_post_handshake(ssl, diagnostics);
            if (ret != 0) {
                MBEDTLS_SSL_DEBUG_RET(1, "ssl_handle_hs_message_post_handshake",
                                      ret);
                MINTLS_RETURN(ret);
            }

            /* At this point, we don't know whether the renegotiation triggered
             * by the post-handshake message has been completed or not. The cases
             * to consider are the following:
             * 1) The renegotiation is complete. In this case, no new record
             *    has been read yet.
             * 2) The renegotiation is incomplete because the client received
             *    an application data record while awaiting the ServerHello.
             * 3) The renegotiation is incomplete because the client received
             *    a non-handshake, non-application data message while awaiting
             *    the ServerHello.
             *
             * In each of these cases, looping will be the proper action:
             * - For 1), the next iteration will read a new record and check
             *   if it's application data.
             * - For 2), the loop condition isn't satisfied as application data
             *   is present, hence continue is the same as break
             * - For 3), the loop condition is satisfied and read_record
             *   will re-deliver the message that was held back by the client
             *   when expecting the ServerHello.
             */

            continue;
        }

        /* Fatal and closure alerts handled by mbedtls_ssl_read_record() */
        if (ssl->in_msgtype == MBEDTLS_SSL_MSG_ALERT) {
            MBEDTLS_SSL_DEBUG_MSG(2, ("ignoring non-fatal non-closure alert"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_WANT_READ);
        }

        if (ssl->in_msgtype != MBEDTLS_SSL_MSG_APPLICATION_DATA) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad application data message"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
        }

        ssl->in_offt = ssl->in_msg;

        /* We're going to return something now, cancel timer,
         * except if handshake (renegotiation) is in progress */
        if (mbedtls_ssl_is_handshake_over(ssl) == 1) {
            mbedtls_ssl_set_timer(ssl, 0);
        }

    }

    ret = ssl_read_application_data(ssl, buf, len);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= read"));

    MINTLS_RETURN(ret);
}

/*
 * Send application data to be encrypted by the SSL layer, taking care of max
 * fragment length and buffer size.
 *
 * According to RFC 5246 Section 6.2.1:
 *
 *      Zero-length fragments of Application data MAY be sent as they are
 *      potentially useful as a traffic analysis countermeasure.
 *
 * Therefore, it is possible that the input message length is 0 and the
 * corresponding return code is 0 on success.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_real(mbedtls_ssl_context *ssl,
                          const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = mbedtls_ssl_get_max_out_record_payload(ssl);
    const size_t max_len = (size_t) ret;

    if (ret < 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_get_max_out_record_payload", ret);
        MINTLS_RETURN(ret);
    }

    if (len > max_len) {
        len = max_len;
    }

    if (ssl->out_left != 0) {
        /*
         * The user has previously tried to send the data and
         * MBEDTLS_ERR_SSL_WANT_WRITE or the message was only partially
         * written. In this case, we expect the high-level write function
         * (e.g. mbedtls_ssl_write()) to be called with the same parameters
         */
        if ((ret = mbedtls_ssl_flush_output(ssl, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_flush_output", ret);
            MINTLS_RETURN(ret);
        }
    } else {
        /*
         * The user is trying to send a message the first time, so we need to
         * copy the data into the internal buffers and setup the data structure
         * to keep track of partial writes
         */
        ssl->out_msglen  = len;
        ssl->out_msgtype = MBEDTLS_SSL_MSG_APPLICATION_DATA;
        if (len > 0) {
            memcpy(ssl->out_msg, buf, len);
        }

        if ((ret = mbedtls_ssl_write_record(ssl, SSL_FORCE_FLUSH, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_record", ret);
            MINTLS_RETURN(ret);
        }
    }

    MINTLS_RETURN((int) len);
}

/*
 * Write application data (public-facing wrapper)
 */
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write"));

    if (ssl == NULL || ssl->conf == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        if ((ret = mbedtls_ssl_handshake(ssl, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_handshake", ret);
            MINTLS_RETURN(ret);
        }
    }

    ret = ssl_write_real(ssl, buf, len, diagnostics);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write"));

    MINTLS_RETURN(ret);
}

/*
 * Notify the peer that the connection is being closed
 */
int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ssl == NULL || ssl->conf == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write close notify"));

    if (mbedtls_ssl_is_handshake_over(ssl) == 1) {
        if ((ret = mbedtls_ssl_send_alert_message(ssl,
                                                  MBEDTLS_SSL_ALERT_LEVEL_WARNING,
                                                  MBEDTLS_SSL_ALERT_MSG_CLOSE_NOTIFY, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_send_alert_message", ret);
            MINTLS_RETURN(ret);
        }
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write close notify"));

    MINTLS_RETURN(0);
}

void mbedtls_ssl_transform_free(mbedtls_ssl_transform *transform)
{
    if (transform == NULL) {
        return;
    }

    mbedtls_cipher_free(&transform->cipher_ctx_enc);
    mbedtls_cipher_free(&transform->cipher_ctx_dec);

    mbedtls_platform_zeroize(transform, sizeof(mbedtls_ssl_transform));
}

void mbedtls_ssl_set_inbound_transform(mbedtls_ssl_context *ssl,
                                       mbedtls_ssl_transform *transform)
{
    ssl->transform_in = transform;
    memset(ssl->in_ctr, 0, MBEDTLS_SSL_SEQUENCE_NUMBER_LEN);
}

void mbedtls_ssl_set_outbound_transform(mbedtls_ssl_context *ssl,
                                        mbedtls_ssl_transform *transform)
{
    ssl->transform_out = transform;
    memset(ssl->cur_out_ctr, 0, sizeof(ssl->cur_out_ctr));
}

/*
 * Convert version numbers to/from wire format
 * and, for DTLS, to/from TLS equivalent.
 *
 * For TLS this is the identity.
 * For DTLS, map as follows, then use 1's complement (v -> ~v):
 * 1.x <-> 3.x+1    for x != 0 (DTLS 1.2 based on TLS 1.2)
 *                  DTLS 1.0 is stored as TLS 1.1 internally
 */
void mbedtls_ssl_write_version(unsigned char version[2], int transport,
                               mbedtls_ssl_protocol_version tls_version)
{
    uint16_t tls_version_formatted;
    ((void) transport);
    {
        tls_version_formatted = (uint16_t) tls_version;
    }
    MBEDTLS_PUT_UINT16_BE(tls_version_formatted, version, 0);
}

uint16_t mbedtls_ssl_read_version(const unsigned char version[2],
                                  int transport)
{
    uint16_t tls_version = MBEDTLS_GET_UINT16_BE(version, 0);
    ((void) transport);
    return tls_version;
}

/*
 * Send pending fatal alert.
 * 0,   No alert message.
 * !0,  if mbedtls_ssl_send_alert_message() returned in error, the error code it
 *      returned, ssl->alert_reason otherwise.
 */
int mbedtls_ssl_handle_pending_alert(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    /* No pending alert, return success*/
    if (ssl->send_alert == 0) {
        MINTLS_RETURN(0);
    }

    ret = mbedtls_ssl_send_alert_message(ssl,
                                         MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                         ssl->alert_type, diagnostics);

    /* If mbedtls_ssl_send_alert_message() returned with MBEDTLS_ERR_SSL_WANT_WRITE,
     * do not clear the alert to be able to send it later.
     */
    if (ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        ssl->send_alert = 0;
    }

    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(ssl->alert_reason);
}

/*
 * Set pending fatal alert flag.
 */
void mbedtls_ssl_pend_fatal_alert(mbedtls_ssl_context *ssl,
                                  unsigned char alert_type,
                                  int alert_reason)
{
    ssl->send_alert = 1;
    ssl->alert_type = alert_type;
    ssl->alert_reason = alert_reason;
}
