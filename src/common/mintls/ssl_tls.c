// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  TLS shared functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ssl_tls.c.
 */
/*
 *  http://www.ietf.org/rfc/rfc2246.txt
 *  http://www.ietf.org/rfc/rfc4346.txt
 */

#include "common.h"

#include "platform.h"

#include "ssl.h"
#include "ssl_client.h"
#include "ssl_debug_helpers.h"
#include "ssl_misc.h"

#include "debug_internal.h"
#include "error.h"
#include "platform_util.h"
#include "version.h"
#include "constant_time.h"

#include <string.h>

#include "oid.h"

int mbedtls_ssl_session_copy(mbedtls_ssl_session *dst,
                             const mbedtls_ssl_session *src, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_ssl_session_free(dst);
    memcpy(dst, src, sizeof(mbedtls_ssl_session));

    if (src->peer_cert != NULL) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        dst->peer_cert = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
        if (dst->peer_cert == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
        }

        mbedtls_x509_crt_init(dst->peer_cert);

        if ((ret = mbedtls_x509_crt_parse_der(dst->peer_cert, src->peer_cert->raw.p,
                                              src->peer_cert->raw.len, diagnostics)) != 0) {
            mbedtls_free(dst->peer_cert);
            dst->peer_cert = NULL;
            MINTLS_RETURN(ret);
        }
    }

    MINTLS_RETURN(0);
}

/* Type for the TLS PRF */
typedef int ssl_tls_prf_t(const unsigned char *, size_t, const char *,
                          const unsigned char *, size_t,
                          unsigned char *, size_t, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_tls12_populate_transform(mbedtls_ssl_transform *transform,
                                        int ciphersuite,
                                        const unsigned char master[48],
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
                                        int encrypt_then_mac,
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
                                        ssl_tls_prf_t tls_prf,
                                        const unsigned char randbytes[64],
                                        mbedtls_ssl_protocol_version tls_version,
                                        unsigned endpoint,
                                        const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int tls_prf_sha256(const unsigned char *secret, size_t slen,
                          const char *label,
                          const unsigned char *random, size_t rlen,
                          unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics);
int ssl_calc_verify_tls_sha256(const mbedtls_ssl_context *, unsigned char *, size_t *, MinTlsDiagnostics* diagnostics);
int ssl_calc_finished_tls_sha256(mbedtls_ssl_context *, unsigned char *, int, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int tls_prf_sha384(const unsigned char *secret, size_t slen,
                          const char *label,
                          const unsigned char *random, size_t rlen,
                          unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics);

int ssl_calc_verify_tls_sha384(const mbedtls_ssl_context *, unsigned char *, size_t *, MinTlsDiagnostics* diagnostics);
int ssl_calc_finished_tls_sha384(mbedtls_ssl_context *, unsigned char *, int, MinTlsDiagnostics* diagnostics);

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_tls12_session_load(mbedtls_ssl_session *session,
                                  const unsigned char *buf,
                                  size_t len, MinTlsDiagnostics* diagnostics);

int ssl_update_checksum_start(mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);

int ssl_update_checksum_sha256(mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);

int ssl_update_checksum_sha384(mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);

int  mbedtls_ssl_tls_prf(const mbedtls_tls_prf_types prf,
                         const unsigned char *secret, size_t slen,
                         const char *label,
                         const unsigned char *random, size_t rlen,
                         unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_ssl_tls_prf_cb *tls_prf = NULL;

    switch (prf) {
        case MBEDTLS_SSL_TLS_PRF_SHA384:
            tls_prf = tls_prf_sha384;
            break;
        case MBEDTLS_SSL_TLS_PRF_SHA256:
            tls_prf = tls_prf_sha256;
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }

    MINTLS_RETURN(tls_prf(secret, slen, label, random, rlen, dstbuf, dlen, diagnostics));
}

void ssl_clear_peer_cert(mbedtls_ssl_session *session)
{
    if (session->peer_cert != NULL) {
        mbedtls_x509_crt_free(session->peer_cert);
        mbedtls_free(session->peer_cert);
        session->peer_cert = NULL;
    }
}

uint32_t mbedtls_ssl_get_extension_id(unsigned int extension_type)
{
    switch (extension_type) {
        case MBEDTLS_TLS_EXT_SERVERNAME:
            return MBEDTLS_SSL_EXT_ID_SERVERNAME;

        case MBEDTLS_TLS_EXT_MAX_FRAGMENT_LENGTH:
            return MBEDTLS_SSL_EXT_ID_MAX_FRAGMENT_LENGTH;

        case MBEDTLS_TLS_EXT_STATUS_REQUEST:
            return MBEDTLS_SSL_EXT_ID_STATUS_REQUEST;

        case MBEDTLS_TLS_EXT_SUPPORTED_GROUPS:
            return MBEDTLS_SSL_EXT_ID_SUPPORTED_GROUPS;

        case MBEDTLS_TLS_EXT_SIG_ALG:
            return MBEDTLS_SSL_EXT_ID_SIG_ALG;

        case MBEDTLS_TLS_EXT_USE_SRTP:
            return MBEDTLS_SSL_EXT_ID_USE_SRTP;

        case MBEDTLS_TLS_EXT_HEARTBEAT:
            return MBEDTLS_SSL_EXT_ID_HEARTBEAT;

        case MBEDTLS_TLS_EXT_ALPN:
            return MBEDTLS_SSL_EXT_ID_ALPN;

        case MBEDTLS_TLS_EXT_SCT:
            return MBEDTLS_SSL_EXT_ID_SCT;

        case MBEDTLS_TLS_EXT_CLI_CERT_TYPE:
            return MBEDTLS_SSL_EXT_ID_CLI_CERT_TYPE;

        case MBEDTLS_TLS_EXT_SERV_CERT_TYPE:
            return MBEDTLS_SSL_EXT_ID_SERV_CERT_TYPE;

        case MBEDTLS_TLS_EXT_PADDING:
            return MBEDTLS_SSL_EXT_ID_PADDING;

        case MBEDTLS_TLS_EXT_PRE_SHARED_KEY:
            return MBEDTLS_SSL_EXT_ID_PRE_SHARED_KEY;

        case MBEDTLS_TLS_EXT_EARLY_DATA:
            return MBEDTLS_SSL_EXT_ID_EARLY_DATA;

        case MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS:
            return MBEDTLS_SSL_EXT_ID_SUPPORTED_VERSIONS;

        case MBEDTLS_TLS_EXT_COOKIE:
            return MBEDTLS_SSL_EXT_ID_COOKIE;

        case MBEDTLS_TLS_EXT_PSK_KEY_EXCHANGE_MODES:
            return MBEDTLS_SSL_EXT_ID_PSK_KEY_EXCHANGE_MODES;

        case MBEDTLS_TLS_EXT_CERT_AUTH:
            return MBEDTLS_SSL_EXT_ID_CERT_AUTH;

        case MBEDTLS_TLS_EXT_OID_FILTERS:
            return MBEDTLS_SSL_EXT_ID_OID_FILTERS;

        case MBEDTLS_TLS_EXT_POST_HANDSHAKE_AUTH:
            return MBEDTLS_SSL_EXT_ID_POST_HANDSHAKE_AUTH;

        case MBEDTLS_TLS_EXT_SIG_ALG_CERT:
            return MBEDTLS_SSL_EXT_ID_SIG_ALG_CERT;

        case MBEDTLS_TLS_EXT_KEY_SHARE:
            return MBEDTLS_SSL_EXT_ID_KEY_SHARE;

        case MBEDTLS_TLS_EXT_TRUNCATED_HMAC:
            return MBEDTLS_SSL_EXT_ID_TRUNCATED_HMAC;

        case MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS:
            return MBEDTLS_SSL_EXT_ID_SUPPORTED_POINT_FORMATS;

        case MBEDTLS_TLS_EXT_ENCRYPT_THEN_MAC:
            return MBEDTLS_SSL_EXT_ID_ENCRYPT_THEN_MAC;

        case MBEDTLS_TLS_EXT_EXTENDED_MASTER_SECRET:
            return MBEDTLS_SSL_EXT_ID_EXTENDED_MASTER_SECRET;

        case MBEDTLS_TLS_EXT_RECORD_SIZE_LIMIT:
            return MBEDTLS_SSL_EXT_ID_RECORD_SIZE_LIMIT;

        case MBEDTLS_TLS_EXT_SESSION_TICKET:
            return MBEDTLS_SSL_EXT_ID_SESSION_TICKET;

    }

    return MBEDTLS_SSL_EXT_ID_UNRECOGNIZED;
}

uint32_t mbedtls_ssl_get_extension_mask(unsigned int extension_type)
{
    return 1 << mbedtls_ssl_get_extension_id(extension_type);
}

void mbedtls_ssl_optimize_checksum(mbedtls_ssl_context *ssl,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info)
{
    ((void) ciphersuite_info);

    if (ciphersuite_info->mac == MBEDTLS_MD_SHA384) {
        ssl->handshake->update_checksum = ssl_update_checksum_sha384;
    } else
    if (ciphersuite_info->mac != MBEDTLS_MD_SHA384) {
        ssl->handshake->update_checksum = ssl_update_checksum_sha256;
    } else
    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        return;
    }
}

int mbedtls_ssl_add_hs_hdr_to_checksum(mbedtls_ssl_context *ssl,
                                       unsigned hs_type,
                                       size_t total_hs_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char hs_hdr[4];

    /* Build HS header for checksum update. */
    hs_hdr[0] = MBEDTLS_BYTE_0(hs_type);
    hs_hdr[1] = MBEDTLS_BYTE_2(total_hs_len);
    hs_hdr[2] = MBEDTLS_BYTE_1(total_hs_len);
    hs_hdr[3] = MBEDTLS_BYTE_0(total_hs_len);

    MINTLS_RETURN(ssl->handshake->update_checksum(ssl, hs_hdr, sizeof(hs_hdr), diagnostics));
}

int mbedtls_ssl_add_hs_msg_to_checksum(mbedtls_ssl_context *ssl,
                                       unsigned hs_type,
                                       unsigned char const *msg,
                                       size_t msg_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;
    ret = mbedtls_ssl_add_hs_hdr_to_checksum(ssl, hs_type, msg_len, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    MINTLS_RETURN(ssl->handshake->update_checksum(ssl, msg, msg_len, diagnostics));
}

int mbedtls_ssl_reset_checksum(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_md_free(&ssl->handshake->fin_sha256);
    mbedtls_md_init(&ssl->handshake->fin_sha256);
    ret = mbedtls_md_setup(&ssl->handshake->fin_sha256,
                           mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                           0, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    ret = mbedtls_md_starts(&ssl->handshake->fin_sha256, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    mbedtls_md_free(&ssl->handshake->fin_sha384);
    mbedtls_md_init(&ssl->handshake->fin_sha384);
    ret = mbedtls_md_setup(&ssl->handshake->fin_sha384,
                           mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), 0, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    ret = mbedtls_md_starts(&ssl->handshake->fin_sha384, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    MINTLS_RETURN(0);
}

int ssl_update_checksum_start(mbedtls_ssl_context *ssl,
                                     const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    ret = mbedtls_md_update(&ssl->handshake->fin_sha256, buf, len, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    ret = mbedtls_md_update(&ssl->handshake->fin_sha384, buf, len, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    MINTLS_RETURN(0);
}

int ssl_update_checksum_sha256(mbedtls_ssl_context *ssl,
                                      const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_md_update(&ssl->handshake->fin_sha256, buf, len, diagnostics));
}

int ssl_update_checksum_sha384(mbedtls_ssl_context *ssl,
                                      const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_md_update(&ssl->handshake->fin_sha384, buf, len, diagnostics));
}

void ssl_handshake_params_init(mbedtls_ssl_handshake_params *handshake)
{
    memset(handshake, 0, sizeof(mbedtls_ssl_handshake_params));

    mbedtls_md_init(&handshake->fin_sha256);
    mbedtls_md_init(&handshake->fin_sha384);

    handshake->update_checksum = ssl_update_checksum_start;

#if !defined(MBEDTLS_USE_PSA_CRYPTO) && \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED)
    mbedtls_ecdh_init(&handshake->ecdh_ctx);
#endif

    handshake->sni_authmode = MBEDTLS_SSL_VERIFY_UNSET;

}

void mbedtls_ssl_transform_init(mbedtls_ssl_transform *transform)
{
    memset(transform, 0, sizeof(mbedtls_ssl_transform));

    mbedtls_cipher_init(&transform->cipher_ctx_enc);
    mbedtls_cipher_init(&transform->cipher_ctx_dec);

}

void mbedtls_ssl_session_init(mbedtls_ssl_session *session)
{
    memset(session, 0, sizeof(mbedtls_ssl_session));
    /* Set verify_result to -1u to indicate 'result not available'. */
    session->verify_result = 0xFFFFFFFF;
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_handshake_init(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* Clear old handshake information if present */
    if (ssl->transform_negotiate) {
        mbedtls_ssl_transform_free(ssl->transform_negotiate);
    }
    if (ssl->session_negotiate) {
        mbedtls_ssl_session_free(ssl->session_negotiate);
    }
    if (ssl->handshake) {
        mbedtls_ssl_handshake_free(ssl);
    }

    /*
     * Either the pointers are now NULL or cleared properly and can be freed.
     * Now allocate missing structures.
     */
    if (ssl->transform_negotiate == NULL) {
        ssl->transform_negotiate = mbedtls_calloc(1, sizeof(mbedtls_ssl_transform));
    }

    if (ssl->session_negotiate == NULL) {
        ssl->session_negotiate = mbedtls_calloc(1, sizeof(mbedtls_ssl_session));
    }

    if (ssl->handshake == NULL) {
        ssl->handshake = mbedtls_calloc(1, sizeof(mbedtls_ssl_handshake_params));
    }

    /* All pointers should exist and can be directly freed without issue */
    if (ssl->handshake           == NULL ||
        ssl->transform_negotiate == NULL ||
        ssl->session_negotiate   == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("alloc() of ssl sub-contexts failed"));

        mbedtls_free(ssl->handshake);
        ssl->handshake = NULL;

        mbedtls_free(ssl->transform_negotiate);
        ssl->transform_negotiate = NULL;

        mbedtls_free(ssl->session_negotiate);
        ssl->session_negotiate = NULL;

        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
    }

    /* Initialize structures */
    mbedtls_ssl_session_init(ssl->session_negotiate);
    ssl_handshake_params_init(ssl->handshake);

    mbedtls_ssl_transform_init(ssl->transform_negotiate);

    /* Setup handshake checksums */
    ret = mbedtls_ssl_reset_checksum(ssl, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_reset_checksum", ret);
        MINTLS_RETURN(ret);
    }

/*
 * curve_list is translated to IANA TLS group identifiers here because
 * mbedtls_ssl_conf_curves returns void and so can't return
 * any error codes.
 */
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    /* Heap allocate and translate curve_list from internal to IANA group ids */
    if (ssl->conf->curve_list != NULL) {
        size_t length;
        const mbedtls_ecp_group_id *curve_list = ssl->conf->curve_list;

        for (length = 0;  (curve_list[length] != MBEDTLS_ECP_DP_NONE); length++) {
        }

        /* Leave room for zero termination */
        uint16_t *group_list = mbedtls_calloc(length + 1, sizeof(uint16_t));
        if (group_list == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
        }

        for (size_t i = 0; i < length; i++) {
            uint16_t tls_id = mbedtls_ssl_get_tls_id_from_ecp_group_id(
                curve_list[i]);
            if (tls_id == 0) {
                mbedtls_free(group_list);
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CONFIG);
            }
            group_list[i] = tls_id;
        }

        group_list[length] = 0;

        ssl->handshake->group_list = group_list;
        ssl->handshake->group_list_heap_allocated = 1;
    } else {
        ssl->handshake->group_list = ssl->conf->group_list;
        ssl->handshake->group_list_heap_allocated = 0;
    }
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    /* Heap allocate and translate sig_hashes from internal hash identifiers to
       signature algorithms IANA identifiers.  */
    if (mbedtls_ssl_conf_is_tls12_only(ssl->conf) &&
        ssl->conf->sig_hashes != NULL) {
        const int *md;
        const int *sig_hashes = ssl->conf->sig_hashes;
        size_t sig_algs_len = 0;
        uint16_t *p;

        MBEDTLS_STATIC_ASSERT(MBEDTLS_SSL_MAX_SIG_ALG_LIST_LEN
                              <= (SIZE_MAX - (2 * sizeof(uint16_t))),
                              "MBEDTLS_SSL_MAX_SIG_ALG_LIST_LEN too big");

        for (md = sig_hashes; *md != MBEDTLS_MD_NONE; md++) {
            if (mbedtls_ssl_hash_from_md_alg(*md) == MBEDTLS_SSL_HASH_NONE) {
                continue;
            }
#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
            sig_algs_len += sizeof(uint16_t);
#endif

            sig_algs_len += sizeof(uint16_t);
            if (sig_algs_len > MBEDTLS_SSL_MAX_SIG_ALG_LIST_LEN) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CONFIG);
            }
        }

        if (sig_algs_len < MBEDTLS_SSL_MIN_SIG_ALG_LIST_LEN) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CONFIG);
        }

        ssl->handshake->sig_algs = mbedtls_calloc(1, sig_algs_len +
                                                  sizeof(uint16_t));
        if (ssl->handshake->sig_algs == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
        }

        p = (uint16_t *) ssl->handshake->sig_algs;
        for (md = sig_hashes; *md != MBEDTLS_MD_NONE; md++) {
            unsigned char hash = mbedtls_ssl_hash_from_md_alg(*md);
            if (hash == MBEDTLS_SSL_HASH_NONE) {
                continue;
            }
#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
            *p = ((hash << 8) | MBEDTLS_SSL_SIG_ECDSA);
            p++;
#endif
            *p = ((hash << 8) | MBEDTLS_SSL_SIG_RSA);
            p++;
        }
        *p = MBEDTLS_TLS_SIG_NONE;
        ssl->handshake->sig_algs_heap_allocated = 1;
    } else
    {
        ssl->handshake->sig_algs_heap_allocated = 0;
    }
#endif /* !MBEDTLS_DEPRECATED_REMOVED */
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */
    MINTLS_RETURN(0);
}

/*
 * Initialize an SSL context
 */
void mbedtls_ssl_init(mbedtls_ssl_context *ssl)
{
    memset(ssl, 0, sizeof(mbedtls_ssl_context));
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_conf_version_check(const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ssl_config *conf = ssl->conf;

    if (mbedtls_ssl_conf_is_tls12_only(conf)) {
        MBEDTLS_SSL_DEBUG_MSG(4, ("The SSL configuration is tls12 only."));
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("The SSL configuration is invalid."));
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CONFIG);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_conf_check(const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;
    ret = ssl_conf_version_check(ssl, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    if (ssl->conf->f_rng == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("no RNG provided"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_NO_RNG);
    }

    /* Space for further checks */

    MINTLS_RETURN(0);
}

/*
 * Setup an SSL context
 */

int mbedtls_ssl_setup(mbedtls_ssl_context *ssl,
                      const mbedtls_ssl_config *conf, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t in_buf_len = MBEDTLS_SSL_IN_BUFFER_LEN;
    size_t out_buf_len = MBEDTLS_SSL_OUT_BUFFER_LEN;

    ssl->conf = conf;

    if ((ret = ssl_conf_check(ssl, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }
    ssl->tls_version = ssl->conf->max_tls_version;

    /*
     * Prepare base structures
     */

    /* Set to NULL in case of an error condition */
    ssl->out_buf = NULL;

    ssl->in_buf = mbedtls_calloc(1, in_buf_len);
    if (ssl->in_buf == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("alloc(%" MBEDTLS_PRINTF_SIZET " bytes) failed", in_buf_len));
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_ALLOC_FAILED);
        goto error;
    }

    ssl->out_buf = mbedtls_calloc(1, out_buf_len);
    if (ssl->out_buf == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("alloc(%" MBEDTLS_PRINTF_SIZET " bytes) failed", out_buf_len));
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_ALLOC_FAILED);
        goto error;
    }

    mbedtls_ssl_reset_in_pointers(ssl);
    mbedtls_ssl_reset_out_pointers(ssl);

    if ((ret = ssl_handshake_init(ssl, diagnostics)) != 0) {
        goto error;
    }

    MINTLS_RETURN(0);

error:
    mbedtls_free(ssl->in_buf);
    mbedtls_free(ssl->out_buf);

    ssl->conf = NULL;

    ssl->in_buf = NULL;
    ssl->out_buf = NULL;

    ssl->in_hdr = NULL;
    ssl->in_ctr = NULL;
    ssl->in_len = NULL;
    ssl->in_iv = NULL;
    ssl->in_msg = NULL;

    ssl->out_hdr = NULL;
    ssl->out_ctr = NULL;
    ssl->out_len = NULL;
    ssl->out_iv = NULL;
    ssl->out_msg = NULL;

    MINTLS_RETURN(ret);
}

/*
 * Reset an initialized and used SSL context for re-use while retaining
 * all application-set variables, function pointers and data.
 *
 * If partial is non-zero, keep data in the input buffer and client ID.
 * (Use when a DTLS client reconnects from the same port.)
 */
void mbedtls_ssl_session_reset_msg_layer(mbedtls_ssl_context *ssl,
                                         int partial)
{
    size_t in_buf_len = MBEDTLS_SSL_IN_BUFFER_LEN;
    size_t out_buf_len = MBEDTLS_SSL_OUT_BUFFER_LEN;

    partial = 0;

    /* Cancel any possibly running timer */
    mbedtls_ssl_set_timer(ssl, 0);

    mbedtls_ssl_reset_in_pointers(ssl);
    mbedtls_ssl_reset_out_pointers(ssl);

    /* Reset incoming message parsing */
    ssl->in_offt    = NULL;
    ssl->nb_zero    = 0;
    ssl->in_msgtype = 0;
    ssl->in_msglen  = 0;
    ssl->in_hslen   = 0;
    ssl->keep_current_message = 0;
    ssl->transform_in  = NULL;

    /* TLS: reset in_hsfraglen, which is part of message parsing.
     * DTLS: on a client reconnect, don't reset badmac_seen. */
    if (!partial) {
        ssl->badmac_seen_or_in_hsfraglen = 0;
    }

    /* Keep current datagram if partial == 1 */
    if (partial == 0) {
        ssl->in_left = 0;
        memset(ssl->in_buf, 0, in_buf_len);
    }

    ssl->send_alert = 0;
    ssl->alert_reason = 0;
    ssl->alert_type = 0;

    /* Reset outgoing message writing */
    ssl->out_msgtype = 0;
    ssl->out_msglen  = 0;
    ssl->out_left    = 0;
    memset(ssl->out_buf, 0, out_buf_len);
    memset(ssl->cur_out_ctr, 0, sizeof(ssl->cur_out_ctr));
    ssl->transform_out = NULL;

    if (ssl->transform) {
        mbedtls_ssl_transform_free(ssl->transform);
        mbedtls_free(ssl->transform);
        ssl->transform = NULL;
    }

}

int mbedtls_ssl_session_reset_int(mbedtls_ssl_context *ssl, int partial, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HELLO_REQUEST);
    ssl->tls_version = ssl->conf->max_tls_version;

    mbedtls_ssl_session_reset_msg_layer(ssl, partial);

    /* Reset renegotiation state */
    ssl->secure_renegotiation = MBEDTLS_SSL_LEGACY_RENEGOTIATION;

    ssl->session_in  = NULL;
    ssl->session_out = NULL;
    if (ssl->session) {
        mbedtls_ssl_session_free(ssl->session);
        mbedtls_free(ssl->session);
        ssl->session = NULL;
    }

    ssl->alpn_chosen = NULL;

    if ((ret = ssl_handshake_init(ssl, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}

/*
 * Reset an initialized and used SSL context for re-use while retaining
 * all application-set variables, function pointers and data.
 */
int mbedtls_ssl_session_reset(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ssl_session_reset_int(ssl, 0, diagnostics));
}

/*
 * SSL set accessors
 */
void mbedtls_ssl_conf_endpoint(mbedtls_ssl_config *conf, int endpoint)
{
    conf->endpoint   = endpoint;
}

void mbedtls_ssl_conf_transport(mbedtls_ssl_config *conf, int transport)
{
    conf->transport = transport;
}

void mbedtls_ssl_conf_dtls_badmac_limit(mbedtls_ssl_config *conf, unsigned limit)
{
    conf->badmac_limit = limit;
}

void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode)
{
    conf->authmode   = authmode;
}

void mbedtls_ssl_conf_verify(mbedtls_ssl_config *conf,
                             int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                             void *p_vrfy)
{
    conf->f_vrfy      = f_vrfy;
    conf->p_vrfy      = p_vrfy;
}

void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                          void *p_rng)
{
    conf->f_rng      = f_rng;
    conf->p_rng      = p_rng;
}

