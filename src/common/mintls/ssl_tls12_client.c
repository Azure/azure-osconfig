// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  TLS client-side functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_tls12_client.c.
 */

#include "common.h"

#include "platform.h"

#include "ssl.h"
#include "ssl_client.h"
#include "ssl_debug_helpers.h"
#include "ssl_misc.h"
#include "debug_internal.h"
#include "error.h"
#include "constant_time.h"

#include <string.h>

#include <stdint.h>

#include "platform_time.h"

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_supported_point_formats_ext(mbedtls_ssl_context *ssl,
                                                 unsigned char *buf,
                                                 const unsigned char *end,
                                                 size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = buf;
    (void) ssl; /* ssl used for debugging only */

    *olen = 0;

    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("client hello, adding supported_point_formats extension"));
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 6);

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS, p, 0);
    p += 2;

    *p++ = 0x00;
    *p++ = 2;

    *p++ = 1;
    *p++ = MBEDTLS_ECP_PF_UNCOMPRESSED;

    *olen = 6;

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_extended_ms_ext(mbedtls_ssl_context *ssl,
                                     unsigned char *buf,
                                     const unsigned char *end,
                                     size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = buf;

    *olen = 0;

    if (ssl->conf->extended_ms == MBEDTLS_SSL_EXTENDED_MS_DISABLED) {
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("client hello, adding extended_master_secret extension"));

    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 4);

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_EXTENDED_MASTER_SECRET, p, 0);
    p += 2;

    *p++ = 0x00;
    *p++ = 0x00;

    *olen = 4;

    MINTLS_RETURN(0);
}

int mbedtls_ssl_tls12_write_client_hello_exts(mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              const unsigned char *end,
                                              int uses_ec,
                                              size_t *out_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = buf;
    size_t ext_len = 0;

    (void) ssl;
    (void) end;
    (void) uses_ec;
    (void) ret;
    (void) ext_len;

    *out_len = 0;

    /* Note that TLS_EMPTY_RENEGOTIATION_INFO_SCSV is always added
     * even if MBEDTLS_SSL_RENEGOTIATION is not defined. */

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    if (uses_ec) {
        if ((ret = ssl_write_supported_point_formats_ext(ssl, p, end,
                                                         &ext_len, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "ssl_write_supported_point_formats_ext", ret);
            MINTLS_RETURN(ret);
        }
        p += ext_len;
    }
#endif

    if ((ret = ssl_write_extended_ms_ext(ssl, p, end, &ext_len, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "ssl_write_extended_ms_ext", ret);
        MINTLS_RETURN(ret);
    }
    p += ext_len;

    *out_len = (size_t) (p - buf);

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_renegotiation_info(mbedtls_ssl_context *ssl,
                                        const unsigned char *buf,
                                        size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    {
        if (len != 1 || buf[0] != 0x00) {
            MBEDTLS_SSL_DEBUG_MSG(1,
                                  ("non-zero length renegotiation info"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
        }

        ssl->secure_renegotiation = MBEDTLS_SSL_SECURE_RENEGOTIATION;
    }

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_extended_ms_ext(mbedtls_ssl_context *ssl,
                                     const unsigned char *buf,
                                     size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ssl->conf->extended_ms == MBEDTLS_SSL_EXTENDED_MS_DISABLED ||
        len != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("non-matching extended master secret extension"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION);
    }

    ((void) buf);

    ssl->handshake->extended_ms = MBEDTLS_SSL_EXTENDED_MS_ENABLED;

    MINTLS_RETURN(0);
}

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_supported_point_formats_ext(mbedtls_ssl_context *ssl,
                                                 const unsigned char *buf,
                                                 size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t list_size;
    const unsigned char *p;

    if (len == 0 || (size_t) (buf[0] + 1) != len) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }
    list_size = buf[0];

    p = buf + 1;
    while (list_size > 0) {
        if (p[0] == MBEDTLS_ECP_PF_UNCOMPRESSED ||
            p[0] == MBEDTLS_ECP_PF_COMPRESSED) {
#if !defined(MBEDTLS_USE_PSA_CRYPTO) && \
            defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED)
            ssl->handshake->ecdh_ctx.point_format = p[0];
#endif /* !MBEDTLS_USE_PSA_CRYPTO && MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED */
            MBEDTLS_SSL_DEBUG_MSG(4, ("point format selected: %d", p[0]));
            MINTLS_RETURN(0);
        }

        list_size--;
        p++;
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("no point format in common"));
    mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                   MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
}
#endif /* MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_alpn_ext(mbedtls_ssl_context *ssl,
                              const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t list_len, name_len;
    const char **p;

    /* If we didn't send it, the server shouldn't send it */
    if (ssl->conf->alpn_list == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("non-matching ALPN extension"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION);
    }

    /*
     * opaque ProtocolName<1..2^8-1>;
     *
     * struct {
     *     ProtocolName protocol_name_list<2..2^16-1>
     * } ProtocolNameList;
     *
     * the "ProtocolNameList" MUST contain exactly one "ProtocolName"
     */

    /* Min length is 2 (list_len) + 1 (name_len) + 1 (name) */
    if (len < 4) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    list_len = MBEDTLS_GET_UINT16_BE(buf, 0);
    if (list_len != len - 2) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    name_len = buf[2];
    if (name_len != list_len - 1) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    /* Check that the server chosen protocol was in our list and save it */
    for (p = ssl->conf->alpn_list; *p != NULL; p++) {
        if (name_len == strlen(*p) &&
            memcmp(buf + 3, *p, name_len) == 0) {
            ssl->alpn_chosen = *p;
            MINTLS_RETURN(0);
        }
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("ALPN extension: no matching protocol"));
    mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                   MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
}

/*
 * Parse HelloVerifyRequest.  Only called after verifying the HS type.
 */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_server_hello(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, i;
    size_t n;
    size_t ext_len;
    unsigned char *buf, *ext;
    unsigned char comp;
    int handshake_failure = 0;
    const mbedtls_ssl_ciphersuite_t *suite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse server hello"));

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        /* No alert on a read error. */
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        MINTLS_RETURN(ret);
    }

    buf = ssl->in_msg;

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {

        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    if (ssl->in_hslen < 38 + mbedtls_ssl_hs_hdr_len(ssl) ||
        buf[0] != MBEDTLS_SSL_HS_SERVER_HELLO) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    /*
     *  0   .  1    server_version
     *  2   . 33    random (maybe including 4 bytes of Unix time)
     * 34   . 34    session_id length = n
     * 35   . 34+n  session_id
     * 35+n . 36+n  cipher_suite
     * 37+n . 37+n  compression_method
     *
     * 38+n . 39+n  extensions length (optional)
     * 40+n .  ..   extensions
     */
    buf += mbedtls_ssl_hs_hdr_len(ssl);

    MBEDTLS_SSL_DEBUG_BUF(3, "server hello, version", buf, 2);
    ssl->tls_version = (mbedtls_ssl_protocol_version) mbedtls_ssl_read_version(buf,
                                                                               ssl->conf->transport);
    ssl->session_negotiate->tls_version = ssl->tls_version;
    ssl->session_negotiate->endpoint = ssl->conf->endpoint;

    if (ssl->tls_version < ssl->conf->min_tls_version ||
        ssl->tls_version > ssl->conf->max_tls_version) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              (
                                  "server version out of bounds -  min: [0x%x], server: [0x%x], max: [0x%x]",
                                  (unsigned) ssl->conf->min_tls_version,
                                  (unsigned) ssl->tls_version,
                                  (unsigned) ssl->conf->max_tls_version));

        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_PROTOCOL_VERSION, diagnostics);

        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("server hello, current time: %lu",
                              ((unsigned long) buf[2] << 24) |
                              ((unsigned long) buf[3] << 16) |
                              ((unsigned long) buf[4] <<  8) |
                              ((unsigned long) buf[5])));

    memcpy(ssl->handshake->randbytes + 32, buf + 2, 32);

    n = buf[34];

    MBEDTLS_SSL_DEBUG_BUF(3,   "server hello, random bytes", buf + 2, 32);

    if (n > 32) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    if (ssl->in_hslen > mbedtls_ssl_hs_hdr_len(ssl) + 39 + n) {
        ext_len = MBEDTLS_GET_UINT16_BE(buf, 38 + n);

        if ((ext_len > 0 && ext_len < 4) ||
            ssl->in_hslen != mbedtls_ssl_hs_hdr_len(ssl) + 40 + n + ext_len) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }
    } else if (ssl->in_hslen == mbedtls_ssl_hs_hdr_len(ssl) + 38 + n) {
        ext_len = 0;
    } else {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    /* ciphersuite (used later) */
    i = (int) MBEDTLS_GET_UINT16_BE(buf, n + 35);

    /*
     * Read and check compression
     */
    comp = buf[37 + n];

    if (comp != MBEDTLS_SSL_COMPRESS_NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("server hello, bad compression: %d", comp));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }

    /*
     * Initialize update checksum functions
     */
    ssl->handshake->ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(i);
    if (ssl->handshake->ciphersuite_info == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("ciphersuite info for %04x not found", (unsigned int) i));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    mbedtls_ssl_optimize_checksum(ssl, ssl->handshake->ciphersuite_info);

    MBEDTLS_SSL_DEBUG_MSG(3, ("server hello, session id len.: %" MBEDTLS_PRINTF_SIZET, n));
    MBEDTLS_SSL_DEBUG_BUF(3,   "server hello, session id", buf + 35, n);

    /*
     * Check if the session can be resumed
     */
    if (ssl->handshake->resume == 0 || n == 0 ||
        ssl->session_negotiate->ciphersuite != i ||
        ssl->session_negotiate->id_len != n ||
        memcmp(ssl->session_negotiate->id, buf + 35, n) != 0) {
        mbedtls_ssl_handshake_increment_state(ssl);
        ssl->handshake->resume = 0;
        ssl->session_negotiate->start = mbedtls_time(NULL);
        ssl->session_negotiate->ciphersuite = i;
        ssl->session_negotiate->id_len = n;
        memcpy(ssl->session_negotiate->id, buf + 35, n);
    } else {
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("%s session has been resumed",
                              ssl->handshake->resume ? "a" : "no"));

    MBEDTLS_SSL_DEBUG_MSG(3, ("server hello, chosen ciphersuite: %04x", (unsigned) i));
    MBEDTLS_SSL_DEBUG_MSG(3, ("server hello, compress alg.: %d",
                              buf[37 + n]));

    /*
     * Perform cipher suite validation in same way as in ssl_write_client_hello.
     */
    i = 0;
    while (1) {
        if (ssl->conf->ciphersuite_list[i] == 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        }

        if (ssl->conf->ciphersuite_list[i++] ==
            ssl->session_negotiate->ciphersuite) {
            break;
        }
    }

    suite_info = mbedtls_ssl_ciphersuite_from_id(
        ssl->session_negotiate->ciphersuite);
    if (mbedtls_ssl_validate_ciphersuite(ssl, suite_info, ssl->tls_version,
                                         ssl->tls_version) != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
    }

    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("server hello, chosen ciphersuite: %s", suite_info->name));

    if (comp != MBEDTLS_SSL_COMPRESS_NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
    }

    ext = buf + 40 + n;

    MBEDTLS_SSL_DEBUG_MSG(2,
                          ("server hello, total extension length: %" MBEDTLS_PRINTF_SIZET,
                           ext_len));

    while (ext_len) {
        unsigned int ext_id   = MBEDTLS_GET_UINT16_BE(ext, 0);
        unsigned int ext_size = MBEDTLS_GET_UINT16_BE(ext, 2);

        if (ext_size + 4 > ext_len) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
            mbedtls_ssl_send_alert_message(
                ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }

        switch (ext_id) {
            case MBEDTLS_TLS_EXT_RENEGOTIATION_INFO:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found renegotiation extension"));

                if ((ret = ssl_parse_renegotiation_info(ssl, ext + 4,
                                                        ext_size, diagnostics)) != 0) {
                    MINTLS_RETURN(ret);
                }

                break;

            case MBEDTLS_TLS_EXT_EXTENDED_MASTER_SECRET:
                MBEDTLS_SSL_DEBUG_MSG(3,
                                      ("found extended_master_secret extension"));

                if ((ret = ssl_parse_extended_ms_ext(ssl,
                                                     ext + 4, ext_size, diagnostics)) != 0) {
                    MINTLS_RETURN(ret);
                }

                break;

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED) || \
                defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED) || \
                defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
            case MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS:
                MBEDTLS_SSL_DEBUG_MSG(3,
                                      ("found supported_point_formats extension"));

                if ((ret = ssl_parse_supported_point_formats_ext(ssl,
                                                                 ext + 4, ext_size, diagnostics)) != 0) {
                    MINTLS_RETURN(ret);
                }

                break;