void mbedtls_ssl_conf_dbg(mbedtls_ssl_config *conf,
                          void (*f_dbg)(void *, int, const char *, int, const char *),
                          void  *p_dbg)
{
    conf->f_dbg      = f_dbg;
    conf->p_dbg      = p_dbg;
}

void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,
                         void *p_bio,
                         mbedtls_ssl_send_t *f_send,
                         mbedtls_ssl_recv_t *f_recv,
                         mbedtls_ssl_recv_timeout_t *f_recv_timeout)
{
    ssl->p_bio          = p_bio;
    ssl->f_send         = f_send;
    ssl->f_recv         = f_recv;
    ssl->f_recv_timeout = f_recv_timeout;
}

void mbedtls_ssl_conf_read_timeout(mbedtls_ssl_config *conf, uint32_t timeout)
{
    conf->read_timeout   = timeout;
}

void mbedtls_ssl_set_timer_cb(mbedtls_ssl_context *ssl,
                              void *p_timer,
                              mbedtls_ssl_set_timer_t *f_set_timer,
                              mbedtls_ssl_get_timer_t *f_get_timer)
{
    ssl->p_timer        = p_timer;
    ssl->f_set_timer    = f_set_timer;
    ssl->f_get_timer    = f_get_timer;

    /* Make sure we start with no timer running */
    mbedtls_ssl_set_timer(ssl, 0);
}

int mbedtls_ssl_set_session(mbedtls_ssl_context *ssl, const mbedtls_ssl_session *session, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ssl == NULL ||
        session == NULL ||
        ssl->session_negotiate == NULL ||
        ssl->conf->endpoint != MBEDTLS_SSL_IS_CLIENT) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (ssl->handshake->resume == 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }

    if ((ret = mbedtls_ssl_session_copy(ssl->session_negotiate,
                                        session, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    ssl->handshake->resume = 1;

    MINTLS_RETURN(0);
}

void mbedtls_ssl_conf_ciphersuites(mbedtls_ssl_config *conf,
                                   const int *ciphersuites)
{
    conf->ciphersuite_list = ciphersuites;
}

void mbedtls_ssl_conf_cert_profile(mbedtls_ssl_config *conf,
                                   const mbedtls_x509_crt_profile *profile)
{
    conf->cert_profile = profile;
}

void ssl_key_cert_free(mbedtls_ssl_key_cert *key_cert)
{
    mbedtls_ssl_key_cert *cur = key_cert, *next;

    while (cur != NULL) {
        next = cur->next;
        mbedtls_free(cur);
        cur = next;
    }
}

/* Append a new keycert entry to a (possibly empty) list */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_append_key_cert(mbedtls_ssl_key_cert **head,
                               mbedtls_x509_crt *cert,
                               mbedtls_pk_context *key, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_ssl_key_cert *new_cert;

    if (cert == NULL) {
        /* Free list if cert is null */
        ssl_key_cert_free(*head);
        *head = NULL;
        MINTLS_RETURN(0);
    }

    new_cert = mbedtls_calloc(1, sizeof(mbedtls_ssl_key_cert));
    if (new_cert == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
    }

    new_cert->cert = cert;
    new_cert->key  = key;
    new_cert->next = NULL;

    /* Update head if the list was null, else add to the end */
    if (*head == NULL) {
        *head = new_cert;
    } else {
        mbedtls_ssl_key_cert *cur = *head;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new_cert;
    }

    MINTLS_RETURN(0);
}

int mbedtls_ssl_conf_own_cert(mbedtls_ssl_config *conf,
                              mbedtls_x509_crt *own_cert,
                              mbedtls_pk_context *pk_key, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ssl_append_key_cert(&conf->key_cert, own_cert, pk_key, diagnostics));
}

void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf,
                               mbedtls_x509_crt *ca_chain,
                               mbedtls_x509_crl *ca_crl)
{
    conf->ca_chain   = ca_chain;
    conf->ca_crl     = ca_crl;

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
    /* mbedtls_ssl_conf_ca_chain() and mbedtls_ssl_conf_ca_cb()
     * cannot be used together. */
    conf->f_ca_cb = NULL;
    conf->p_ca_cb = NULL;
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */
}

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
void mbedtls_ssl_conf_ca_cb(mbedtls_ssl_config *conf,
                            mbedtls_x509_crt_ca_cb_t f_ca_cb,
                            void *p_ca_cb)
{
    conf->f_ca_cb = f_ca_cb;
    conf->p_ca_cb = p_ca_cb;

    /* mbedtls_ssl_conf_ca_chain() and mbedtls_ssl_conf_ca_cb()
     * cannot be used together. */
    conf->ca_chain   = NULL;
    conf->ca_crl     = NULL;
}
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */

const unsigned char *mbedtls_ssl_get_hs_sni(mbedtls_ssl_context *ssl,
                                            size_t *name_len)
{
    *name_len = ssl->handshake->sni_name_len;
    return ssl->handshake->sni_name;
}

int mbedtls_ssl_set_hs_own_cert(mbedtls_ssl_context *ssl,
                                mbedtls_x509_crt *own_cert,
                                mbedtls_pk_context *pk_key, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ssl_append_key_cert(&ssl->handshake->sni_key_cert,
                               own_cert, pk_key, diagnostics));
}

void mbedtls_ssl_set_hs_ca_chain(mbedtls_ssl_context *ssl,
                                 mbedtls_x509_crt *ca_chain,
                                 mbedtls_x509_crl *ca_crl)
{
    ssl->handshake->sni_ca_chain   = ca_chain;
    ssl->handshake->sni_ca_crl     = ca_crl;
}

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
void mbedtls_ssl_set_hs_dn_hints(mbedtls_ssl_context *ssl,
                                 const mbedtls_x509_crt *crt)
{
    ssl->handshake->dn_hints = crt;
}
#endif /* MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED */

void mbedtls_ssl_set_hs_authmode(mbedtls_ssl_context *ssl,
                                 int authmode)
{
    ssl->handshake->sni_authmode = authmode;
}

void mbedtls_ssl_set_verify(mbedtls_ssl_context *ssl,
                            int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                            void *p_vrfy)
{
    ssl->f_vrfy = f_vrfy;
    ssl->p_vrfy = p_vrfy;
}

mbedtls_ssl_mode_t mbedtls_ssl_get_base_mode(
    mbedtls_cipher_mode_t mode)
{

    if (mode == MBEDTLS_MODE_GCM ||
        mode == MBEDTLS_MODE_CCM ||
        mode == MBEDTLS_MODE_CHACHAPOLY) {
        return MBEDTLS_SSL_MODE_AEAD;
    }

    return MBEDTLS_SSL_MODE_STREAM;
}

mbedtls_ssl_mode_t mbedtls_ssl_get_actual_mode(
    mbedtls_ssl_mode_t base_mode,
    int encrypt_then_mac)
{
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
    if (encrypt_then_mac == MBEDTLS_SSL_ETM_ENABLED &&
        base_mode == MBEDTLS_SSL_MODE_CBC) {
        return MBEDTLS_SSL_MODE_CBC_ETM;
    }
#else
    (void) encrypt_then_mac;
#endif
    return base_mode;
}

mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_transform(
    const mbedtls_ssl_transform *transform)
{
    mbedtls_ssl_mode_t base_mode = mbedtls_ssl_get_base_mode(
        mbedtls_cipher_get_cipher_mode(&transform->cipher_ctx_enc)
        );

    int encrypt_then_mac = 0;
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
    encrypt_then_mac = transform->encrypt_then_mac;
#endif
    return mbedtls_ssl_get_actual_mode(base_mode, encrypt_then_mac);
}

mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_ciphersuite(
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
    int encrypt_then_mac,
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
    const mbedtls_ssl_ciphersuite_t *suite)
{
    mbedtls_ssl_mode_t base_mode = MBEDTLS_SSL_MODE_STREAM;

    const mbedtls_cipher_info_t *cipher =
        mbedtls_cipher_info_from_type((mbedtls_cipher_type_t) suite->cipher);
    if (cipher != NULL) {
        base_mode =
            mbedtls_ssl_get_base_mode(
                mbedtls_cipher_info_get_mode(cipher));
    }

#if !defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
    int encrypt_then_mac = 0;
#endif
    return mbedtls_ssl_get_actual_mode(base_mode, encrypt_then_mac);
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
#if !defined(MBEDTLS_DEPRECATED_REMOVED) && defined(MBEDTLS_SSL_PROTO_TLS1_2)
/*
 * Set allowed/preferred hashes for handshake signatures
 */
void mbedtls_ssl_conf_sig_hashes(mbedtls_ssl_config *conf,
                                 const int *hashes)
{
    conf->sig_hashes = hashes;
}
#endif /* !MBEDTLS_DEPRECATED_REMOVED && MBEDTLS_SSL_PROTO_TLS1_2 */

/* Configure allowed signature algorithms for handshake */
void mbedtls_ssl_conf_sig_algs(mbedtls_ssl_config *conf,
                               const uint16_t *sig_algs)
{
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    conf->sig_hashes = NULL;
#endif /* !MBEDTLS_DEPRECATED_REMOVED */
    conf->sig_algs = sig_algs;
}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/*
 * Set the allowed elliptic curves
 *
 * mbedtls_ssl_setup() takes the provided list
 * and translates it to a list of IANA TLS group identifiers,
 * stored in ssl->handshake->group_list.
 *
 */
void mbedtls_ssl_conf_curves(mbedtls_ssl_config *conf,
                             const mbedtls_ecp_group_id *curve_list)
{
    conf->curve_list = curve_list;
    conf->group_list = NULL;
}
#endif /* MBEDTLS_DEPRECATED_REMOVED */

/*
 * Set the allowed groups
 */
void mbedtls_ssl_conf_groups(mbedtls_ssl_config *conf,
                             const uint16_t *group_list)
{
#if defined(MBEDTLS_ECP_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
    conf->curve_list = NULL;
#endif
    conf->group_list = group_list;
}

/* A magic value for `ssl->hostname` indicating that
 * mbedtls_ssl_set_hostname() has been called with `NULL`.
 * If mbedtls_ssl_set_hostname() has never been called on `ssl`, then
 * `ssl->hostname == NULL`. */
static const char *const ssl_hostname_skip_cn_verification = "";

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
/** Whether mbedtls_ssl_set_hostname() has been called.
 *
 * \param[in]   ssl     SSL context
 *
 * \return \c 1 if mbedtls_ssl_set_hostname() has been called on \p ssl
 *         (including `mbedtls_ssl_set_hostname(ssl, NULL)`),
 *         otherwise \c 0.
 */
int mbedtls_ssl_has_set_hostname_been_called(
    const mbedtls_ssl_context *ssl)
{
    return ssl->hostname != NULL;
}
#endif

/*
 * Exported for ssl_client.c when SNI is enabled.  When SNI is off the only
 * in-file caller is get_hostname_for_verification() which is guarded by
 * MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED, so compile the function out
 * entirely when neither macro is defined.
 */
const char *mbedtls_ssl_get_hostname_pointer(const mbedtls_ssl_context *ssl)
{
    if (ssl->hostname == ssl_hostname_skip_cn_verification) {
        return NULL;
    }
    return ssl->hostname;
}

void mbedtls_ssl_free_hostname(mbedtls_ssl_context *ssl)
{
    if (ssl->hostname != NULL &&
        ssl->hostname != ssl_hostname_skip_cn_verification) {
        mbedtls_zeroize_and_free(ssl->hostname, strlen(ssl->hostname));
    }
    ssl->hostname = NULL;
}

int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* Initialize to suppress unnecessary compiler warning */
    size_t hostname_len = 0;

    /* Check if new hostname is valid before
     * making any change to current one */
    if (hostname != NULL) {
        hostname_len = strlen(hostname);

        if (hostname_len > MBEDTLS_SSL_MAX_HOST_NAME_LEN) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
        }
    }

    /* Now it's clear that we will overwrite the old hostname,
     * so we can free it safely */
    mbedtls_ssl_free_hostname(ssl);

    if (hostname == NULL) {
        /* Passing NULL as hostname clears the old one, but leaves a
         * special marker to indicate that mbedtls_ssl_set_hostname()
         * has been called. */
        /* ssl->hostname should be const, but isn't. We won't actually
         * write to the buffer, so it's ok to cast away the const. */
        ssl->hostname = (char *) ssl_hostname_skip_cn_verification;
    } else {
        ssl->hostname = mbedtls_calloc(1, hostname_len + 1);
        if (ssl->hostname == NULL) {
            /* mbedtls_ssl_set_hostname() has been called, but unsuccessfully.
             * Leave ssl->hostname in the same state as if the function had
             * not been called, i.e. a null pointer. */
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
        }

        memcpy(ssl->hostname, hostname, hostname_len);

        ssl->hostname[hostname_len] = '\0';
    }

    MINTLS_RETURN(0);
}

void mbedtls_ssl_conf_sni(mbedtls_ssl_config *conf,
                          int (*f_sni)(void *, mbedtls_ssl_context *,
                                       const unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                          void *p_sni)
{
    conf->f_sni = f_sni;
    conf->p_sni = p_sni;
}

int mbedtls_ssl_conf_alpn_protocols(mbedtls_ssl_config *conf, const char **protos, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t cur_len, tot_len;
    const char **p;

    /*
     * RFC 7301 3.1: "Empty strings MUST NOT be included and byte strings
     * MUST NOT be truncated."
     * We check lengths now rather than later.
     */
    tot_len = 0;
    for (p = protos; *p != NULL; p++) {
        cur_len = strlen(*p);
        tot_len += cur_len;

        if ((cur_len == 0) ||
            (cur_len > MBEDTLS_SSL_MAX_ALPN_NAME_LEN) ||
            (tot_len > MBEDTLS_SSL_MAX_ALPN_LIST_LEN)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
        }
    }

    conf->alpn_list = protos;

    MINTLS_RETURN(0);
}

const char *mbedtls_ssl_get_alpn_protocol(const mbedtls_ssl_context *ssl)
{
    return ssl->alpn_chosen;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_ssl_conf_max_version(mbedtls_ssl_config *conf, int major, int minor)
{
    conf->max_tls_version = (mbedtls_ssl_protocol_version) ((major << 8) | minor);
}

void mbedtls_ssl_conf_min_version(mbedtls_ssl_config *conf, int major, int minor)
{
    conf->min_tls_version = (mbedtls_ssl_protocol_version) ((major << 8) | minor);
}
#endif /* MBEDTLS_DEPRECATED_REMOVED */

void mbedtls_ssl_conf_extended_master_secret(mbedtls_ssl_config *conf, char ems)
{
    conf->extended_ms = ems;
}

void mbedtls_ssl_conf_legacy_renegotiation(mbedtls_ssl_config *conf, int allow_legacy)
{
    conf->allow_legacy_renegotiation = allow_legacy;
}

void mbedtls_ssl_set_export_keys_cb(mbedtls_ssl_context *ssl,
                                    mbedtls_ssl_export_keys_t *f_export_keys,
                                    void *p_export_keys)
{
    ssl->f_export_keys = f_export_keys;
    ssl->p_export_keys = p_export_keys;
}

/*
 * SSL get accessors
 */
uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl)
{
    if (ssl->session != NULL) {
        return ssl->session->verify_result;
    }

    if (ssl->session_negotiate != NULL) {
        return ssl->session_negotiate->verify_result;
    }

    return 0xFFFFFFFF;
}

int mbedtls_ssl_get_ciphersuite_id_from_ssl(const mbedtls_ssl_context *ssl)
{
    if (ssl == NULL || ssl->session == NULL) {
        return 0;
    }

    return ssl->session->ciphersuite;
}

const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl)
{
    if (ssl == NULL || ssl->session == NULL) {
        return NULL;
    }

    return mbedtls_ssl_get_ciphersuite_name(ssl->session->ciphersuite);
}

const char *mbedtls_ssl_get_version(const mbedtls_ssl_context *ssl)
{

    switch (ssl->tls_version) {
        case MBEDTLS_SSL_VERSION_TLS1_2:
            return "TLSv1.2";
        case MBEDTLS_SSL_VERSION_TLS1_3:
            return "TLSv1.3";
        default:
            return "unknown";
    }
}

int mbedtls_ssl_get_max_out_record_payload(const mbedtls_ssl_context *ssl)
{
    size_t max_len = MBEDTLS_SSL_OUT_CONTENT_LEN;

    (void) ssl;

    if (ssl->transform_out != NULL &&
        ssl->transform_out->tls_version == MBEDTLS_SSL_VERSION_TLS1_3) {
        /*
         * In TLS 1.3 case, when records are protected, `max_len` as computed
         * above is the maximum length of the TLSInnerPlaintext structure that
         * along the plaintext payload contains the inner content type (one byte)
         * and some zero padding. Given the algorithm used for padding
         * in mbedtls_ssl_encrypt_buf(), compute the maximum length for
         * the plaintext payload. Round down to a multiple of
         * MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY and
         * subtract 1.
         */
        max_len = ((max_len / MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY) *
                   MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY) - 1;
    }

    ((void) ssl);

    return (int) max_len;
}

int mbedtls_ssl_get_max_in_record_payload(const mbedtls_ssl_context *ssl)
{
    size_t max_len = MBEDTLS_SSL_IN_CONTENT_LEN;

    (void) ssl;

    return (int) max_len;
}

const mbedtls_x509_crt *mbedtls_ssl_get_peer_cert(const mbedtls_ssl_context *ssl)
{
    if (ssl == NULL || ssl->session == NULL) {
        return NULL;
    }

    return ssl->session->peer_cert;
}

int mbedtls_ssl_get_session(const mbedtls_ssl_context *ssl,
                            mbedtls_ssl_session *dst, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    if (ssl == NULL ||
        dst == NULL ||
        ssl->session == NULL ||
        ssl->conf->endpoint != MBEDTLS_SSL_IS_CLIENT) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    /* Since Mbed TLS 3.0, mbedtls_ssl_get_session() is no longer
     * idempotent: Each session can only be exported once.
     *
     * (This is in preparation for TLS 1.3 support where we will
     * need the ability to export multiple sessions (aka tickets),
     * which will be achieved by calling mbedtls_ssl_get_session()
     * multiple times until it fails.)
     *
     * Check whether we have already exported the current session,
     * and fail if so.
     */
    if (ssl->session->exported == 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }

    ret = mbedtls_ssl_session_copy(dst, ssl->session, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    /* Remember that we've exported the session. */
    ssl->session->exported = 1;
    MINTLS_RETURN(0);
}

/* Serialization of TLS 1.2 sessions
 *
 * For more detail, see the description of ssl_session_save().
 */
size_t ssl_tls12_session_save(const mbedtls_ssl_session *session,
                                     unsigned char *buf,
                                     size_t buf_len)
{
    unsigned char *p = buf;
    size_t used = 0;

    uint64_t start;
    size_t cert_len;

    /*
     * Time
     */
    used += 8;

    if (used <= buf_len) {
        start = (uint64_t) session->start;

        MBEDTLS_PUT_UINT64_BE(start, p, 0);
        p += 8;
    }

    /*
     * Basic mandatory fields
     */
    used += 1 /* id_len */
            + sizeof(session->id)
            + sizeof(session->master)
            + 4; /* verify_result */

    if (used <= buf_len) {
        *p++ = MBEDTLS_BYTE_0(session->id_len);
        memcpy(p, session->id, 32);
        p += 32;

        memcpy(p, session->master, 48);
        p += 48;

        MBEDTLS_PUT_UINT32_BE(session->verify_result, p, 0);
        p += 4;
    }

    /*
     * Peer's end-entity certificate
     */
    if (session->peer_cert == NULL) {
        cert_len = 0;
    } else {
        cert_len = session->peer_cert->raw.len;
    }

    used += 3 + cert_len;

    if (used <= buf_len) {
        *p++ = MBEDTLS_BYTE_2(cert_len);
        *p++ = MBEDTLS_BYTE_1(cert_len);
        *p++ = MBEDTLS_BYTE_0(cert_len);

        if (session->peer_cert != NULL) {
            memcpy(p, session->peer_cert->raw.p, cert_len);
            p += cert_len;
        }
    }

    /*
     * Session ticket if any, plus associated data
     */

    /*
     * Misc extension-related info
     */

    return used;
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_tls12_session_load(mbedtls_ssl_session *session,
                                  const unsigned char *buf,
                                  size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    uint64_t start;
    size_t cert_len;

    const unsigned char *p = buf;
    const unsigned char * const end = buf + len;

    /*
     * Time
     */
    if (8 > (size_t) (end - p)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    start = MBEDTLS_GET_UINT64_BE(p, 0);
    p += 8;

    session->start = (mbedtls_time_t) start;

    /*
     * Basic mandatory fields
     */
    if (1 + 32 + 48 + 4 > (size_t) (end - p)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    session->id_len = *p++;
    memcpy(session->id, p, 32);
    p += 32;

    memcpy(session->master, p, 48);
    p += 48;

    session->verify_result = MBEDTLS_GET_UINT32_BE(p, 0);
    p += 4;

    /* Immediately clear invalid pointer values that have been read, in case
     * we exit early before we replaced them with valid ones. */
    session->peer_cert = NULL;

    /*
     * Peer certificate
     */
    /* Deserialize CRT from the end of the ticket. */
    if (3 > (size_t) (end - p)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    cert_len = MBEDTLS_GET_UINT24_BE(p, 0);
    p += 3;

    if (cert_len != 0) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        if (cert_len > (size_t) (end - p)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
        }

        session->peer_cert = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));

        if (session->peer_cert == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
        }

        mbedtls_x509_crt_init(session->peer_cert);

        if ((ret = mbedtls_x509_crt_parse_der(session->peer_cert,
                                              p, cert_len, diagnostics)) != 0) {
            mbedtls_x509_crt_free(session->peer_cert);
            mbedtls_free(session->peer_cert);
            session->peer_cert = NULL;
            MINTLS_RETURN(ret);
        }

        p += cert_len;
    }

    /*
     * Session ticket and associated data
     */

    /*
     * Misc extension-related info
     */

    /* Done, should have consumed entire buffer */
    if (p != end) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(0);
}

/*
 * Define ticket header determining Mbed TLS version
 * and structure of the ticket.
 */

/*
 * Define bitflag determining compile-time settings influencing
 * structure of serialized SSL sessions.
 */

#define SSL_SERIALIZED_SESSION_CONFIG_TIME 1

#define SSL_SERIALIZED_SESSION_CONFIG_CRT 1

#define SSL_SERIALIZED_SESSION_CONFIG_KEEP_PEER_CRT 1

#define SSL_SERIALIZED_SESSION_CONFIG_CLIENT_TICKET 0

#define SSL_SERIALIZED_SESSION_CONFIG_MFL 0

#define SSL_SERIALIZED_SESSION_CONFIG_ETM 0

#define SSL_SERIALIZED_SESSION_CONFIG_TICKET 0

#define SSL_SERIALIZED_SESSION_CONFIG_SNI 1

#define SSL_SERIALIZED_SESSION_CONFIG_EARLY_DATA 0

#define SSL_SERIALIZED_SESSION_CONFIG_RECORD_SIZE 0

#define SSL_SERIALIZED_SESSION_CONFIG_ALPN 0

#define SSL_SERIALIZED_SESSION_CONFIG_TIME_BIT          0
#define SSL_SERIALIZED_SESSION_CONFIG_CRT_BIT           1
#define SSL_SERIALIZED_SESSION_CONFIG_CLIENT_TICKET_BIT 2
#define SSL_SERIALIZED_SESSION_CONFIG_MFL_BIT           3
#define SSL_SERIALIZED_SESSION_CONFIG_ETM_BIT           4
#define SSL_SERIALIZED_SESSION_CONFIG_TICKET_BIT        5
#define SSL_SERIALIZED_SESSION_CONFIG_KEEP_PEER_CRT_BIT 6
#define SSL_SERIALIZED_SESSION_CONFIG_SNI_BIT           7
#define SSL_SERIALIZED_SESSION_CONFIG_EARLY_DATA_BIT    8
#define SSL_SERIALIZED_SESSION_CONFIG_RECORD_SIZE_BIT   9
#define SSL_SERIALIZED_SESSION_CONFIG_ALPN_BIT          10

#define SSL_SERIALIZED_SESSION_CONFIG_BITFLAG                           \
    ((uint16_t) (                                                      \
         (SSL_SERIALIZED_SESSION_CONFIG_TIME << SSL_SERIALIZED_SESSION_CONFIG_TIME_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_CRT << SSL_SERIALIZED_SESSION_CONFIG_CRT_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_CLIENT_TICKET << \
             SSL_SERIALIZED_SESSION_CONFIG_CLIENT_TICKET_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_MFL << SSL_SERIALIZED_SESSION_CONFIG_MFL_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_ETM << SSL_SERIALIZED_SESSION_CONFIG_ETM_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_TICKET << SSL_SERIALIZED_SESSION_CONFIG_TICKET_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_KEEP_PEER_CRT << \
             SSL_SERIALIZED_SESSION_CONFIG_KEEP_PEER_CRT_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_SNI << SSL_SERIALIZED_SESSION_CONFIG_SNI_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_EARLY_DATA << \
             SSL_SERIALIZED_SESSION_CONFIG_EARLY_DATA_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_RECORD_SIZE << \
             SSL_SERIALIZED_SESSION_CONFIG_RECORD_SIZE_BIT) | \
         (SSL_SERIALIZED_SESSION_CONFIG_ALPN << \
             SSL_SERIALIZED_SESSION_CONFIG_ALPN_BIT)))

static const unsigned char ssl_serialized_session_header[] = {
    MBEDTLS_VERSION_MAJOR,
    MBEDTLS_VERSION_MINOR,
    MBEDTLS_VERSION_PATCH,
    MBEDTLS_BYTE_1(SSL_SERIALIZED_SESSION_CONFIG_BITFLAG),
    MBEDTLS_BYTE_0(SSL_SERIALIZED_SESSION_CONFIG_BITFLAG),
};

/*
 * Serialize a session in the following format:
 * (in the presentation language of TLS, RFC 8446 section 3)
 *
 * TLS 1.2 session:
 *
 * struct {
 * #if defined(MBEDTLS_SSL_SESSION_TICKETS)
 *    opaque ticket<0..2^24-1>;       // length 0 means no ticket
 *    uint32 ticket_lifetime;
 * #endif
 * } ClientOnlyData;
 *
 * struct {
 * #if defined(MBEDTLS_HAVE_TIME)
 *    uint64 start_time;
 * #endif
 *     uint8 session_id_len;           // at most 32
 *     opaque session_id[32];
 *     opaque master[48];              // fixed length in the standard
 *     uint32 verify_result;
 * #if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
 *    opaque peer_cert<0..2^24-1>;    // length 0 means no peer cert
 * #else
 *    uint8 peer_cert_digest_type;
 *    opaque peer_cert_digest<0..2^8-1>
 * #endif
 *     select (endpoint) {
 *         case client: ClientOnlyData;
 *         case server: uint64 ticket_creation_time;
 *     };
 * #if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
 *    uint8 mfl_code;                 // up to 255 according to standard
 * #endif
 * #if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
 *    uint8 encrypt_then_mac;         // 0 or 1
 * #endif
 * } serialized_session_tls12;
 *
 *
 * TLS 1.3 Session:
 *
 * struct {
 * #if defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
 *    opaque hostname<0..2^16-1>;
 * #endif
 * #if defined(MBEDTLS_HAVE_TIME)
 *    uint64 ticket_reception_time;
 * #endif
 *    uint32 ticket_lifetime;
 *    opaque ticket<1..2^16-1>;
 * } ClientOnlyData;
 *
 * struct {
 *    uint32 ticket_age_add;
 *    uint8 ticket_flags;
 *    opaque resumption_key<0..255>;
 * #if defined(MBEDTLS_SSL_EARLY_DATA)
 *    uint32 max_early_data_size;
 * #endif
 * #if defined(MBEDTLS_SSL_RECORD_SIZE_LIMIT)
 *    uint16 record_size_limit;
 * #endif
 *    select ( endpoint ) {
 *         case client: ClientOnlyData;
 *         case server:
 * #if defined(MBEDTLS_HAVE_TIME)
 *                      uint64 ticket_creation_time;
 * #endif
 * #if defined(MBEDTLS_SSL_EARLY_DATA) && defined(MBEDTLS_SSL_ALPN)
 *                      opaque ticket_alpn<0..256>;
 * #endif
 *     };
 * } serialized_session_tls13;
 *
 *
 * SSL session:
 *
 * struct {
 *
 *    opaque mbedtls_version[3];   // library version: major, minor, patch
 *    opaque session_format[2];    // library-version specific 16-bit field
 *                                 // determining the format of the remaining
 *                                 // serialized data.
 *
 *          Note: When updating the format, remember to keep
 *          these version+format bytes.
 *
 *                                 // In this version, `session_format` determines
 *                                 // the setting of those compile-time
 *                                 // configuration options which influence
 *                                 // the structure of mbedtls_ssl_session.
 *
 *    uint8_t minor_ver;           // Protocol minor version. Possible values:
 *                                 // - TLS 1.2 (0x0303)
 *                                 // - TLS 1.3 (0x0304)
 *    uint8_t endpoint;
 *    uint16_t ciphersuite;
 *
 *    select (serialized_session.tls_version) {
 *
 *      case MBEDTLS_SSL_VERSION_TLS1_2:
 *        serialized_session_tls12 data;
 *      case MBEDTLS_SSL_VERSION_TLS1_3:
 *        serialized_session_tls13 data;
 *
 *   };
 *
 * } serialized_session;
 *
 */

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_session_save(const mbedtls_ssl_session *session,
                            unsigned char omit_header,
                            unsigned char *buf,
                            size_t buf_len,
                            size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = buf;
    size_t used = 0;
    size_t remaining_len;
    if (session == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    if (!omit_header) {
        /*
         * Add Mbed TLS version identifier
         */
        used += sizeof(ssl_serialized_session_header);

        if (used <= buf_len) {
            memcpy(p, ssl_serialized_session_header,
                   sizeof(ssl_serialized_session_header));
            p += sizeof(ssl_serialized_session_header);
        }
    }

    /*
     * TLS version identifier, endpoint, ciphersuite
     */
    used += 1    /* TLS version */
            + 1  /* endpoint */
            + 2; /* ciphersuite */
    if (used <= buf_len) {
        *p++ = MBEDTLS_BYTE_0(session->tls_version);
        *p++ = session->endpoint;
        MBEDTLS_PUT_UINT16_BE(session->ciphersuite, p, 0);
        p += 2;
    }

    /* Forward to version-specific serialization routine. */
    remaining_len = (buf_len >= used) ? buf_len - used : 0;
    switch (session->tls_version) {
        case MBEDTLS_SSL_VERSION_TLS1_2:
            used += ssl_tls12_session_save(session, p, remaining_len);
            break;

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE);
    }

    *olen = used;
    if (used > buf_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL);
    }

    MINTLS_RETURN(0);
}

/*
 * Public wrapper for ssl_session_save()
 */
int mbedtls_ssl_session_save(const mbedtls_ssl_session *session,
                             unsigned char *buf,
                             size_t buf_len,
                             size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ssl_session_save(session, 0, buf, buf_len, olen, diagnostics));
}