#endif /* MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED ||
          MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */

            case MBEDTLS_TLS_EXT_ALPN:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found alpn extension"));

                if ((ret = ssl_parse_alpn_ext(ssl, ext + 4, ext_size, diagnostics)) != 0) {
                    MINTLS_RETURN(ret);
                }

                break;

            default:
                MBEDTLS_SSL_DEBUG_MSG(3,
                                      ("unknown extension found: %u (ignoring)", ext_id));
        }

        ext_len -= 4 + ext_size;
        ext += 4 + ext_size;

        if (ext_len > 0 && ext_len < 4) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello message"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }
    }

    /*
     * mbedtls_ssl_derive_keys() has to be called after the parsing of the
     * extensions. It sets the transform data for the resumed session which in
     * case of DTLS includes the server CID extracted from the CID extension.
     */
    if (ssl->handshake->resume) {
        if ((ret = mbedtls_ssl_derive_keys(ssl, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_derive_keys", ret);
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR, diagnostics);
            MINTLS_RETURN(ret);
        }
    }

    /*
     * Renegotiation security checks
     */
    if (ssl->secure_renegotiation == MBEDTLS_SSL_LEGACY_RENEGOTIATION &&
        ssl->conf->allow_legacy_renegotiation ==
        MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("legacy renegotiation, breaking off handshake"));
        handshake_failure = 1;
    }

    if (handshake_failure == 1) {
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse server hello"));

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_check_server_ecdh_params(const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    uint16_t tls_id;
    mbedtls_ecp_group_id grp_id;
#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    grp_id = ssl->handshake->ecdh_ctx.grp.id;
#else
    grp_id = ssl->handshake->ecdh_ctx.grp_id;
#endif

    tls_id = mbedtls_ssl_get_tls_id_from_ecp_group_id(grp_id);
    if (tls_id == 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("ECDH curve: %s",
                              mbedtls_ssl_get_curve_name_from_tls_id(tls_id)));

    if (mbedtls_ssl_check_curve(ssl, grp_id, diagnostics) != 0) {
        MINTLS_RETURN_CAUSE(-1);
    }

    MBEDTLS_SSL_DEBUG_ECDH(3, &ssl->handshake->ecdh_ctx,
                           MBEDTLS_DEBUG_ECDH_QP);

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_server_ecdh_params(mbedtls_ssl_context *ssl,
                                        unsigned char **p,
                                        unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;

    /*
     * Ephemeral ECDH parameters:
     *
     * struct {
     *     ECParameters curve_params;
     *     ECPoint      public;
     * } ServerECDHParams;
     */
    if ((ret = mbedtls_ecdh_read_params(&ssl->handshake->ecdh_ctx,
                                        (const unsigned char **) p, end, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, ("mbedtls_ecdh_read_params"), ret);
        MINTLS_RETURN(ret);
    }

    if (ssl_check_server_ecdh_params(ssl, diagnostics) != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("bad server key exchange message (ECDHE curve)"));
        MINTLS_RETURN_CAUSE(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
    }

    MINTLS_RETURN(ret);
}

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED)
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_signature_algorithm(mbedtls_ssl_context *ssl,
                                         uint16_t sig_alg,
                                         mbedtls_md_type_t *md_alg,
                                         mbedtls_pk_type_t *pk_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    diagnosticFrame.positiveIsFailure = true;

    if (mbedtls_ssl_get_pk_type_and_md_alg_from_sig_alg(sig_alg, pk_alg, md_alg, diagnostics) != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("Server used unsupported %s signature algorithm",
                               mbedtls_ssl_sig_alg_to_str(sig_alg)));
        MINTLS_RETURN_CAUSE(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER);
    }

    /*
     * mbedtls_ssl_get_pk_type_and_md_alg_from_sig_alg() understands
     * signature algorithm code points from both TLS 1.2 and TLS 1.3. Make sure
     * that the selected signature algorithm is acceptable when TLS 1.2 is
     * negotiated.
     *
     * In TLS 1.2, RSA-PSS signature algorithms (rsa_pss_rsae_*) are not
     * defined by RFC 5246. However, RFC 8446 Section 4.2.3 requires that
     * implementations which advertise support for RSASSA-PSS must be
     * prepared to accept such signatures even when TLS 1.2 is negotiated,
     * provided they were offered in the signature_algorithms extension.
     *
     * Therefore, we allow rsa_pss_rsae_* here if:
     *  - the implementation supports them, and
     *  - they were offered in the signature_algorithms extension (checked by
     *    `mbedtls_ssl_sig_alg_is_offered()` below).
     *
     * If we were to add full support for rsa_pss_rsae_* signature algorithms
     * in TLS 1.2, we should then integrate RSA-PSS into the TLS 1.2 signature
     * algorithm support logic (`mbedtls_ssl_tls12_sig_alg_is_supported()`)
     * instead of handling it as a special case here.
     */
    if (!mbedtls_ssl_sig_alg_is_supported(ssl, sig_alg)) {
        switch (sig_alg) {
#if defined(PSA_WANT_ALG_RSA_PSS)
#if defined(PSA_WANT_ALG_SHA_256)
            case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256:
#endif
#if defined(PSA_WANT_ALG_SHA_384)
            case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384:
#endif
#if defined(PSA_WANT_ALG_SHA_512)
            case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512:
#endif
#if defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA_384) || defined(PSA_WANT_ALG_SHA_512)
        MBEDTLS_SSL_DEBUG_MSG(3,
                              (
                                  "Accepting TLS 1.2 RSA-PSS signature algorithm %s via compatibility exception",
                                  mbedtls_ssl_sig_alg_to_str(sig_alg)));
        break;
#endif
#endif /* PSA_WANT_ALG_RSA_PSS */
            default:
                MBEDTLS_SSL_DEBUG_MSG(1,
                                      ("Server used unsupported %s signature algorithm",
                                       mbedtls_ssl_sig_alg_to_str(sig_alg)));
                MINTLS_RETURN(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER);
        }
    }

    /*
     * Check if the signature algorithm is acceptable
     */
    if (!mbedtls_ssl_sig_alg_is_offered(ssl, sig_alg)) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("Server used the signature algorithm %s that was not offered",
                               mbedtls_ssl_sig_alg_to_str(sig_alg)));
        MINTLS_RETURN(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("Server used the signature algorithm %s",
                              mbedtls_ssl_sig_alg_to_str(sig_alg)));

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED) */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_server_key_exchange(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;
    unsigned char *p = NULL, *end = NULL;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse server key exchange"));

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        MINTLS_RETURN(ret);
    }

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server key exchange message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    /*
     * ServerKeyExchange may be skipped with PSK and RSA-PSK when the server
     * doesn't use a psk_identity_hint
     */
    if (ssl->in_msg[0] != MBEDTLS_SSL_HS_SERVER_KEY_EXCHANGE) {
        if (ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_PSK ||
            ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_RSA_PSK) {
            /* Current message is probably either
             * CertificateRequest or ServerHelloDone */
            ssl->keep_current_message = 1;
            goto exit;
        }

        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("server key exchange message must not be skipped"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);

        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    p   = ssl->in_msg + mbedtls_ssl_hs_hdr_len(ssl);
    end = ssl->in_msg + ssl->in_hslen;
    MBEDTLS_SSL_DEBUG_BUF(3,   "server key exchange", p, (size_t) (end - p));

    if (ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_RSA ||
        ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_PSK ||
        ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA) {
        if (ssl_parse_server_ecdh_params(ssl, &p, end, diagnostics) != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        }
    } else

    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED)
    if (mbedtls_ssl_ciphersuite_uses_server_signature(ciphersuite_info)) {
        size_t sig_len, hashlen;
        unsigned char hash[MBEDTLS_MD_MAX_SIZE];

        mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;
        mbedtls_pk_type_t pk_alg = MBEDTLS_PK_NONE;
        unsigned char *params = ssl->in_msg + mbedtls_ssl_hs_hdr_len(ssl);
        size_t params_len = (size_t) (p - params);
        void *rs_ctx = NULL;

        mbedtls_pk_context *peer_pk;

        if (ssl->session_negotiate->peer_cert == NULL) {
            /* Should never happen */
            MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }
        peer_pk = &ssl->session_negotiate->peer_cert->pk;

        /*
         * Handle the digitally-signed structure
         */
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
        uint16_t sig_alg = MBEDTLS_GET_UINT16_BE(p, 0);
        if (ssl_parse_signature_algorithm(ssl, sig_alg, &md_alg, &pk_alg, diagnostics) != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1,
                                  ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
            MINTLS_RETURN_CAUSE(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        }
        p += 2;

        if (!mbedtls_pk_can_do(peer_pk, pk_alg)) {
            MBEDTLS_SSL_DEBUG_MSG(1,
                                  ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        }

        /*
         * Read signature
         */

        if (p > end - 2) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }
        sig_len = MBEDTLS_GET_UINT16_BE(p, 0);
        p += 2;

        if (p != end - sig_len) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }

        MBEDTLS_SSL_DEBUG_BUF(3, "signature", p, sig_len);

        /*
         * Compute the hash that has been signed
         */
        if (md_alg != MBEDTLS_MD_NONE) {
            ret = mbedtls_ssl_get_key_exchange_md_tls1_2(ssl, hash, &hashlen,
                                                         params, params_len,
                                                         md_alg, diagnostics);
            if (ret != 0) {
                MINTLS_RETURN(ret);
            }
        } else {
            MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }

        MBEDTLS_SSL_DEBUG_BUF(3, "parameters hash", hash, hashlen);

        /*
         * Verify signature
         */
        if (!mbedtls_pk_can_do(peer_pk, pk_alg)) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad server key exchange message"));
            mbedtls_ssl_send_alert_message(
                ssl,
                MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH);
        }

        if (pk_alg == MBEDTLS_PK_RSASSA_PSS) {
            mbedtls_pk_rsassa_pss_options rsassa_pss_options;
            rsassa_pss_options.mgf1_hash_id = md_alg;
            rsassa_pss_options.expected_salt_len =
                mbedtls_md_get_size_from_type(md_alg);
            if (rsassa_pss_options.expected_salt_len == 0) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
            }

            ret = mbedtls_pk_verify_ext(pk_alg, &rsassa_pss_options,
                                        peer_pk,
                                        md_alg, hash, hashlen,
                                        p, sig_len, diagnostics);
        } else
        ret = mbedtls_pk_verify_restartable(peer_pk,
                                            md_alg, hash, hashlen, p, sig_len, rs_ctx, diagnostics);

        if (ret != 0) {
            int send_alert_msg = 1;
            if (send_alert_msg) {
                mbedtls_ssl_send_alert_message(
                    ssl,
                    MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                    MBEDTLS_SSL_ALERT_MSG_DECRYPT_ERROR, diagnostics);
            }
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_pk_verify", ret);
            MINTLS_RETURN(ret);
        }

    }
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_SERVER_SIGNATURE_ENABLED */