/*
 * Deserialize session, see mbedtls_ssl_session_save() for format.
 *
 * This internal version is wrapped by a public function that cleans up in
 * case of error, and has an extra option omit_header.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_session_load(mbedtls_ssl_session *session,
                            unsigned char omit_header,
                            const unsigned char *buf,
                            size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const unsigned char *p = buf;
    const unsigned char * const end = buf + len;
    size_t remaining_len;

    if (session == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    if (!omit_header) {
        /*
         * Check Mbed TLS version identifier
         */

        if ((size_t) (end - p) < sizeof(ssl_serialized_session_header)) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
        }

        if (memcmp(p, ssl_serialized_session_header,
                   sizeof(ssl_serialized_session_header)) != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_VERSION_MISMATCH);
        }
        p += sizeof(ssl_serialized_session_header);
    }

    /*
     * TLS version identifier, endpoint, ciphersuite
     */
    if (4 > (size_t) (end - p)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }
    session->tls_version = (mbedtls_ssl_protocol_version) (0x0300 | *p++);
    session->endpoint = *p++;
    session->ciphersuite = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    /* Dispatch according to TLS version. */
    remaining_len = (size_t) (end - p);
    switch (session->tls_version) {
        case MBEDTLS_SSL_VERSION_TLS1_2:
            MINTLS_RETURN(ssl_tls12_session_load(session, p, remaining_len, diagnostics));

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }
}

/*
 * Deserialize session: public wrapper for error cleaning
 */
int mbedtls_ssl_session_load(mbedtls_ssl_session *session,
                             const unsigned char *buf,
                             size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = ssl_session_load(session, 0, buf, len, diagnostics);

    if (ret != 0) {
        mbedtls_ssl_session_free(session);
    }

    MINTLS_RETURN(ret);
}

/*
 * Perform a single step of the SSL handshake
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_prepare_handshake_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /*
     * We may have not been able to send to the peer all the handshake data
     * that were written into the output buffer by the previous handshake step,
     * if the write to the network callback returned with the
     * #MBEDTLS_ERR_SSL_WANT_WRITE error code.
     * We proceed to the next handshake step only when all data from the
     * previous one have been sent to the peer, thus we make sure that this is
     * the case here by calling `mbedtls_ssl_flush_output()`. The function may
     * return with the #MBEDTLS_ERR_SSL_WANT_WRITE error code in which case
     * we have to wait before to go ahead.
     * In the case of TLS 1.3, handshake step handlers do not send data to the
     * peer. Data are only sent here and through
     * `mbedtls_ssl_handle_pending_alert` in case an error that triggered an
     * alert occurred.
     */
    if ((ret = mbedtls_ssl_flush_output(ssl, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(ret);
}

int mbedtls_ssl_handshake_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ssl            == NULL                       ||
        ssl->conf      == NULL                       ||
        ssl->handshake == NULL                       ||
        ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    ret = ssl_prepare_handshake_step(ssl, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    ret = mbedtls_ssl_handle_pending_alert(ssl, diagnostics);
    if (ret != 0) {
        goto cleanup;
    }

    /* If ssl->conf->endpoint is not one of MBEDTLS_SSL_IS_CLIENT or
     * MBEDTLS_SSL_IS_SERVER, this is the return code we give */
    ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_BAD_INPUT_DATA);

    if (ssl->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("client state: %s",
                                  mbedtls_ssl_states_str((mbedtls_ssl_states) ssl->state)));

        switch (ssl->state) {
            case MBEDTLS_SSL_HELLO_REQUEST:
                mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_HELLO);
                ret = 0;
                break;

            case MBEDTLS_SSL_CLIENT_HELLO:
                ret = mbedtls_ssl_write_client_hello(ssl, diagnostics);
                break;

            default:
                ret = mbedtls_ssl_handshake_client_step(ssl, diagnostics);
        }
    }

    if (ret != 0) {
        /* handshake_step return error. And it is same
         * with alert_reason.
         */
        if (ssl->send_alert) {
            ret = mbedtls_ssl_handle_pending_alert(ssl, diagnostics);
            goto cleanup;
        }
    }

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Perform the SSL handshake
 */
int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;

    /* Sanity checks */

    if (ssl == NULL || ssl->conf == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> handshake"));

    /* Main handshake loop */
    while (ssl->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        ret = mbedtls_ssl_handshake_step(ssl, diagnostics);

        if (ret != 0) {
            break;
        }
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= handshake"));

    MINTLS_RETURN(ret);
}

void mbedtls_ssl_handshake_free(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

    if (handshake == NULL) {
        return;
    }

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    if (ssl->handshake->group_list_heap_allocated) {
        mbedtls_free((void *) handshake->group_list);
    }
    handshake->group_list = NULL;
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    if (ssl->handshake->sig_algs_heap_allocated) {
        mbedtls_free((void *) handshake->sig_algs);
    }
    handshake->sig_algs = NULL;
#endif /* MBEDTLS_DEPRECATED_REMOVED */
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

    mbedtls_md_free(&handshake->fin_sha256);
    mbedtls_md_free(&handshake->fin_sha384);

#if !defined(MBEDTLS_USE_PSA_CRYPTO) && \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED)
    mbedtls_ecdh_free(&handshake->ecdh_ctx);
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_WITH_ECDSA_ANY_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    /* explicit void pointer cast for buggy MS compiler */
    mbedtls_free((void *) handshake->curves_tls_id);
#endif

    /*
     * Free only the linked list wrapper, not the keys themselves
     * since the belong to the SNI callback
     */
    ssl_key_cert_free(handshake->sni_key_cert);

    /* mbedtls_platform_zeroize MUST be last one in this function */
    mbedtls_platform_zeroize(handshake,
                             sizeof(mbedtls_ssl_handshake_params));
}

void mbedtls_ssl_session_free(mbedtls_ssl_session *session)
{
    if (session == NULL) {
        return;
    }

    ssl_clear_peer_cert(session);

    mbedtls_platform_zeroize(session, sizeof(mbedtls_ssl_session));

    /* Set verify_result to -1u to indicate 'result not available'. */
    session->verify_result = 0xFFFFFFFF;
}

/*
 * Free an SSL context
 */
void mbedtls_ssl_free(mbedtls_ssl_context *ssl)
{
    if (ssl == NULL) {
        return;
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> free"));

    if (ssl->out_buf != NULL) {
        size_t out_buf_len = MBEDTLS_SSL_OUT_BUFFER_LEN;

        mbedtls_zeroize_and_free(ssl->out_buf, out_buf_len);
        ssl->out_buf = NULL;
    }

    if (ssl->in_buf != NULL) {
        size_t in_buf_len = MBEDTLS_SSL_IN_BUFFER_LEN;

        mbedtls_zeroize_and_free(ssl->in_buf, in_buf_len);
        ssl->in_buf = NULL;
    }

    if (ssl->transform) {
        mbedtls_ssl_transform_free(ssl->transform);
        mbedtls_free(ssl->transform);
    }

    if (ssl->handshake) {
        mbedtls_ssl_handshake_free(ssl);
        mbedtls_free(ssl->handshake);

        mbedtls_ssl_transform_free(ssl->transform_negotiate);
        mbedtls_free(ssl->transform_negotiate);

        mbedtls_ssl_session_free(ssl->session_negotiate);
        mbedtls_free(ssl->session_negotiate);
    }

    if (ssl->session) {
        mbedtls_ssl_session_free(ssl->session);
        mbedtls_free(ssl->session);
    }

    mbedtls_ssl_free_hostname(ssl);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= free"));

    /* Actually clear after last debug message */
    mbedtls_platform_zeroize(ssl, sizeof(mbedtls_ssl_context));
}

/*
 * Initialize mbedtls_ssl_config
 */
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf)
{
    memset(conf, 0, sizeof(mbedtls_ssl_config));
}

/* The selection should be the same as mbedtls_x509_crt_profile_default in
 * x509_crt.c, plus Montgomery curves for ECDHE. Here, the order matters:
 * curves with a lower resource usage come first.
 * See the documentation of mbedtls_ssl_conf_curves() for what we promise
 * about this list.
 */
static const uint16_t ssl_preset_default_groups[] = {
    MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1,
    MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1,
    MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1,
#if defined(PSA_WANT_ALG_FFDH)
    MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE2048,
    MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE3072,
    MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE4096,
    MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE6144,
    MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE8192,
#endif
    MBEDTLS_SSL_IANA_TLS_GROUP_NONE
};

static const int ssl_preset_suiteb_ciphersuites[] = {
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
    0
};

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

/* NOTICE:
 *   For ssl_preset_*_sig_algs and ssl_tls12_preset_*_sig_algs, the following
 *   rules SHOULD be upheld.
 *   - No duplicate entries.
 *   - But if there is a good reason, do not change the order of the algorithms.
 *   - ssl_tls12_preset* is for TLS 1.2 use only.
 *   - ssl_preset_* is for TLS 1.3 only or hybrid TLS 1.3/1.2 handshakes.
 */
static const uint16_t ssl_preset_default_sig_algs[] = {

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) && \
    defined(MBEDTLS_MD_CAN_SHA256) && \
    defined(PSA_WANT_ECC_SECP_R1_256)
    MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256,
    // == MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA256)
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) && \
    defined(MBEDTLS_MD_CAN_SHA384) && \
    defined(PSA_WANT_ECC_SECP_R1_384)
    MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384,
    // == MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA384)
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) && \
    defined(MBEDTLS_MD_CAN_SHA512) && \
    defined(PSA_WANT_ECC_SECP_R1_521)
    MBEDTLS_TLS1_3_SIG_ECDSA_SECP521R1_SHA512,
    // == MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA512)
#endif

    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512,

    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384,

    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256,

    MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA512,

    MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA384,

    MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA256,

    MBEDTLS_TLS_SIG_NONE
};

/* NOTICE: see above */
static const uint16_t ssl_tls12_preset_default_sig_algs[] = {

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA512),
#endif
    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512,
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_RSA, MBEDTLS_SSL_HASH_SHA512),

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA384),
#endif
    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384,
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_RSA, MBEDTLS_SSL_HASH_SHA384),

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA256),
#endif
    MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256,
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_RSA, MBEDTLS_SSL_HASH_SHA256),

    MBEDTLS_TLS_SIG_NONE
};

/* NOTICE: see above */
static const uint16_t ssl_preset_suiteb_sig_algs[] = {

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) && \
    defined(MBEDTLS_MD_CAN_SHA256) && \
    defined(MBEDTLS_ECP_HAVE_SECP256R1)
    MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256,
    // == MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA256)
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) && \
    defined(MBEDTLS_MD_CAN_SHA384) && \
    defined(MBEDTLS_ECP_HAVE_SECP384R1)
    MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384,
    // == MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA384)
#endif

    MBEDTLS_TLS_SIG_NONE
};

/* NOTICE: see above */
static const uint16_t ssl_tls12_preset_suiteb_sig_algs[] = {

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA256),
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
    MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(MBEDTLS_SSL_SIG_ECDSA, MBEDTLS_SSL_HASH_SHA384),
#endif

    MBEDTLS_TLS_SIG_NONE
};

#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

static const uint16_t ssl_preset_suiteb_groups[] = {
    MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1,
    MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1,
    MBEDTLS_SSL_IANA_TLS_GROUP_NONE
};

/*
 * Load default in mbedtls_ssl_config
 */
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,
                                int endpoint, int transport, int preset)
{

    /* Use the functions here so that they are covered in tests,
     * but otherwise access member directly for efficiency */
    mbedtls_ssl_conf_endpoint(conf, endpoint);
    mbedtls_ssl_conf_transport(conf, transport);

    /*
     * Things that are common to all presets
     */
    if (endpoint == MBEDTLS_SSL_IS_CLIENT) {
        conf->authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
    }

    conf->extended_ms = MBEDTLS_SSL_EXTENDED_MS_ENABLED;

    if (transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM) {
        conf->min_tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
        conf->max_tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
    } else {
        conf->min_tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
        conf->max_tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
    }

    /*
     * Preset-specific defaults
     */
    switch (preset) {
        /*
         * NSA Suite B
         */
        case MBEDTLS_SSL_PRESET_SUITEB:

            conf->ciphersuite_list = ssl_preset_suiteb_ciphersuites;

            conf->cert_profile = &mbedtls_x509_crt_profile_suiteb;

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
            if (mbedtls_ssl_conf_is_tls12_only(conf)) {
                conf->sig_algs = ssl_tls12_preset_suiteb_sig_algs;
            } else
            conf->sig_algs = ssl_preset_suiteb_sig_algs;
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if defined(MBEDTLS_ECP_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
            conf->curve_list = NULL;
#endif
            conf->group_list = ssl_preset_suiteb_groups;
            break;

        /*
         * Default
         */
        default:

            conf->ciphersuite_list = mbedtls_ssl_list_ciphersuites();

            conf->cert_profile = &mbedtls_x509_crt_profile_default;

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
            if (mbedtls_ssl_conf_is_tls12_only(conf)) {
                conf->sig_algs = ssl_tls12_preset_default_sig_algs;
            } else
            conf->sig_algs = ssl_preset_default_sig_algs;
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if defined(MBEDTLS_ECP_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
            conf->curve_list = NULL;
#endif
            conf->group_list = ssl_preset_default_groups;

    }

    return 0;
}

/*
 * Free mbedtls_ssl_config
 */
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf)
{
    if (conf == NULL) {
        return;
    }

    ssl_key_cert_free(conf->key_cert);

    mbedtls_platform_zeroize(conf, sizeof(mbedtls_ssl_config));
}

/*
 * Convert between MBEDTLS_PK_XXX and SSL_SIG_XXX
 */
unsigned char mbedtls_ssl_sig_from_pk(mbedtls_pk_context *pk)
{
    if (mbedtls_pk_can_do(pk, MBEDTLS_PK_RSA)) {
        return MBEDTLS_SSL_SIG_RSA;
    }
#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED)
    if (mbedtls_pk_can_do(pk, MBEDTLS_PK_ECDSA)) {
        return MBEDTLS_SSL_SIG_ECDSA;
    }
#endif
    return MBEDTLS_SSL_SIG_ANON;
}

unsigned char mbedtls_ssl_sig_from_pk_alg(mbedtls_pk_type_t type)
{
    switch (type) {
        case MBEDTLS_PK_RSA:
            return MBEDTLS_SSL_SIG_RSA;
        case MBEDTLS_PK_ECDSA:
        case MBEDTLS_PK_ECKEY:
            return MBEDTLS_SSL_SIG_ECDSA;
        default:
            return MBEDTLS_SSL_SIG_ANON;
    }
}

mbedtls_pk_type_t mbedtls_ssl_pk_alg_from_sig(unsigned char sig)
{
    switch (sig) {
        case MBEDTLS_SSL_SIG_RSA:
            return MBEDTLS_PK_RSA;
#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED)
        case MBEDTLS_SSL_SIG_ECDSA:
            return MBEDTLS_PK_ECDSA;
#endif
        default:
            return MBEDTLS_PK_NONE;
    }
}

/*
 * Convert from MBEDTLS_SSL_HASH_XXX to MBEDTLS_MD_XXX
 */
mbedtls_md_type_t mbedtls_ssl_md_alg_from_hash(unsigned char hash)
{
    switch (hash) {
        case MBEDTLS_SSL_HASH_SHA1:
            return MBEDTLS_MD_SHA1;
        case MBEDTLS_SSL_HASH_SHA256:
            return MBEDTLS_MD_SHA256;
        case MBEDTLS_SSL_HASH_SHA384:
            return MBEDTLS_MD_SHA384;
        case MBEDTLS_SSL_HASH_SHA512:
            return MBEDTLS_MD_SHA512;
        default:
            return MBEDTLS_MD_NONE;
    }
}

/*
 * Convert from MBEDTLS_MD_XXX to MBEDTLS_SSL_HASH_XXX
 */
unsigned char mbedtls_ssl_hash_from_md_alg(int md)
{
    switch (md) {
        case MBEDTLS_MD_SHA1:
            return MBEDTLS_SSL_HASH_SHA1;
        case MBEDTLS_MD_SHA256:
            return MBEDTLS_SSL_HASH_SHA256;
        case MBEDTLS_MD_SHA384:
            return MBEDTLS_SSL_HASH_SHA384;
        case MBEDTLS_MD_SHA512:
            return MBEDTLS_SSL_HASH_SHA512;
        default:
            return MBEDTLS_SSL_HASH_NONE;
    }
}

/*
 * Check if a curve proposed by the peer is in our list.
 * Return 0 if we're willing to use it, -1 otherwise.
 */
int mbedtls_ssl_check_curve_tls_id(const mbedtls_ssl_context *ssl, uint16_t tls_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const uint16_t *group_list = mbedtls_ssl_get_groups(ssl);

    if (group_list == NULL) {
        MINTLS_RETURN_ERROR(-1);
    }

    for (; *group_list != 0; group_list++) {
        if (*group_list == tls_id) {
            MINTLS_RETURN(0);
        }
    }

    MINTLS_RETURN_ERROR(-1);
}

/*
 * Same as mbedtls_ssl_check_curve_tls_id() but with a mbedtls_ecp_group_id.
 */
int mbedtls_ssl_check_curve(const mbedtls_ssl_context *ssl, mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    uint16_t tls_id = mbedtls_ssl_get_tls_id_from_ecp_group_id(grp_id);

    if (tls_id == 0) {
        MINTLS_RETURN_ERROR(-1);
    }

    MINTLS_RETURN(mbedtls_ssl_check_curve_tls_id(ssl, tls_id, diagnostics));
}

static const struct {
    uint16_t tls_id;
    mbedtls_ecp_group_id ecp_group_id;
} tls_id_match_table[] =
{
    { 25, MBEDTLS_ECP_DP_SECP521R1 },
    { 24, MBEDTLS_ECP_DP_SECP384R1 },
    { 23, MBEDTLS_ECP_DP_SECP256R1 },
    { 0, MBEDTLS_ECP_DP_NONE },
};

mbedtls_ecp_group_id mbedtls_ssl_get_ecp_group_id_from_tls_id(uint16_t tls_id)
{
    for (int i = 0; tls_id_match_table[i].tls_id != 0; i++) {
        if (tls_id_match_table[i].tls_id == tls_id) {
            return tls_id_match_table[i].ecp_group_id;
        }
    }

    return MBEDTLS_ECP_DP_NONE;
}

uint16_t mbedtls_ssl_get_tls_id_from_ecp_group_id(mbedtls_ecp_group_id grp_id)
{
    for (int i = 0; tls_id_match_table[i].ecp_group_id != MBEDTLS_ECP_DP_NONE;
         i++) {
        if (tls_id_match_table[i].ecp_group_id == grp_id) {
            return tls_id_match_table[i].tls_id;
        }
    }

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_get_handshake_transcript_sha384(mbedtls_ssl_context *ssl,
                                               unsigned char *dst,
                                               size_t dst_len,
                                               size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;
    mbedtls_md_context_t sha384;

    if (dst_len < 48) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    mbedtls_md_init(&sha384);
    ret = mbedtls_md_setup(&sha384, mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), 0, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_clone(&sha384, &ssl->handshake->fin_sha384, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    if ((ret = mbedtls_md_finish(&sha384, dst, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_finish", ret);
        goto exit;
    }

    *olen = 48;

exit:

    mbedtls_md_free(&sha384);
    MINTLS_RETURN(ret);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_get_handshake_transcript_sha256(mbedtls_ssl_context *ssl,
                                               unsigned char *dst,
                                               size_t dst_len,
                                               size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;
    mbedtls_md_context_t sha256;

    if (dst_len < 32) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    mbedtls_md_init(&sha256);
    ret = mbedtls_md_setup(&sha256, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_clone(&sha256, &ssl->handshake->fin_sha256, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    if ((ret = mbedtls_md_finish(&sha256, dst, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_finish", ret);
        goto exit;
    }

    *olen = 32;

exit:

    mbedtls_md_free(&sha256);
    MINTLS_RETURN(ret);
}

int mbedtls_ssl_get_handshake_transcript(mbedtls_ssl_context *ssl,
                                         const mbedtls_md_type_t md,
                                         unsigned char *dst,
                                         size_t dst_len,
                                         size_t *olen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    switch (md) {

        case MBEDTLS_MD_SHA384:
            MINTLS_RETURN(ssl_get_handshake_transcript_sha384(ssl, dst, dst_len, olen, diagnostics));

        case MBEDTLS_MD_SHA256:
            MINTLS_RETURN(ssl_get_handshake_transcript_sha256(ssl, dst, dst_len, olen, diagnostics));

        default:
            break;
    }
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
/* mbedtls_ssl_parse_sig_alg_ext()
 *
 * The `extension_data` field of signature algorithm contains  a `SignatureSchemeList`
 * value (TLS 1.3 RFC8446):
 *      enum {
 *         ....
 *        ecdsa_secp256r1_sha256( 0x0403 ),
 *        ecdsa_secp384r1_sha384( 0x0503 ),
 *        ecdsa_secp521r1_sha512( 0x0603 ),
 *         ....
 *      } SignatureScheme;
 *
 *      struct {
 *         SignatureScheme supported_signature_algorithms<2..2^16-2>;
 *      } SignatureSchemeList;
 *
 * The `extension_data` field of signature algorithm contains a `SignatureAndHashAlgorithm`
 * value (TLS 1.2 RFC5246):
 *      enum {
 *          none(0), md5(1), sha1(2), sha224(3), sha256(4), sha384(5),
 *          sha512(6), (255)
 *      } HashAlgorithm;
 *
 *      enum { anonymous(0), rsa(1), dsa(2), ecdsa(3), (255) }
 *        SignatureAlgorithm;
 *
 *      struct {
 *          HashAlgorithm hash;
 *          SignatureAlgorithm signature;
 *      } SignatureAndHashAlgorithm;
 *
 *      SignatureAndHashAlgorithm
 *        supported_signature_algorithms<2..2^16-2>;
 *
 * The TLS 1.3 signature algorithm extension was defined to be a compatible
 * generalization of the TLS 1.2 signature algorithm extension.
 * `SignatureAndHashAlgorithm` field of TLS 1.2 can be represented by
 * `SignatureScheme` field of TLS 1.3
 *
 */
int mbedtls_ssl_parse_sig_alg_ext(mbedtls_ssl_context *ssl,
                                  const unsigned char *buf,
                                  const unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const unsigned char *p = buf;
    size_t supported_sig_algs_len = 0;
    const unsigned char *supported_sig_algs_end;
    uint16_t sig_alg;
    uint32_t common_idx = 0;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    supported_sig_algs_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    memset(ssl->handshake->received_sig_algs, 0,
           sizeof(ssl->handshake->received_sig_algs));

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, supported_sig_algs_len);
    supported_sig_algs_end = p + supported_sig_algs_len;
    while (p < supported_sig_algs_end) {
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, supported_sig_algs_end, 2);
        sig_alg = MBEDTLS_GET_UINT16_BE(p, 0);
        p += 2;
        MBEDTLS_SSL_DEBUG_MSG(4, ("received signature algorithm: 0x%x %s",
                                  sig_alg,
                                  mbedtls_ssl_sig_alg_to_str(sig_alg)));
        if (ssl->tls_version == MBEDTLS_SSL_VERSION_TLS1_2 &&
            (!(mbedtls_ssl_sig_alg_is_supported(ssl, sig_alg) &&
               mbedtls_ssl_sig_alg_is_offered(ssl, sig_alg)))) {
            continue;
        }

        MBEDTLS_SSL_DEBUG_MSG(4, ("valid signature algorithm: %s",
                                  mbedtls_ssl_sig_alg_to_str(sig_alg)));

        if (common_idx + 1 < MBEDTLS_RECEIVED_SIG_ALGS_SIZE) {
            ssl->handshake->received_sig_algs[common_idx] = sig_alg;
            common_idx += 1;
        }
    }
    /* Check that we consumed all the message. */
    if (p != end) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("Signature algorithms extension length misaligned"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,
                                     MBEDTLS_ERR_SSL_DECODE_ERROR);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    if (common_idx == 0) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("no signature algorithm in common"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE,
                                     MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
    }

    ssl->handshake->received_sig_algs[common_idx] = MBEDTLS_TLS_SIG_NONE;
    MINTLS_RETURN(0);
}

#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

MBEDTLS_CHECK_RETURN_CRITICAL
int tls_prf_generic(mbedtls_md_type_t md_type,
                           const unsigned char *secret, size_t slen,
                           const char *label, size_t label_len,
                           const unsigned char *random, size_t rlen,
                           unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t nb;
    size_t i, j, k, md_len;
    unsigned char *tmp;
    size_t tmp_len = 0;
    unsigned char h_i[MBEDTLS_MD_MAX_SIZE];
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    mbedtls_md_init(&md_ctx);

    if ((md_info = mbedtls_md_info_from_type(md_type)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    md_len = mbedtls_md_get_size(md_info);

    tmp_len = md_len + label_len + rlen;
    tmp = mbedtls_calloc(1, tmp_len);
    if (tmp == NULL) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_ALLOC_FAILED);
        goto exit;
    }

    nb = label_len;
    memcpy(tmp + md_len, label, nb);
    memcpy(tmp + md_len + nb, random, rlen);
    nb += rlen;

    /*
     * Compute P_<hash>(secret, label + random)[0..dlen]
     */
    if ((ret = mbedtls_md_setup(&md_ctx, md_info, 1, diagnostics)) != 0) {
        goto exit;
    }

    ret = mbedtls_md_hmac_starts(&md_ctx, secret, slen, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_hmac_update(&md_ctx, tmp + md_len, nb, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_hmac_finish(&md_ctx, tmp, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    for (i = 0; i < dlen; i += md_len) {
        ret = mbedtls_md_hmac_reset(&md_ctx, diagnostics);
        if (ret != 0) {
            goto exit;
        }
        ret = mbedtls_md_hmac_update(&md_ctx, tmp, md_len + nb, diagnostics);
        if (ret != 0) {
            goto exit;
        }
        ret = mbedtls_md_hmac_finish(&md_ctx, h_i, diagnostics);
        if (ret != 0) {
            goto exit;
        }

        ret = mbedtls_md_hmac_reset(&md_ctx, diagnostics);
        if (ret != 0) {
            goto exit;
        }
        ret = mbedtls_md_hmac_update(&md_ctx, tmp, md_len, diagnostics);
        if (ret != 0) {
            goto exit;
        }
        ret = mbedtls_md_hmac_finish(&md_ctx, tmp, diagnostics);
        if (ret != 0) {
            goto exit;
        }

        k = (i + md_len > dlen) ? dlen % md_len : md_len;

        for (j = 0; j < k; j++) {
            dstbuf[i + j]  = h_i[j];
        }
    }

exit:
    mbedtls_md_free(&md_ctx);

    if (tmp != NULL) {
        mbedtls_platform_zeroize(tmp, tmp_len);
    }

    mbedtls_platform_zeroize(h_i, sizeof(h_i));

    mbedtls_free(tmp);

    MINTLS_RETURN(ret);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int tls_prf_sha256(const unsigned char *secret, size_t slen,
                          const char *label,
                          const unsigned char *random, size_t rlen,
                          unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(tls_prf_generic(MBEDTLS_MD_SHA256, secret, slen,
                           label, strlen(label), random, rlen, dstbuf, dlen, diagnostics));
}

MBEDTLS_CHECK_RETURN_CRITICAL
int tls_prf_sha384(const unsigned char *secret, size_t slen,
                          const char *label,
                          const unsigned char *random, size_t rlen,
                          unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(tls_prf_generic(MBEDTLS_MD_SHA384, secret, slen,
                           label, strlen(label), random, rlen, dstbuf, dlen, diagnostics));
}

/*
 * Set appropriate PRF function and other SSL / TLS1.2 functions
 *
 * Inputs:
 * - hash associated with the ciphersuite (only used by TLS 1.2)
 *
 * Outputs:
 * - the tls_prf, calc_verify and calc_finished members of handshake structure
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_set_handshake_prfs(mbedtls_ssl_handshake_params *handshake,
                                  mbedtls_md_type_t hash)
{
    if (hash == MBEDTLS_MD_SHA384) {
        handshake->tls_prf = tls_prf_sha384;
        handshake->calc_verify = ssl_calc_verify_tls_sha384;
        handshake->calc_finished = ssl_calc_finished_tls_sha384;
    } else
    {
        (void) hash;
        handshake->tls_prf = tls_prf_sha256;
        handshake->calc_verify = ssl_calc_verify_tls_sha256;
        handshake->calc_finished = ssl_calc_finished_tls_sha256;
    }

    return 0;
}

/*
 * Compute master secret if needed
 *
 * Parameters:
 * [in/out] handshake
 *          [in] resume, premaster, extended_ms, calc_verify, tls_prf
 *               (PSA-PSK) ciphersuite_info, psk_opaque
 *          [out] premaster (cleared)
 * [out] master
 * [in] ssl: optionally used for debugging, EMS and PSA-PSK
 *      debug: conf->f_dbg, conf->p_dbg
 *      EMS: passed to calc_verify (debug + session_negotiate)
 *      PSA-PSA: conf
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_compute_master(mbedtls_ssl_handshake_params *handshake,
                              unsigned char *master,
                              const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* cf. RFC 5246, Section 8.1:
     * "The master secret is always exactly 48 bytes in length." */
    size_t const master_secret_len = 48;

    unsigned char session_hash[48];

    /* The label for the KDF used for key expansion.
     * This is either "master secret" or "extended master secret"
     * depending on whether the Extended Master Secret extension
     * is used. */
    char const *lbl = "master secret";

    /* The seed for the KDF used for key expansion.
     * - If the Extended Master Secret extension is not used,
     *   this is ClientHello.Random + ServerHello.Random
     *   (see Sect. 8.1 in RFC 5246).
     * - If the Extended Master Secret extension is used,
     *   this is the transcript of the handshake so far.
     *   (see Sect. 4 in RFC 7627). */
    unsigned char const *seed = handshake->randbytes;
    size_t seed_len = 64;

    if (handshake->resume != 0) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("no premaster (session resumed)"));
        MINTLS_RETURN(0);
    }

    if (handshake->extended_ms == MBEDTLS_SSL_EXTENDED_MS_ENABLED) {
        lbl  = "extended master secret";
        ret = handshake->calc_verify(ssl, session_hash, &seed_len, diagnostics);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "calc_verify", ret);
            MINTLS_RETURN(ret);
        }
        if (seed_len > sizeof(session_hash)) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad session hash length"));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        }
        seed = session_hash;

        MBEDTLS_SSL_DEBUG_BUF(3, "session hash for extended master secret",
                              session_hash, seed_len);
    }

    {
        ret = handshake->tls_prf(handshake->premaster, handshake->pmslen,
                                 lbl, seed, seed_len,
                                 master,
                                 master_secret_len, diagnostics);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "prf", ret);
            MINTLS_RETURN(ret);
        }

        MBEDTLS_SSL_DEBUG_BUF(3, "premaster secret",
                              handshake->premaster,
                              handshake->pmslen);

        mbedtls_platform_zeroize(handshake->premaster,
                                 sizeof(handshake->premaster));
    }

    MINTLS_RETURN(0);
}

int mbedtls_ssl_derive_keys(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_ssl_ciphersuite_t * const ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> derive keys"));

    /* Set PRF, calc_verify and calc_finished function pointers */
    ret = ssl_set_handshake_prfs(ssl->handshake,
                                 (mbedtls_md_type_t) ciphersuite_info->mac);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "ssl_set_handshake_prfs", ret);
        MINTLS_RETURN(ret);
    }

    /* Compute master secret if needed */
    ret = ssl_compute_master(ssl->handshake,
                             ssl->session_negotiate->master,
                             ssl, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "ssl_compute_master", ret);
        MINTLS_RETURN(ret);
    }

    /* Swap the client and server random values:
     * - MS derivation wanted client+server (RFC 5246 8.1)
     * - key derivation wants server+client (RFC 5246 6.3) */
    {
        unsigned char tmp[64];
        memcpy(tmp, ssl->handshake->randbytes, 64);
        memcpy(ssl->handshake->randbytes, tmp + 32, 32);
        memcpy(ssl->handshake->randbytes + 32, tmp, 32);
        mbedtls_platform_zeroize(tmp, sizeof(tmp));
    }

    /* Populate transform structure */
    ret = ssl_tls12_populate_transform(ssl->transform_negotiate,
                                       ssl->session_negotiate->ciphersuite,
                                       ssl->session_negotiate->master,
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
                                       ssl->session_negotiate->encrypt_then_mac,
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
                                       ssl->handshake->tls_prf,
                                       ssl->handshake->randbytes,
                                       ssl->tls_version,
                                       ssl->conf->endpoint,
                                       ssl, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "ssl_tls12_populate_transform", ret);
        MINTLS_RETURN(ret);
    }

    /* We no longer need Server/ClientHello.random values */
    mbedtls_platform_zeroize(ssl->handshake->randbytes,
                             sizeof(ssl->handshake->randbytes));

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= derive keys"));

    MINTLS_RETURN(0);
}

int mbedtls_ssl_set_calc_verify_md(mbedtls_ssl_context *ssl, int md, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    switch (md) {
        case MBEDTLS_SSL_HASH_SHA384:
            ssl->handshake->calc_verify = ssl_calc_verify_tls_sha384;
            break;
        case MBEDTLS_SSL_HASH_SHA256:
            ssl->handshake->calc_verify = ssl_calc_verify_tls_sha256;
            break;
        default:
            MINTLS_RETURN_ERROR(-1);
    }
    MINTLS_RETURN(0);
}

int ssl_calc_verify_tls_legacy(const mbedtls_ssl_context *ssl,
                                      const mbedtls_md_context_t *hs_ctx,
                                      unsigned char *hash,
                                      size_t *hlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_md_context_t cloned_ctx;

    mbedtls_md_init(&cloned_ctx);

    (void) ssl;
    MBEDTLS_SSL_DEBUG_MSG(2, ("=> calc verify"));

    ret = mbedtls_md_setup(&cloned_ctx, mbedtls_md_info_from_ctx(hs_ctx), 0, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_clone(&cloned_ctx, hs_ctx, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    ret = mbedtls_md_finish(&cloned_ctx, hash, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    *hlen = mbedtls_md_get_size(mbedtls_md_info_from_ctx(hs_ctx));

    MBEDTLS_SSL_DEBUG_BUF(3, "calculated verify result", hash, *hlen);
    MBEDTLS_SSL_DEBUG_MSG(2, ("<= calc verify"));

exit:
    mbedtls_md_free(&cloned_ctx);
    MINTLS_RETURN(ret);
}

int ssl_calc_verify_tls_sha256(const mbedtls_ssl_context *ssl,
                               unsigned char *hash,
                               size_t *hlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ssl_calc_verify_tls_legacy(ssl, &ssl->handshake->fin_sha256,
                                      hash, hlen, diagnostics));
}

int ssl_calc_verify_tls_sha384(const mbedtls_ssl_context *ssl,
                               unsigned char *hash,
                               size_t *hlen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ssl_calc_verify_tls_legacy(ssl, &ssl->handshake->fin_sha384,
                                      hash, hlen, diagnostics));
}

/*
 * Handshake functions
 */
#if !defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
/* No certificate support -> dummy functions */
int mbedtls_ssl_write_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write certificate"));

    if (!mbedtls_ssl_ciphersuite_uses_srv_cert(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}

int mbedtls_ssl_parse_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse certificate"));

    if (!mbedtls_ssl_ciphersuite_uses_srv_cert(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip parse certificate"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
}

#else
/* Some certificate support -> implement write and parse */

int mbedtls_ssl_write_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
    size_t i, n;
    const mbedtls_x509_crt *crt;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write certificate"));

    if (!mbedtls_ssl_ciphersuite_uses_srv_cert(ciphersuite_info)) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate"));
        mbedtls_ssl_handshake_increment_state(ssl);
        MINTLS_RETURN(0);
    }

    if (ssl->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
        if (ssl->handshake->client_auth == 0) {
            MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip write certificate"));
            mbedtls_ssl_handshake_increment_state(ssl);
            MINTLS_RETURN(0);
        }
    }

    MBEDTLS_SSL_DEBUG_CRT(3, "own certificate", mbedtls_ssl_own_cert(ssl));

    /*
     *     0  .  0    handshake type
     *     1  .  3    handshake length
     *     4  .  6    length of all certs
     *     7  .  9    length of cert. 1
     *    10  . n-1   peer certificate
     *     n  . n+2   length of cert. 2
     *    n+3 . ...   upper level cert, etc.
     */
    i = 7;
    crt = mbedtls_ssl_own_cert(ssl);

    while (crt != NULL) {
        n = crt->raw.len;
        if (n > MBEDTLS_SSL_OUT_CONTENT_LEN - 3 - i) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("certificate too large, %" MBEDTLS_PRINTF_SIZET
                                      " > %" MBEDTLS_PRINTF_SIZET,
                                      i + 3 + n, (size_t) MBEDTLS_SSL_OUT_CONTENT_LEN));
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL);
        }

        ssl->out_msg[i] = MBEDTLS_BYTE_2(n);
        ssl->out_msg[i + 1] = MBEDTLS_BYTE_1(n);
        ssl->out_msg[i + 2] = MBEDTLS_BYTE_0(n);

        i += 3; memcpy(ssl->out_msg + i, crt->raw.p, n);
        i += n; crt = crt->next;
    }

    ssl->out_msg[4]  = MBEDTLS_BYTE_2(i - 7);
    ssl->out_msg[5]  = MBEDTLS_BYTE_1(i - 7);
    ssl->out_msg[6]  = MBEDTLS_BYTE_0(i - 7);

    ssl->out_msglen  = i;
    ssl->out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = MBEDTLS_SSL_HS_CERTIFICATE;

    mbedtls_ssl_handshake_increment_state(ssl);

    if ((ret = mbedtls_ssl_write_handshake_msg(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_handshake_msg", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write certificate"));

    MINTLS_RETURN(ret);
}

/*
 * Once the certificate message is read, parse it into a cert chain and
 * perform basic checks, but leave actual verification to the caller
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_certificate_chain(mbedtls_ssl_context *ssl,
                                       mbedtls_x509_crt *chain, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i, n;
    uint8_t alert;

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    if (ssl->in_msg[0] != MBEDTLS_SSL_HS_CERTIFICATE) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
    }

    if (ssl->in_hslen < mbedtls_ssl_hs_hdr_len(ssl) + 3 + 3) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    i = mbedtls_ssl_hs_hdr_len(ssl);

    /*
     * Same message structure as in mbedtls_ssl_write_certificate()
     */
    n = MBEDTLS_GET_UINT16_BE(ssl->in_msg, i + 1);

    if (ssl->in_msg[i] != 0 ||
        ssl->in_hslen != n + 3 + mbedtls_ssl_hs_hdr_len(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
    }

    /* Make &ssl->in_msg[i] point to the beginning of the CRT chain. */
    i += 3;

    /* Iterate through and parse the CRTs in the provided chain. */
    while (i < ssl->in_hslen) {
        /* Check that there's room for the next CRT's length fields. */
        if (i + 3 > ssl->in_hslen) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
            mbedtls_ssl_send_alert_message(ssl,
                                           MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                           MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }
        /* In theory, the CRT can be up to 2**24 Bytes, but we don't support
         * anything beyond 2**16 ~ 64K. */
        if (ssl->in_msg[i] != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
            mbedtls_ssl_send_alert_message(ssl,
                                           MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                           MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CERTIFICATE);
        }

        /* Read length of the next CRT in the chain. */
        n = MBEDTLS_GET_UINT16_BE(ssl->in_msg, i + 1);
        i += 3;

        if (n < 128 || i + n > ssl->in_hslen) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate message"));
            mbedtls_ssl_send_alert_message(ssl,
                                           MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                           MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_DECODE_ERROR);
        }

        /* Check if we're handling the first CRT in the chain. */

        /* Parse the next certificate in the chain. */
        ret = mbedtls_x509_crt_parse_der(chain, ssl->in_msg + i, n, diagnostics);
        switch (ret) {
            case 0: /*ok*/
            case MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG + MBEDTLS_ERR_OID_NOT_FOUND:
                /* Ignore certificate with an unknown algorithm: maybe a
                   prior certificate was already trusted. */
                break;

            case MBEDTLS_ERR_X509_ALLOC_FAILED:
                alert = MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR;
                goto crt_parse_der_failed;

            case MBEDTLS_ERR_X509_UNKNOWN_VERSION:
                alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT;
                goto crt_parse_der_failed;

            default:
                alert = MBEDTLS_SSL_ALERT_MSG_BAD_CERT;
crt_parse_der_failed:
                mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL, alert, diagnostics);
                MBEDTLS_SSL_DEBUG_RET(1, " mbedtls_x509_crt_parse_der", ret);
                MINTLS_RETURN(ret);
        }

        i += n;
    }

    MBEDTLS_SSL_DEBUG_CRT(3, "peer certificate", chain);
    MINTLS_RETURN(0);
}