exit:
    mbedtls_ssl_handshake_increment_state(ssl);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse server key exchange"));

    MINTLS_RETURN(0);
}

#if !defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_certificate_request(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse certificate request"));

    if (!mbedtls_ssl_ciphersuite_cert_req_allowed(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip parse certificate request"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}
#else
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_certificate_request(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *buf;
    size_t n = 0;
    size_t cert_type_len = 0, dn_len = 0;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;
    size_t sig_alg_len;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse certificate request"));

    if (!mbedtls_ssl_ciphersuite_cert_req_allowed(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip parse certificate request"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        MINTLS_RETURN(ret);
    }

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate request message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    mbedtls_ssl_handshake_increment_state(ssl);
    ssl->handshake->client_auth =
        (ssl->in_msg[0] == MBEDTLS_SSL_HS_CERTIFICATE_REQUEST);

    MBEDTLS_SSL_DEBUG_MSG(3, ("got %s certificate request",
                              ssl->handshake->client_auth ? "a" : "no"));

    if (ssl->handshake->client_auth == 0) {
        /* Current message is probably the ServerHelloDone */
        ssl->keep_current_message = 1;
        goto exit;
    }

    /*
     *  struct {
     *      ClientCertificateType certificate_types<1..2^8-1>;
     *      SignatureAndHashAlgorithm
     *        supported_signature_algorithms<2^16-1>; -- TLS 1.2 only
     *      DistinguishedName certificate_authorities<0..2^16-1>;
     *  } CertificateRequest;
     *
     *  Since we only support a single certificate on clients, let's just
     *  ignore all the information that's supposed to help us pick a
     *  certificate.
     *
     *  We could check that our certificate matches the request, and bail out
     *  if it doesn't, but it's simpler to just send the certificate anyway,
     *  and give the server the opportunity to decide if it should terminate
     *  the connection when it doesn't like our certificate.
     *
     *  Same goes for the hash in TLS 1.2's signature_algorithms: at this
     *  point we only have one hash available (see comments in
     *  write_certificate_verify), so let's just use what we have.
     *
     *  However, we still minimally parse the message to check it is at least
     *  superficially sane.
     */
    buf = ssl->in_msg;

    /* certificate_types */
    if (ssl->in_hslen <= mbedtls_ssl_hs_hdr_len(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate request message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }
    cert_type_len = buf[mbedtls_ssl_hs_hdr_len(ssl)];
    n = cert_type_len;

    /*
     * In the subsequent code there are two paths that read from buf:
     *     * the length of the signature algorithms field (if minor version of
     *       SSL is 3),
     *     * distinguished name length otherwise.
     * Both reach at most the index:
     *    ...hdr_len + 2 + n,
     * therefore the buffer length at this point must be greater than that
     * regardless of the actual code path.
     */
    if (ssl->in_hslen <= mbedtls_ssl_hs_hdr_len(ssl) + 2 + n) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate request message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    /* supported_signature_algorithms */
    sig_alg_len = MBEDTLS_GET_UINT16_BE(buf, mbedtls_ssl_hs_hdr_len(ssl) + 1 + n);

    /*
     * The furthest access in buf is in the loop few lines below:
     *     sig_alg[i + 1],
     * where:
     *     sig_alg = buf + ...hdr_len + 3 + n,
     *     max(i) = sig_alg_len - 1.
     * Therefore the furthest access is:
     *     buf[...hdr_len + 3 + n + sig_alg_len - 1 + 1],
     * which reduces to:
     *     buf[...hdr_len + 3 + n + sig_alg_len],
     * which is one less than we need the buf to be.
     */
    if (ssl->in_hslen <= mbedtls_ssl_hs_hdr_len(ssl) + 3 + n + sig_alg_len) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate request message"));
        mbedtls_ssl_send_alert_message(
            ssl,
            MBEDTLS_SSL_ALERT_LEVEL_FATAL,
            MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    n += 2 + sig_alg_len;

    /* certificate_authorities */
    dn_len = MBEDTLS_GET_UINT16_BE(buf, mbedtls_ssl_hs_hdr_len(ssl) + 1 + n);

    n += dn_len;
    if (ssl->in_hslen != mbedtls_ssl_hs_hdr_len(ssl) + 3 + n) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate request message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

exit:
    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse certificate request"));

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_server_hello_done(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse server hello done"));

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        MINTLS_RETURN(ret);
    }

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello done message"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    if (ssl->in_hslen  != mbedtls_ssl_hs_hdr_len(ssl) ||
        ssl->in_msg[0] != MBEDTLS_SSL_HS_SERVER_HELLO_DONE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad server hello done message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    mbedtls_ssl_handshake_increment_state(ssl);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse server hello done"));

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_client_key_exchange(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    size_t header_len;
    size_t content_len;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write client key exchange"));

    if (ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_RSA ||
        ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA ||
        ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDH_RSA ||
        ciphersuite_info->key_exchange == MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA) {
        /*
         * ECDH key exchange -- send client public value
         */
        header_len = 4;

        ret = mbedtls_ecdh_make_public(&ssl->handshake->ecdh_ctx,
                                       &content_len,
                                       &ssl->out_msg[header_len], 1000,
                                       ssl->conf->f_rng, ssl->conf->p_rng, diagnostics);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ecdh_make_public", ret);
            MINTLS_RETURN(ret);
        }

        MBEDTLS_SSL_DEBUG_ECDH(3, &ssl->handshake->ecdh_ctx,
                               MBEDTLS_DEBUG_ECDH_Q);

        if ((ret = mbedtls_ecdh_calc_secret(&ssl->handshake->ecdh_ctx,
                                            &ssl->handshake->pmslen,
                                            ssl->handshake->premaster,
                                            MBEDTLS_PREMASTER_SIZE,
                                            ssl->conf->f_rng, ssl->conf->p_rng, diagnostics)) != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ecdh_calc_secret", ret);
            MINTLS_RETURN(ret);
        }

        MBEDTLS_SSL_DEBUG_ECDH(3, &ssl->handshake->ecdh_ctx,
                               MBEDTLS_DEBUG_ECDH_Z);
    } else

    {
        ((void) ciphersuite_info);
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    ssl->out_msglen  = header_len + content_len;
    ssl->out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = MBEDTLS_SSL_HS_CLIENT_KEY_EXCHANGE;

    mbedtls_ssl_handshake_increment_state(ssl);

    if ((ret = mbedtls_ssl_write_handshake_msg(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_handshake_msg", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write client key exchange"));

    MINTLS_RETURN(0);
}

#if !defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_certificate_verify(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write certificate verify"));

    if ((ret = mbedtls_ssl_derive_keys(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_derive_keys", ret);
        MINTLS_RETURN(ret);
    }

    if (!mbedtls_ssl_ciphersuite_cert_req_allowed(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate verify"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}
#else
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_write_certificate_verify(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;
    size_t n = 0, offset = 0;
    unsigned char hash[48];
    unsigned char *hash_start = hash;
    mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;
    size_t hashlen;
    void *rs_ctx = NULL;
    size_t out_buf_len = MBEDTLS_SSL_OUT_BUFFER_LEN - (size_t) (ssl->out_msg - ssl->out_buf);

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write certificate verify"));

    if ((ret = mbedtls_ssl_derive_keys(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_derive_keys", ret);
        MINTLS_RETURN(ret);
    }

    if (!mbedtls_ssl_ciphersuite_cert_req_allowed(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate verify"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    if (ssl->handshake->client_auth == 0 ||
        mbedtls_ssl_own_cert(ssl) == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate verify"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    if (mbedtls_ssl_own_key(ssl) == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("got no private key for certificate"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED);
    }

    /*
     * Make a signature of the handshake digests
     */

    ret = ssl->handshake->calc_verify(ssl, hash, &hashlen, diagnostics);
    if (0 != ret) {
        MBEDTLS_SSL_DEBUG_RET(1, ("calc_verify"), ret);
        MINTLS_RETURN(ret);
    }

    /*
     * digitally-signed struct {
     *     opaque handshake_messages[handshake_messages_length];
     * };
     *
     * Taking shortcut here. We assume that the server always allows the
     * PRF Hash function and has sent it in the allowed signature
     * algorithms list received in the Certificate Request message.
     *
     * Until we encounter a server that does not, we will take this
     * shortcut.
     *
     * Reason: Otherwise we should have running hashes for SHA512 and
     *         SHA224 in order to satisfy 'weird' needs from the server
     *         side.
     */
    if (ssl->handshake->ciphersuite_info->mac == MBEDTLS_MD_SHA384) {
        md_alg = MBEDTLS_MD_SHA384;
        ssl->out_msg[4] = MBEDTLS_SSL_HASH_SHA384;
    } else {
        md_alg = MBEDTLS_MD_SHA256;
        ssl->out_msg[4] = MBEDTLS_SSL_HASH_SHA256;
    }
    ssl->out_msg[5] = mbedtls_ssl_sig_from_pk(mbedtls_ssl_own_key(ssl));

    /* Info from md_alg will be used instead */
    hashlen = 0;
    offset = 2;

    if ((ret = mbedtls_pk_sign_restartable(mbedtls_ssl_own_key(ssl),
                                           md_alg, hash_start, hashlen,
                                           ssl->out_msg + 6 + offset,
                                           out_buf_len - 6 - offset,
                                           &n,
                                           ssl->conf->f_rng, ssl->conf->p_rng, rs_ctx, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_pk_sign", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_PUT_UINT16_BE(n, ssl->out_msg, offset + 4);

    ssl->out_msglen  = 6 + n + offset;
    ssl->out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = MBEDTLS_SSL_HS_CERTIFICATE_VERIFY;

    mbedtls_ssl_handshake_increment_state(ssl);

    if ((ret = mbedtls_ssl_write_handshake_msg(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_handshake_msg", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write certificate verify"));

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED */

/*
 * SSL handshake -- client side -- single step
 */
int mbedtls_ssl_handshake_client_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;

    /* Change state now, so that it is right in mbedtls_ssl_read_record(), used
     * by DTLS for dropping out-of-sequence ChangeCipherSpec records */

    switch (ssl->state) {
        case MBEDTLS_SSL_HELLO_REQUEST:
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_HELLO);
            break;

        /*
         *  ==>   ClientHello
         */
        case MBEDTLS_SSL_CLIENT_HELLO:
            ret = mbedtls_ssl_write_client_hello(ssl, diagnostics);
            break;

        /*
         *  <==   ServerHello
         *        Certificate
         *      ( ServerKeyExchange  )
         *      ( CertificateRequest )
         *        ServerHelloDone
         */
        case MBEDTLS_SSL_SERVER_HELLO:
            ret = ssl_parse_server_hello(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_SERVER_CERTIFICATE:
            ret = mbedtls_ssl_parse_certificate(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_SERVER_KEY_EXCHANGE:
            ret = ssl_parse_server_key_exchange(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_CERTIFICATE_REQUEST:
            ret = ssl_parse_certificate_request(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_SERVER_HELLO_DONE:
            ret = ssl_parse_server_hello_done(ssl, diagnostics);
            break;

        /*
         *  ==> ( Certificate/Alert  )
         *        ClientKeyExchange
         *      ( CertificateVerify  )
         *        ChangeCipherSpec
         *        Finished
         */
        case MBEDTLS_SSL_CLIENT_CERTIFICATE:
            ret = mbedtls_ssl_write_certificate(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_CLIENT_KEY_EXCHANGE:
            ret = ssl_write_client_key_exchange(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_CERTIFICATE_VERIFY:
            ret = ssl_write_certificate_verify(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC:
            ret = mbedtls_ssl_write_change_cipher_spec(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_CLIENT_FINISHED:
            ret = mbedtls_ssl_write_finished(ssl, diagnostics);
            break;

            /*
             *  <==   ( NewSessionTicket )
             *        ChangeCipherSpec
             *        Finished
             */

        case MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC:
            ret = mbedtls_ssl_parse_change_cipher_spec(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_SERVER_FINISHED:
            ret = mbedtls_ssl_parse_finished(ssl, diagnostics);
            break;

        case MBEDTLS_SSL_FLUSH_BUFFERS:
            MBEDTLS_SSL_DEBUG_MSG(2, ("handshake: done"));
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_WRAPUP);
            break;

        case MBEDTLS_SSL_HANDSHAKE_WRAPUP:
            mbedtls_ssl_handshake_wrapup(ssl);
            break;

        default:
            MBEDTLS_SSL_DEBUG_MSG(1, ("invalid state %d", ssl->state));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(ret);
}