/* Check if a certificate message is expected.
 * Return either
 * - SSL_CERTIFICATE_EXPECTED, or
 * - SSL_CERTIFICATE_SKIP
 * indicating whether a Certificate message is expected or not.
 */
#define SSL_CERTIFICATE_EXPECTED 0
#define SSL_CERTIFICATE_SKIP     1
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_parse_certificate_coordinate(mbedtls_ssl_context *ssl,
                                            int authmode)
{
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info =
        ssl->handshake->ciphersuite_info;

    if (!mbedtls_ssl_ciphersuite_uses_srv_cert(ciphersuite_info)) {
        ssl->session_negotiate->verify_result = 0;
        return SSL_CERTIFICATE_SKIP;
    }

    ((void) authmode);

    return SSL_CERTIFICATE_EXPECTED;
}

int mbedtls_ssl_parse_certificate(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    int crt_expected;
    /* Authmode: precedence order is SNI if used else configuration */
    const int authmode = ssl->conf->authmode;
    void *rs_ctx = NULL;
    mbedtls_x509_crt *chain = NULL;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse certificate"));

    crt_expected = ssl_parse_certificate_coordinate(ssl, authmode);
    if (crt_expected == SSL_CERTIFICATE_SKIP) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("<= skip parse certificate"));
        goto exit;
    }

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        /* mbedtls_ssl_read_record may have sent an alert already. We
           let it decide whether to alert. */
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        goto exit;
    }

    /* Clear existing peer CRT structure in case we tried to
     * reuse a session but it failed, and allocate a new one. */
    ssl_clear_peer_cert(ssl->session_negotiate);

    chain = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (chain == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("alloc(%" MBEDTLS_PRINTF_SIZET " bytes) failed",
                                  sizeof(mbedtls_x509_crt)));
        mbedtls_ssl_send_alert_message(ssl,
                                       MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR, diagnostics);

        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_ALLOC_FAILED);
        goto exit;
    }
    mbedtls_x509_crt_init(chain);

    ret = ssl_parse_certificate_chain(ssl, chain, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    ret = mbedtls_ssl_verify_certificate(ssl, authmode, chain,
                                         ssl->handshake->ciphersuite_info,
                                         rs_ctx, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    /* Pass ownership to session structure. */
    ssl->session_negotiate->peer_cert = chain;
    chain = NULL;

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse certificate"));

exit:

    if (ret == 0) {
        mbedtls_ssl_handshake_increment_state(ssl);
    }

    if (chain != NULL) {
        mbedtls_x509_crt_free(chain);
        mbedtls_free(chain);
    }

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

int ssl_calc_finished_tls_generic(mbedtls_ssl_context *ssl, void *ctx,
                                         unsigned char *padbuf, size_t hlen,
                                         unsigned char *buf, int from, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned int len = 12;
    const char *sender;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_md_context_t *hs_ctx = ctx;
    mbedtls_md_context_t cloned_ctx;
    mbedtls_md_init(&cloned_ctx);

    mbedtls_ssl_session *session = ssl->session_negotiate;
    if (!session) {
        session = ssl->session;
    }

    sender = (from == MBEDTLS_SSL_IS_CLIENT)
             ? "client finished"
             : "server finished";

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> calc finished tls"));

    ret = mbedtls_md_setup(&cloned_ctx, mbedtls_md_info_from_ctx(hs_ctx), 0, diagnostics);
    if (ret != 0) {
        goto exit;
    }
    ret = mbedtls_md_clone(&cloned_ctx, hs_ctx, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    ret = mbedtls_md_finish(&cloned_ctx, padbuf, diagnostics);
    if (ret != 0) {
        goto exit;
    }

    MBEDTLS_SSL_DEBUG_BUF(4, "finished output", padbuf, hlen);

    /*
     * TLSv1.2:
     *   hash = PRF( master, finished_label,
     *               Hash( handshake ) )[0.11]
     */
    ret = ssl->handshake->tls_prf(session->master, 48, sender,
                            padbuf, hlen, buf, len, diagnostics);

    MBEDTLS_SSL_DEBUG_BUF(3, "calc finished result", buf, len);

    mbedtls_platform_zeroize(padbuf, hlen);

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= calc finished"));

exit:
    mbedtls_md_free(&cloned_ctx);
    MINTLS_RETURN(ret);
}

int ssl_calc_finished_tls_sha256(
    mbedtls_ssl_context *ssl, unsigned char *buf, int from, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char padbuf[32];
    MINTLS_RETURN(ssl_calc_finished_tls_generic(ssl,
                                         &ssl->handshake->fin_sha256,
                                         padbuf, sizeof(padbuf),
                                         buf, from, diagnostics));
}

int ssl_calc_finished_tls_sha384(
    mbedtls_ssl_context *ssl, unsigned char *buf, int from, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char padbuf[48];
    MINTLS_RETURN(ssl_calc_finished_tls_generic(ssl,
                                         &ssl->handshake->fin_sha384,
                                         padbuf, sizeof(padbuf),
                                         buf, from, diagnostics));
}

void mbedtls_ssl_handshake_wrapup_free_hs_transform(mbedtls_ssl_context *ssl)
{
    MBEDTLS_SSL_DEBUG_MSG(3, ("=> handshake wrapup: final free"));

    /*
     * Free our handshake params
     */
    mbedtls_ssl_handshake_free(ssl);
    mbedtls_free(ssl->handshake);
    ssl->handshake = NULL;

    /*
     * Free the previous transform and switch in the current one
     */
    if (ssl->transform) {
        mbedtls_ssl_transform_free(ssl->transform);
        mbedtls_free(ssl->transform);
    }
    ssl->transform = ssl->transform_negotiate;
    ssl->transform_negotiate = NULL;

    MBEDTLS_SSL_DEBUG_MSG(3, ("<= handshake wrapup: final free"));
}

void mbedtls_ssl_handshake_wrapup(mbedtls_ssl_context *ssl)
{
    int resume = ssl->handshake->resume;

    MBEDTLS_SSL_DEBUG_MSG(3, ("=> handshake wrapup"));

    /*
     * Free the previous session and switch in the current one
     */
    if (ssl->session) {

        mbedtls_ssl_session_free(ssl->session);
        mbedtls_free(ssl->session);
    }
    ssl->session = ssl->session_negotiate;
    ssl->session_negotiate = NULL;

    /*
     * Add cache entry
     */
    if (ssl->conf->f_set_cache != NULL &&
        ssl->session->id_len != 0 &&
        resume == 0) {
        if (ssl->conf->f_set_cache(ssl->conf->p_cache,
                                   ssl->session->id,
                                   ssl->session->id_len,
                                   ssl->session) != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("cache did not store session"));
        }
    }

    mbedtls_ssl_handshake_wrapup_free_hs_transform(ssl);

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_OVER);

    MBEDTLS_SSL_DEBUG_MSG(3, ("<= handshake wrapup"));
}

int mbedtls_ssl_write_finished(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;
    unsigned int hash_len;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write finished"));

    mbedtls_ssl_update_out_pointers(ssl, ssl->transform_negotiate);

    ret = ssl->handshake->calc_finished(ssl, ssl->out_msg + 4, ssl->conf->endpoint, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "calc_finished", ret);
        MINTLS_RETURN(ret);
    }

    /*
     * RFC 5246 7.4.9 (Page 63) says 12 is the default length and ciphersuites
     * may define some other value. Currently (early 2016), no defined
     * ciphersuite does this (and this is unlikely to change as activity has
     * moved to TLS 1.3 now) so we can keep the hardcoded 12 here.
     */
    hash_len = 12;

    ssl->out_msglen  = 4 + hash_len;
    ssl->out_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = MBEDTLS_SSL_HS_FINISHED;

    /*
     * In case of session resuming, invert the client and server
     * ChangeCipherSpec messages order.
     */
    if (ssl->handshake->resume != 0) {
        if (ssl->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_WRAPUP);
        }
    } else {
        mbedtls_ssl_handshake_increment_state(ssl);
    }

    /*
     * Switch to our negotiated transform and session parameters for outbound
     * data.
     */
    MBEDTLS_SSL_DEBUG_MSG(3, ("switching to new transform spec for outbound data"));

    memset(ssl->cur_out_ctr, 0, sizeof(ssl->cur_out_ctr));

    ssl->transform_out = ssl->transform_negotiate;
    ssl->session_out = ssl->session_negotiate;

    if ((ret = mbedtls_ssl_write_handshake_msg(ssl, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_write_handshake_msg", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write finished"));

    MINTLS_RETURN(0);
}

#define SSL_MAX_HASH_LEN 12

int mbedtls_ssl_parse_finished(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned int hash_len = 12;
    unsigned char buf[SSL_MAX_HASH_LEN];

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse finished"));

    ret = ssl->handshake->calc_finished(ssl, buf, ssl->conf->endpoint ^ 1, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "calc_finished", ret);
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_ssl_read_record(ssl, 1, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        goto exit;
    }

    if (ssl->in_msgtype != MBEDTLS_SSL_MSG_HANDSHAKE) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad finished message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
        goto exit;
    }

    if (ssl->in_msg[0] != MBEDTLS_SSL_HS_FINISHED) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE, diagnostics);
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
        goto exit;
    }

    if (ssl->in_hslen  != mbedtls_ssl_hs_hdr_len(ssl) + hash_len) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad finished message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR, diagnostics);
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_DECODE_ERROR);
        goto exit;
    }

    if (mbedtls_ct_memcmp(ssl->in_msg + mbedtls_ssl_hs_hdr_len(ssl),
                          buf, hash_len) != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad finished message"));
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_DECRYPT_ERROR, diagnostics);
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
        goto exit;
    }

    if (ssl->handshake->resume != 0) {
        if (ssl->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC);
        }
    } else {
        mbedtls_ssl_handshake_increment_state(ssl);
    }

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse finished"));

exit:
    mbedtls_platform_zeroize(buf, hash_len);
    MINTLS_RETURN(ret);
}

mbedtls_tls_prf_types tls_prf_get_type(mbedtls_ssl_tls_prf_cb *tls_prf)
{
    ((void) tls_prf);
    if (tls_prf == tls_prf_sha384) {
        return MBEDTLS_SSL_TLS_PRF_SHA384;
    } else
    if (tls_prf == tls_prf_sha256) {
        return MBEDTLS_SSL_TLS_PRF_SHA256;
    } else
    return MBEDTLS_SSL_TLS_PRF_NONE;
}

/*
 * Populate a transform structure with session keys and all the other
 * necessary information.
 *
 * Parameters:
 * - [in/out]: transform: structure to populate
 *      [in] must be just initialised with mbedtls_ssl_transform_init()
 *      [out] fully populated, ready for use by mbedtls_ssl_{en,de}crypt_buf()
 * - [in] ciphersuite
 * - [in] master
 * - [in] encrypt_then_mac
 * - [in] tls_prf: pointer to PRF to use for key derivation
 * - [in] randbytes: buffer holding ServerHello.random + ClientHello.random
 * - [in] tls_version: TLS version
 * - [in] endpoint: client or server
 * - [in] ssl: used for:
 *        - ssl->conf->{f,p}_export_keys
 *      [in] optionally used for:
 *        - MBEDTLS_DEBUG_C: ssl->conf->{f,p}_dbg
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int ssl_tls12_populate_transform(mbedtls_ssl_transform *transform,
                                        int ciphersuite,
                                        const unsigned char master[48],
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
                                        int encrypt_then_mac,
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
                                        ssl_tls_prf_t tls_prf,
                                        const unsigned char randbytes[64],
                                        mbedtls_ssl_protocol_version tls_version,
                                        unsigned endpoint,
                                        const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    unsigned char keyblk[256];
    unsigned char *key1;
    unsigned char *key2;
    unsigned char *mac_enc;
    unsigned char *mac_dec;
    size_t mac_key_len = 0;
    size_t iv_copy_len;
    size_t keylen;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
    mbedtls_ssl_mode_t ssl_mode;
    const mbedtls_cipher_info_t *cipher_info;
    const mbedtls_md_info_t *md_info;

    /*
     * Some data just needs copying into the structure
     */
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
    transform->encrypt_then_mac = encrypt_then_mac;
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
    transform->tls_version = tls_version;

#if defined(MBEDTLS_SSL_KEEP_RANDBYTES)
    memcpy(transform->randbytes, randbytes, sizeof(transform->randbytes));
#endif

    /*
     * Get various info structures
     */
    ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(ciphersuite);
    if (ciphersuite_info == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("ciphersuite info for %d not found",
                                  ciphersuite));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    ssl_mode = mbedtls_ssl_get_mode_from_ciphersuite(
#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
        encrypt_then_mac,
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */
        ciphersuite_info);

    if (ssl_mode == MBEDTLS_SSL_MODE_AEAD) {
        transform->taglen =
            ciphersuite_info->flags & MBEDTLS_CIPHERSUITE_SHORT_TAG ? 8 : 16;
    }

    cipher_info = mbedtls_cipher_info_from_type((mbedtls_cipher_type_t) ciphersuite_info->cipher);
    if (cipher_info == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("cipher info for %u not found",
                                  ciphersuite_info->cipher));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    md_info = mbedtls_md_info_from_type((mbedtls_md_type_t) ciphersuite_info->mac);
    if (md_info == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("mbedtls_md info for %u not found",
                                  (unsigned) ciphersuite_info->mac));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    /*
     * Compute key block using the PRF
     */
    ret = tls_prf(master, 48, "key expansion", randbytes, 64, keyblk, 256, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "prf", ret);
        MINTLS_RETURN(ret);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("ciphersuite = %s",
                              mbedtls_ssl_get_ciphersuite_name(ciphersuite)));
    MBEDTLS_SSL_DEBUG_BUF(3, "master secret", master, 48);
    MBEDTLS_SSL_DEBUG_BUF(4, "random bytes", randbytes, 64);
    MBEDTLS_SSL_DEBUG_BUF(4, "key block", keyblk, 256);

    /*
     * Determine the appropriate key, IV and MAC length.
     */

    keylen = mbedtls_cipher_info_get_key_bitlen(cipher_info) / 8;

    if (ssl_mode == MBEDTLS_SSL_MODE_AEAD) {
        size_t explicit_ivlen;

        transform->maclen = 0;
        mac_key_len = 0;

        /* All modes haves 96-bit IVs, but the length of the static parts vary
         * with mode and version:
         * - For GCM and CCM in TLS 1.2, there's a static IV of 4 Bytes
         *   (to be concatenated with a dynamically chosen IV of 8 Bytes)
         * - For ChaChaPoly in TLS 1.2, and all modes in TLS 1.3, there's
         *   a static IV of 12 Bytes (to be XOR'ed with the 8 Byte record
         *   sequence number).
         */
        transform->ivlen = 12;

        int is_chachapoly = 0;
        is_chachapoly = (mbedtls_cipher_info_get_mode(cipher_info)
                         == MBEDTLS_MODE_CHACHAPOLY);

        if (is_chachapoly) {
            transform->fixed_ivlen = 12;
        } else {
            transform->fixed_ivlen = 4;
        }

        /* Minimum length of encrypted record */
        explicit_ivlen = transform->ivlen - transform->fixed_ivlen;
        transform->minlen = explicit_ivlen + transform->taglen;
    } else
    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("keylen: %u, minlen: %u, ivlen: %u, maclen: %u",
                              (unsigned) keylen,
                              (unsigned) transform->minlen,
                              (unsigned) transform->ivlen,
                              (unsigned) transform->maclen));

    /*
     * Finally setup the cipher contexts, IVs and MAC secrets.
     */
    if (endpoint == MBEDTLS_SSL_IS_CLIENT) {
        key1 = keyblk + mac_key_len * 2;
        key2 = keyblk + mac_key_len * 2 + keylen;

        mac_enc = keyblk;
        mac_dec = keyblk + mac_key_len;

        iv_copy_len = (transform->fixed_ivlen) ?
                      transform->fixed_ivlen : transform->ivlen;
        memcpy(transform->iv_enc, key2 + keylen,  iv_copy_len);
        memcpy(transform->iv_dec, key2 + keylen + iv_copy_len,
               iv_copy_len);
    } else
    {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_INTERNAL_ERROR);
        goto end;
    }

    if (ssl->f_export_keys != NULL) {
        ssl->f_export_keys(ssl->p_export_keys,
                           MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET,
                           master, 48,
                           randbytes + 32,
                           randbytes,
                           tls_prf_get_type(tls_prf));
    }

    if ((ret = mbedtls_cipher_setup(&transform->cipher_ctx_enc,
                                    cipher_info, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_setup", ret);
        goto end;
    }

    if ((ret = mbedtls_cipher_setup(&transform->cipher_ctx_dec,
                                    cipher_info, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_setup", ret);
        goto end;
    }

    if ((ret = mbedtls_cipher_setkey(&transform->cipher_ctx_enc, key1,
                                     (int) mbedtls_cipher_info_get_key_bitlen(cipher_info),
                                     MBEDTLS_ENCRYPT, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_setkey", ret);
        goto end;
    }

    if ((ret = mbedtls_cipher_setkey(&transform->cipher_ctx_dec, key2,
                                     (int) mbedtls_cipher_info_get_key_bitlen(cipher_info),
                                     MBEDTLS_DECRYPT, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_cipher_setkey", ret);
        goto end;
    }

    ((void) mac_dec);
    ((void) mac_enc);

end:
    mbedtls_platform_zeroize(keyblk, sizeof(keyblk));
    MINTLS_RETURN(ret);
}

int mbedtls_ssl_get_key_exchange_md_tls1_2(mbedtls_ssl_context *ssl,
                                           unsigned char *hash, size_t *hashlen,
                                           unsigned char *data, size_t data_len,
                                           mbedtls_md_type_t md_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_alg);
    *hashlen = mbedtls_md_get_size(md_info);

    MBEDTLS_SSL_DEBUG_MSG(3, ("Perform mbedtls-based computation of digest of ServerKeyExchange"));

    mbedtls_md_init(&ctx);

    /*
     * digitally-signed struct {
     *     opaque client_random[32];
     *     opaque server_random[32];
     *     ServerDHParams params;
     * };
     */
    if ((ret = mbedtls_md_setup(&ctx, md_info, 0, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_setup", ret);
        goto exit;
    }
    if ((ret = mbedtls_md_starts(&ctx, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_starts", ret);
        goto exit;
    }
    if ((ret = mbedtls_md_update(&ctx, ssl->handshake->randbytes, 64, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_update", ret);
        goto exit;
    }
    if ((ret = mbedtls_md_update(&ctx, data, data_len, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_update", ret);
        goto exit;
    }
    if ((ret = mbedtls_md_finish(&ctx, hash, diagnostics)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_md_finish", ret);
        goto exit;
    }

exit:
    mbedtls_md_free(&ctx);

    if (ret != 0) {
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR, diagnostics);
    }

    MINTLS_RETURN(ret);
}

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)

/* Find the preferred hash for a given signature algorithm. */
unsigned int mbedtls_ssl_tls12_get_preferred_hash_for_sig_alg(
    mbedtls_ssl_context *ssl,
    unsigned int sig_alg)
{
    unsigned int i;
    uint16_t *received_sig_algs = ssl->handshake->received_sig_algs;

    if (sig_alg == MBEDTLS_SSL_SIG_ANON) {
        return MBEDTLS_SSL_HASH_NONE;
    }

    for (i = 0; received_sig_algs[i] != MBEDTLS_TLS_SIG_NONE; i++) {
        unsigned int hash_alg_received =
            MBEDTLS_SSL_TLS12_HASH_ALG_FROM_SIG_AND_HASH_ALG(
                received_sig_algs[i]);
        unsigned int sig_alg_received =
            MBEDTLS_SSL_TLS12_SIG_ALG_FROM_SIG_AND_HASH_ALG(
                received_sig_algs[i]);

        mbedtls_md_type_t md_alg =
            mbedtls_ssl_md_alg_from_hash((unsigned char) hash_alg_received);
        if (md_alg == MBEDTLS_MD_NONE) {
            continue;
        }

        if (sig_alg == sig_alg_received) {

            return hash_alg_received;
        }
    }

    return MBEDTLS_SSL_HASH_NONE;
}

#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

int mbedtls_ssl_validate_ciphersuite(
    const mbedtls_ssl_context *ssl,
    const mbedtls_ssl_ciphersuite_t *suite_info,
    mbedtls_ssl_protocol_version min_tls_version,
    mbedtls_ssl_protocol_version max_tls_version)
{
    (void) ssl;

    if (suite_info == NULL) {
        return -1;
    }

    if ((suite_info->min_tls_version > max_tls_version) ||
        (suite_info->max_tls_version < min_tls_version)) {
        return -1;
    }

    /* Don't suggest PSK-based ciphersuite if no PSK is available. */

    return 0;
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
/*
 * Function for writing a signature algorithm extension.
 *
 * The `extension_data` field of signature algorithm contains  a `SignatureSchemeList`
 * value (TLS 1.3 RFC8446):
 *      enum {
 *         ....
 *        ecdsa_secp256r1_sha256( 0x0403 ),
 *        ecdsa_secp384r1_sha384( 0x0503 ),
 *        ecdsa_secp521r1_sha512( 0x0603 ),
 *         ....
 *      } SignatureScheme;
 *
 *      struct {
 *         SignatureScheme supported_signature_algorithms<2..2^16-2>;
 *      } SignatureSchemeList;
 *
 * The `extension_data` field of signature algorithm contains a `SignatureAndHashAlgorithm`
 * value (TLS 1.2 RFC5246):
 *      enum {
 *          none(0), md5(1), sha1(2), sha224(3), sha256(4), sha384(5),
 *          sha512(6), (255)
 *      } HashAlgorithm;
 *
 *      enum { anonymous(0), rsa(1), dsa(2), ecdsa(3), (255) }
 *        SignatureAlgorithm;
 *
 *      struct {
 *          HashAlgorithm hash;
 *          SignatureAlgorithm signature;
 *      } SignatureAndHashAlgorithm;
 *
 *      SignatureAndHashAlgorithm
 *        supported_signature_algorithms<2..2^16-2>;
 *
 * The TLS 1.3 signature algorithm extension was defined to be a compatible
 * generalization of the TLS 1.2 signature algorithm extension.
 * `SignatureAndHashAlgorithm` field of TLS 1.2 can be represented by
 * `SignatureScheme` field of TLS 1.3
 *
 */
int mbedtls_ssl_write_sig_alg_ext(mbedtls_ssl_context *ssl, unsigned char *buf,
                                  const unsigned char *end, size_t *out_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = buf;
    unsigned char *supported_sig_alg; /* Start of supported_signature_algorithms */
    size_t supported_sig_alg_len = 0; /* Length of supported_signature_algorithms */

    *out_len = 0;

    MBEDTLS_SSL_DEBUG_MSG(3, ("adding signature_algorithms extension"));

    /* Check if we have space for header and length field:
     * - extension_type         (2 bytes)
     * - extension_data_length  (2 bytes)
     * - supported_signature_algorithms_length   (2 bytes)
     */
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 6);
    p += 6;

    /*
     * Write supported_signature_algorithms
     */
    supported_sig_alg = p;
    const uint16_t *sig_alg = mbedtls_ssl_get_sig_algs(ssl);
    if (sig_alg == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_CONFIG);
    }

    for (; *sig_alg != MBEDTLS_TLS1_3_SIG_NONE; sig_alg++) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("got signature scheme [%x] %s",
                                  *sig_alg,
                                  mbedtls_ssl_sig_alg_to_str(*sig_alg)));
        if (!mbedtls_ssl_sig_alg_is_supported(ssl, *sig_alg)) {
            continue;
        }
        MBEDTLS_SSL_CHK_BUF_PTR(p, end, 2);
        MBEDTLS_PUT_UINT16_BE(*sig_alg, p, 0);
        p += 2;
        MBEDTLS_SSL_DEBUG_MSG(3, ("sent signature scheme [%x] %s",
                                  *sig_alg,
                                  mbedtls_ssl_sig_alg_to_str(*sig_alg)));
    }

    /* Length of supported_signature_algorithms */
    supported_sig_alg_len = (size_t) (p - supported_sig_alg);
    if (supported_sig_alg_len == 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("No signature algorithms defined."));
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    }

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_SIG_ALG, buf, 0);
    MBEDTLS_PUT_UINT16_BE(supported_sig_alg_len + 2, buf, 2);
    MBEDTLS_PUT_UINT16_BE(supported_sig_alg_len, buf, 4);

    *out_len = (size_t) (p - buf);

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/*
 * mbedtls_ssl_parse_server_name_ext
 *
 * Structure of server_name extension:
 *
 *  enum {
 *        host_name(0), (255)
 *     } NameType;
 *  opaque HostName<1..2^16-1>;
 *
 *  struct {
 *          NameType name_type;
 *          select (name_type) {
 *             case host_name: HostName;
 *           } name;
 *     } ServerName;
 *  struct {
 *          ServerName server_name_list<1..2^16-1>
 *     } ServerNameList;
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_server_name_ext(mbedtls_ssl_context *ssl,
                                      const unsigned char *buf,
                                      const unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const unsigned char *p = buf;
    size_t server_name_list_len, hostname_len;
    const unsigned char *server_name_list_end;

    MBEDTLS_SSL_DEBUG_MSG(3, ("parse ServerName extension"));

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    server_name_list_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, server_name_list_len);
    server_name_list_end = p + server_name_list_len;
    while (p < server_name_list_end) {
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, server_name_list_end, 3);
        hostname_len = MBEDTLS_GET_UINT16_BE(p, 1);
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, server_name_list_end,
                                     hostname_len + 3);

        if (p[0] == MBEDTLS_TLS_EXT_SERVERNAME_HOSTNAME) {
            /* sni_name is intended to be used only during the parsing of the
             * ClientHello message (it is reset to NULL before the end of
             * the message parsing). Thus it is ok to just point to the
             * reception buffer and not make a copy of it.
             */
            ssl->handshake->sni_name = p + 3;
            ssl->handshake->sni_name_len = hostname_len;
            if (ssl->conf->f_sni == NULL) {
                MINTLS_RETURN(0);
            }
            ret = ssl->conf->f_sni(ssl->conf->p_sni,
                                   ssl, p + 3, hostname_len, diagnostics);
            if (ret != 0) {
                MBEDTLS_SSL_DEBUG_RET(1, "ssl_sni_wrapper", ret);
                MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_UNRECOGNIZED_NAME,
                                             MBEDTLS_ERR_SSL_UNRECOGNIZED_NAME);
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_UNRECOGNIZED_NAME);
            }
            MINTLS_RETURN(0);
        }

        p += hostname_len + 3;
    }

    MINTLS_RETURN(0);
}

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_alpn_ext(mbedtls_ssl_context *ssl,
                               const unsigned char *buf,
                               const unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const unsigned char *p = buf;
    size_t protocol_name_list_len;
    const unsigned char *protocol_name_list;
    const unsigned char *protocol_name_list_end;
    size_t protocol_name_len;

    /* If ALPN not configured, just ignore the extension */
    if (ssl->conf->alpn_list == NULL) {
        MINTLS_RETURN(0);
    }

    /*
     * RFC7301, section 3.1
     *      opaque ProtocolName<1..2^8-1>;
     *
     *      struct {
     *          ProtocolName protocol_name_list<2..2^16-1>
     *      } ProtocolNameList;
     */

    /*
     * protocol_name_list_len    2 bytes
     * protocol_name_len         1 bytes
     * protocol_name             >=1 byte
     */
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 4);

    protocol_name_list_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, protocol_name_list_len);
    protocol_name_list = p;
    protocol_name_list_end = p + protocol_name_list_len;

    /* Validate peer's list (lengths) */
    while (p < protocol_name_list_end) {
        protocol_name_len = *p++;
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, protocol_name_list_end,
                                     protocol_name_len);
        if (protocol_name_len == 0) {
            MBEDTLS_SSL_PEND_FATAL_ALERT(
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        }

        p += protocol_name_len;
    }

    /* Use our order of preference */
    for (const char **alpn = ssl->conf->alpn_list; *alpn != NULL; alpn++) {
        size_t const alpn_len = strlen(*alpn);
        p = protocol_name_list;
        while (p < protocol_name_list_end) {
            protocol_name_len = *p++;
            if (protocol_name_len == alpn_len &&
                memcmp(p, *alpn, alpn_len) == 0) {
                ssl->alpn_chosen = *alpn;
                MINTLS_RETURN(0);
            }

            p += protocol_name_len;
        }
    }

    /* If we get here, no match was found */
    MBEDTLS_SSL_PEND_FATAL_ALERT(
        MBEDTLS_SSL_ALERT_MSG_NO_APPLICATION_PROTOCOL,
        MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL);
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL);
}

int mbedtls_ssl_write_alpn_ext(mbedtls_ssl_context *ssl,
                               unsigned char *buf,
                               unsigned char *end,
                               size_t *out_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = buf;
    size_t protocol_name_len;
    *out_len = 0;

    if (ssl->alpn_chosen == NULL) {
        MINTLS_RETURN(0);
    }

    protocol_name_len = strlen(ssl->alpn_chosen);
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 7 + protocol_name_len);

    MBEDTLS_SSL_DEBUG_MSG(3, ("server side, adding alpn extension"));
    /*
     * 0 . 1    ext identifier
     * 2 . 3    ext length
     * 4 . 5    protocol list length
     * 6 . 6    protocol name length
     * 7 . 7+n  protocol name
     */
    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_ALPN, p, 0);

    *out_len = 7 + protocol_name_len;

    MBEDTLS_PUT_UINT16_BE(protocol_name_len + 3, p, 2);
    MBEDTLS_PUT_UINT16_BE(protocol_name_len + 1, p, 4);
    /* Note: the length of the chosen protocol has been checked to be less
     * than 255 bytes in `mbedtls_ssl_conf_alpn_protocols`.
     */
    p[6] = MBEDTLS_BYTE_0(protocol_name_len);

    memcpy(p + 7, ssl->alpn_chosen, protocol_name_len);

    MINTLS_RETURN(0);
}

/*
 * The following functions are used by 1.2 and 1.3, client and server.
 */
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
int mbedtls_ssl_check_cert_usage(const mbedtls_x509_crt *cert,
                                 const mbedtls_ssl_ciphersuite_t *ciphersuite,
                                 int recv_endpoint,
                                 mbedtls_ssl_protocol_version tls_version,
                                 uint32_t *flags, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    unsigned int usage = 0;
    const char *ext_oid;
    size_t ext_len;

    /*
     * keyUsage
     */

    /* Note: don't guard this with MBEDTLS_SSL_CLI_C because the server wants
     * to check what a compliant client will think while choosing which cert
     * to send to the client. */
    if (tls_version == MBEDTLS_SSL_VERSION_TLS1_2 &&
        recv_endpoint == MBEDTLS_SSL_IS_CLIENT) {
        /* TLS 1.2 server part of the key exchange */
        switch (ciphersuite->key_exchange) {
            case MBEDTLS_KEY_EXCHANGE_RSA:
            case MBEDTLS_KEY_EXCHANGE_RSA_PSK:
                usage = MBEDTLS_X509_KU_KEY_ENCIPHERMENT;
                break;

            case MBEDTLS_KEY_EXCHANGE_DHE_RSA:
            case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:
            case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:
                usage = MBEDTLS_X509_KU_DIGITAL_SIGNATURE;
                break;

            case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:
            case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:
                usage = MBEDTLS_X509_KU_KEY_AGREEMENT;
                break;

            /* Don't use default: we want warnings when adding new values */
            case MBEDTLS_KEY_EXCHANGE_NONE:
            case MBEDTLS_KEY_EXCHANGE_PSK:
            case MBEDTLS_KEY_EXCHANGE_DHE_PSK:
            case MBEDTLS_KEY_EXCHANGE_ECDHE_PSK:
            case MBEDTLS_KEY_EXCHANGE_ECJPAKE:
                usage = 0;
        }
    } else
    {
        /* This is either TLS 1.3 authentication, which always uses signatures,
         * or 1.2 client auth: rsa_sign and mbedtls_ecdsa_sign are the only
         * options we implement, both using signatures. */
        (void) tls_version;
        (void) ciphersuite;
        usage = MBEDTLS_X509_KU_DIGITAL_SIGNATURE;
    }

    if (mbedtls_x509_crt_check_key_usage(cert, usage, diagnostics) != 0) {
        *flags |= MBEDTLS_X509_BADCERT_KEY_USAGE;
        ret = -1;
    }

    /*
     * extKeyUsage
     */

    if (recv_endpoint == MBEDTLS_SSL_IS_CLIENT) {
        ext_oid = MBEDTLS_OID_SERVER_AUTH;
        ext_len = MBEDTLS_OID_SIZE(MBEDTLS_OID_SERVER_AUTH);
    } else {
        ext_oid = MBEDTLS_OID_CLIENT_AUTH;
        ext_len = MBEDTLS_OID_SIZE(MBEDTLS_OID_CLIENT_AUTH);
    }

    if (mbedtls_x509_crt_check_extended_key_usage(cert, ext_oid, ext_len, diagnostics) != 0) {
        *flags |= MBEDTLS_X509_BADCERT_EXT_KEY_USAGE;
        ret = -1;
    }

    MINTLS_RETURN(ret);
}

int get_hostname_for_verification(mbedtls_ssl_context *ssl,
                                         const char **hostname, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (!mbedtls_ssl_has_set_hostname_been_called(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Certificate verification without having set hostname"));
#if !defined(MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME)
        if (mbedtls_ssl_conf_get_endpoint(ssl->conf) == MBEDTLS_SSL_IS_CLIENT &&
            ssl->conf->authmode == MBEDTLS_SSL_VERIFY_REQUIRED) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME);
        }
#endif
    }

    *hostname = mbedtls_ssl_get_hostname_pointer(ssl);
    if (*hostname == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(2, ("Certificate verification without CN verification"));
    }

    MINTLS_RETURN(0);
}

int mbedtls_ssl_verify_certificate(mbedtls_ssl_context *ssl,
                                   int authmode,
                                   mbedtls_x509_crt *chain,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info,
                                   void *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (authmode == MBEDTLS_SSL_VERIFY_NONE) {
        ssl->session_negotiate->verify_result = 0;
        MINTLS_RETURN(0);
    }

    /*
     * Primary check: use the appropriate X.509 verification function
     */
    int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics);
    void *p_vrfy;
    if (ssl->f_vrfy != NULL) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("Use context-specific verification callback"));
        f_vrfy = ssl->f_vrfy;
        p_vrfy = ssl->p_vrfy;
    } else {
        MBEDTLS_SSL_DEBUG_MSG(3, ("Use configuration-specific verification callback"));
        f_vrfy = ssl->conf->f_vrfy;
        p_vrfy = ssl->conf->p_vrfy;
    }

    const char *hostname = "";
    int ret = get_hostname_for_verification(ssl, &hostname, diagnostics);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "get_hostname_for_verification", ret);
        MINTLS_RETURN(ret);
    }

    int have_ca_chain_or_callback = 0;
#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
    if (ssl->conf->f_ca_cb != NULL) {
        ((void) rs_ctx);
        have_ca_chain_or_callback = 1;

        MBEDTLS_SSL_DEBUG_MSG(3, ("use CA callback for X.509 CRT verification"));
        ret = mbedtls_x509_crt_verify_with_ca_cb(
            chain,
            ssl->conf->f_ca_cb,
            ssl->conf->p_ca_cb,
            ssl->conf->cert_profile,
            hostname,
            &ssl->session_negotiate->verify_result,
            f_vrfy, p_vrfy, diagnostics);
    } else
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */
    {
        mbedtls_x509_crt *ca_chain;
        mbedtls_x509_crl *ca_crl;
        if (ssl->handshake->sni_ca_chain != NULL) {
            ca_chain = ssl->handshake->sni_ca_chain;
            ca_crl   = ssl->handshake->sni_ca_crl;
        } else
        {
            ca_chain = ssl->conf->ca_chain;
            ca_crl   = ssl->conf->ca_crl;
        }

        if (ca_chain != NULL) {
            have_ca_chain_or_callback = 1;
        }

        ret = mbedtls_x509_crt_verify_restartable(
            chain,
            ca_chain, ca_crl,
            ssl->conf->cert_profile,
            hostname,
            &ssl->session_negotiate->verify_result,
            f_vrfy, p_vrfy, rs_ctx, diagnostics);
    }

    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "x509_verify_cert", ret);
    }

    /*
     * Secondary checks: always done, but change 'ret' only if it was 0
     */

    /* With TLS 1.2 and ECC certs, check that the curve used by the
     * certificate is on our list of acceptable curves.
     *
     * With TLS 1.3 this is not needed because the curve is part of the
     * signature algorithm (eg ecdsa_secp256r1_sha256) which is checked when
     * we validate the signature made with the key associated to this cert.
     */
    if (ssl->tls_version == MBEDTLS_SSL_VERSION_TLS1_2 &&
        mbedtls_pk_can_do(&chain->pk, MBEDTLS_PK_ECKEY)) {
        if (mbedtls_ssl_check_curve(ssl, mbedtls_pk_get_ec_group_id(&chain->pk), diagnostics) != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate (EC key curve)"));
            ssl->session_negotiate->verify_result |= MBEDTLS_X509_BADCERT_BAD_KEY;
            if (ret == 0) {
                ret = MinTlsMapDiagnostic(diagnostics, MBEDTLS_ERR_SSL_BAD_CERTIFICATE);
            }
        }
    }

    /* Check X.509 usage extensions (keyUsage, extKeyUsage) */
    if (mbedtls_ssl_check_cert_usage(chain,
                                     ciphersuite_info,
                                     ssl->conf->endpoint,
                                     ssl->tls_version,
                                     &ssl->session_negotiate->verify_result, diagnostics) != 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad certificate (usage extensions)"));
        if (ret == 0) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_BAD_CERTIFICATE);
        }
    }

    /* With authmode optional, we want to keep going if the certificate was
     * unacceptable, but still fail on other errors (out of memory etc),
     * including fatal errors from the f_vrfy callback.
     *
     * The only acceptable errors are:
     * - MBEDTLS_ERR_X509_CERT_VERIFY_FAILED: cert rejected by primary check;
     * - MBEDTLS_ERR_SSL_BAD_CERTIFICATE: cert rejected by secondary checks.
     * Anything else is a fatal error. */
    if (authmode == MBEDTLS_SSL_VERIFY_OPTIONAL &&
        (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED ||
         ret == MBEDTLS_ERR_SSL_BAD_CERTIFICATE)) {
        ret = 0;
    }

    /* Return a specific error as this is a user error: inconsistent
     * configuration - can't verify without trust anchors. */
    if (have_ca_chain_or_callback == 0 && authmode == MBEDTLS_SSL_VERIFY_REQUIRED) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("got no CA chain"));
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED);
    }

    if (ret != 0) {
        uint8_t alert;

        /* The certificate may have been rejected for several reasons.
           Pick one and send the corresponding alert. Which alert to send
           may be a subject of debate in some cases. */
        if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_OTHER) {
            alert = MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_CN_MISMATCH) {
            alert = MBEDTLS_SSL_ALERT_MSG_BAD_CERT;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_KEY_USAGE) {
            alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_EXT_KEY_USAGE) {
            alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_BAD_PK) {
            alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_BAD_KEY) {
            alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_EXPIRED) {
            alert = MBEDTLS_SSL_ALERT_MSG_CERT_EXPIRED;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_REVOKED) {
            alert = MBEDTLS_SSL_ALERT_MSG_CERT_REVOKED;
        } else if (ssl->session_negotiate->verify_result & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
            alert = MBEDTLS_SSL_ALERT_MSG_UNKNOWN_CA;
        } else {
            alert = MBEDTLS_SSL_ALERT_MSG_CERT_UNKNOWN;
        }
        mbedtls_ssl_send_alert_message(ssl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                       alert, diagnostics);
    }

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT)

int mbedtls_ssl_tls12_export_keying_material(const mbedtls_ssl_context *ssl,
                                                    const mbedtls_md_type_t hash_alg,
                                                    uint8_t *out,
                                                    const size_t key_len,
                                                    const char *label,
                                                    const size_t label_len,
                                                    const unsigned char *context,
                                                    const size_t context_len,
                                                    const int use_context, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    unsigned char *prf_input = NULL;

    /* The input to the PRF is client_random, then server_random.
     * If a context is provided, this is then followed by the context length
     * as a 16-bit big-endian integer, and then the context itself. */
    const size_t randbytes_len = MBEDTLS_CLIENT_HELLO_RANDOM_LEN + MBEDTLS_SERVER_HELLO_RANDOM_LEN;
    size_t prf_input_len = randbytes_len;
    if (use_context) {
        if (context_len > UINT16_MAX) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
        }

        /* This does not overflow a 32-bit size_t because the current value of
         * prf_input_len is 64 (length of client_random + server_random) and
         * context_len fits into two bytes (checked above). */
        prf_input_len += sizeof(uint16_t) + context_len;
    }

    prf_input = mbedtls_calloc(prf_input_len, sizeof(unsigned char));
    if (prf_input == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_ALLOC_FAILED);
    }

    memcpy(prf_input,
           ssl->transform->randbytes + MBEDTLS_SERVER_HELLO_RANDOM_LEN,
           MBEDTLS_CLIENT_HELLO_RANDOM_LEN);
    memcpy(prf_input + MBEDTLS_CLIENT_HELLO_RANDOM_LEN,
           ssl->transform->randbytes,
           MBEDTLS_SERVER_HELLO_RANDOM_LEN);
    if (use_context) {
        MBEDTLS_PUT_UINT16_BE(context_len, prf_input, randbytes_len);
        memcpy(prf_input + randbytes_len + sizeof(uint16_t), context, context_len);
    }
    ret = tls_prf_generic(hash_alg, ssl->session->master, sizeof(ssl->session->master),
                          label, label_len,
                          prf_input, prf_input_len,
                          out, key_len, diagnostics);
    mbedtls_free(prf_input);
    MINTLS_RETURN(ret);
}

int mbedtls_ssl_export_keying_material(mbedtls_ssl_context *ssl,
                                       uint8_t *out, const size_t key_len,
                                       const char *label, const size_t label_len,
                                       const unsigned char *context, const size_t context_len,
                                       const int use_context, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (!mbedtls_ssl_is_handshake_over(ssl)) {
        /* TODO: Change this to a more appropriate error code when one is available. */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (key_len > MBEDTLS_SSL_EXPORT_MAX_KEY_LEN) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    int ciphersuite_id = mbedtls_ssl_get_ciphersuite_id_from_ssl(ssl);
    const mbedtls_ssl_ciphersuite_t *ciphersuite = mbedtls_ssl_ciphersuite_from_id(ciphersuite_id);
    const mbedtls_md_type_t hash_alg = ciphersuite->mac;

    switch (mbedtls_ssl_get_version_number(ssl)) {
        case MBEDTLS_SSL_VERSION_TLS1_2:
            MINTLS_RETURN(mbedtls_ssl_tls12_export_keying_material(ssl, hash_alg, out, key_len,
                                                            label, label_len,
                                                            context, context_len, use_context, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION);
    }
}

#endif /* defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT) */
