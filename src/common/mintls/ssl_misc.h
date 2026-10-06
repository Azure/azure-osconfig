/**
 * \file ssl_misc.h
 *
 * \brief Internal functions shared by the SSL modules
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: library/ssl_misc.h.
 */
#ifndef MBEDTLS_SSL_MISC_H
#define MBEDTLS_SSL_MISC_H

#include "build_info.h"
#include "common.h"

#include "error.h"

#include "ssl.h"
#include "debug.h"
#include "debug_internal.h"

#include "cipher.h"

#include "sha1.h"

#include "sha256.h"

#include "sha512.h"

#include "pk.h"
#include "ssl_ciphersuites_internal.h"
#include "x509_internal.h"
#include "pk_internal.h"

/* Shorthand for restartable ECC */

#define MBEDTLS_SSL_INITIAL_HANDSHAKE           0
#define MBEDTLS_SSL_RENEGOTIATION_IN_PROGRESS   1   /* In progress */
#define MBEDTLS_SSL_RENEGOTIATION_DONE          2   /* Done or aborted */
#define MBEDTLS_SSL_RENEGOTIATION_PENDING       3   /* Requested (server only) */

/* Faked handshake message identity for HelloRetryRequest. */
#define MBEDTLS_SSL_TLS1_3_HS_HELLO_RETRY_REQUEST (-MBEDTLS_SSL_HS_SERVER_HELLO)

/*
 * Internal identity of handshake extensions
 */
#define MBEDTLS_SSL_EXT_ID_UNRECOGNIZED                0
#define MBEDTLS_SSL_EXT_ID_SERVERNAME                  1
#define MBEDTLS_SSL_EXT_ID_SERVERNAME_HOSTNAME         1
#define MBEDTLS_SSL_EXT_ID_MAX_FRAGMENT_LENGTH         2
#define MBEDTLS_SSL_EXT_ID_STATUS_REQUEST              3
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_GROUPS            4
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_ELLIPTIC_CURVES   4
#define MBEDTLS_SSL_EXT_ID_SIG_ALG                     5
#define MBEDTLS_SSL_EXT_ID_USE_SRTP                    6
#define MBEDTLS_SSL_EXT_ID_HEARTBEAT                   7
#define MBEDTLS_SSL_EXT_ID_ALPN                        8
#define MBEDTLS_SSL_EXT_ID_SCT                         9
#define MBEDTLS_SSL_EXT_ID_CLI_CERT_TYPE              10
#define MBEDTLS_SSL_EXT_ID_SERV_CERT_TYPE             11
#define MBEDTLS_SSL_EXT_ID_PADDING                    12
#define MBEDTLS_SSL_EXT_ID_PRE_SHARED_KEY             13
#define MBEDTLS_SSL_EXT_ID_EARLY_DATA                 14
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_VERSIONS         15
#define MBEDTLS_SSL_EXT_ID_COOKIE                     16
#define MBEDTLS_SSL_EXT_ID_PSK_KEY_EXCHANGE_MODES     17
#define MBEDTLS_SSL_EXT_ID_CERT_AUTH                  18
#define MBEDTLS_SSL_EXT_ID_OID_FILTERS                19
#define MBEDTLS_SSL_EXT_ID_POST_HANDSHAKE_AUTH        20
#define MBEDTLS_SSL_EXT_ID_SIG_ALG_CERT               21
#define MBEDTLS_SSL_EXT_ID_KEY_SHARE                  22
#define MBEDTLS_SSL_EXT_ID_TRUNCATED_HMAC             23
#define MBEDTLS_SSL_EXT_ID_SUPPORTED_POINT_FORMATS    24
#define MBEDTLS_SSL_EXT_ID_ENCRYPT_THEN_MAC           25
#define MBEDTLS_SSL_EXT_ID_EXTENDED_MASTER_SECRET     26
#define MBEDTLS_SSL_EXT_ID_SESSION_TICKET             27
#define MBEDTLS_SSL_EXT_ID_RECORD_SIZE_LIMIT          28

/* Utility for translating IANA extension type. */
uint32_t mbedtls_ssl_get_extension_id(unsigned int extension_type);
uint32_t mbedtls_ssl_get_extension_mask(unsigned int extension_type);
/* Macros used to define mask constants */
#define MBEDTLS_SSL_EXT_MASK(id)       (1ULL << (MBEDTLS_SSL_EXT_ID_##id))
/* Reset value of extension mask */
#define MBEDTLS_SSL_EXT_MASK_NONE                                              0

/* In messages containing extension requests, we should ignore unrecognized
 * extensions. In messages containing extension responses, unrecognized
 * extensions should result in handshake abortion. Messages containing
 * extension requests include ClientHello, CertificateRequest and
 * NewSessionTicket. Messages containing extension responses include
 * ServerHello, HelloRetryRequest, EncryptedExtensions and Certificate.
 *
 * RFC 8446 section 4.1.3
 *
 * The ServerHello MUST only include extensions which are required to establish
 * the cryptographic context and negotiate the protocol version.
 *
 * RFC 8446 section 4.2
 *
 * If an implementation receives an extension which it recognizes and which is
 * not specified for the message in which it appears, it MUST abort the handshake
 * with an "illegal_parameter" alert.
 */

/* Extensions that are not recognized by TLS 1.3 */
#define MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED                               \
    (MBEDTLS_SSL_EXT_MASK(SUPPORTED_POINT_FORMATS)                | \
     MBEDTLS_SSL_EXT_MASK(ENCRYPT_THEN_MAC)                       | \
     MBEDTLS_SSL_EXT_MASK(EXTENDED_MASTER_SECRET)                 | \
     MBEDTLS_SSL_EXT_MASK(SESSION_TICKET)                         | \
     MBEDTLS_SSL_EXT_MASK(TRUNCATED_HMAC)                         | \
     MBEDTLS_SSL_EXT_MASK(UNRECOGNIZED))

/* RFC 8446 section 4.2. Allowed extensions for ClientHello */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CH                                  \
    (MBEDTLS_SSL_EXT_MASK(SERVERNAME)                             | \
     MBEDTLS_SSL_EXT_MASK(MAX_FRAGMENT_LENGTH)                    | \
     MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_GROUPS)                       | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG)                                | \
     MBEDTLS_SSL_EXT_MASK(USE_SRTP)                               | \
     MBEDTLS_SSL_EXT_MASK(HEARTBEAT)                              | \
     MBEDTLS_SSL_EXT_MASK(ALPN)                                   | \
     MBEDTLS_SSL_EXT_MASK(SCT)                                    | \
     MBEDTLS_SSL_EXT_MASK(CLI_CERT_TYPE)                          | \
     MBEDTLS_SSL_EXT_MASK(SERV_CERT_TYPE)                         | \
     MBEDTLS_SSL_EXT_MASK(PADDING)                                | \
     MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY)                         | \
     MBEDTLS_SSL_EXT_MASK(PSK_KEY_EXCHANGE_MODES)                 | \
     MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_EXT_MASK(COOKIE)                                 | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS)                     | \
     MBEDTLS_SSL_EXT_MASK(CERT_AUTH)                              | \
     MBEDTLS_SSL_EXT_MASK(POST_HANDSHAKE_AUTH)                    | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG_CERT)                           | \
     MBEDTLS_SSL_EXT_MASK(RECORD_SIZE_LIMIT)                      | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/* RFC 8446 section 4.2. Allowed extensions for EncryptedExtensions */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_EE                                  \
    (MBEDTLS_SSL_EXT_MASK(SERVERNAME)                             | \
     MBEDTLS_SSL_EXT_MASK(MAX_FRAGMENT_LENGTH)                    | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_GROUPS)                       | \
     MBEDTLS_SSL_EXT_MASK(USE_SRTP)                               | \
     MBEDTLS_SSL_EXT_MASK(HEARTBEAT)                              | \
     MBEDTLS_SSL_EXT_MASK(ALPN)                                   | \
     MBEDTLS_SSL_EXT_MASK(CLI_CERT_TYPE)                          | \
     MBEDTLS_SSL_EXT_MASK(SERV_CERT_TYPE)                         | \
     MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_EXT_MASK(RECORD_SIZE_LIMIT))

/* RFC 8446 section 4.2. Allowed extensions for CertificateRequest */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CR                                  \
    (MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG)                                | \
     MBEDTLS_SSL_EXT_MASK(SCT)                                    | \
     MBEDTLS_SSL_EXT_MASK(CERT_AUTH)                              | \
     MBEDTLS_SSL_EXT_MASK(OID_FILTERS)                            | \
     MBEDTLS_SSL_EXT_MASK(SIG_ALG_CERT)                           | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/* RFC 8446 section 4.2. Allowed extensions for Certificate */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CT                                  \
    (MBEDTLS_SSL_EXT_MASK(STATUS_REQUEST)                         | \
     MBEDTLS_SSL_EXT_MASK(SCT))

/* RFC 8446 section 4.2. Allowed extensions for ServerHello */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_SH                                  \
    (MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY)                         | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS))

/* RFC 8446 section 4.2. Allowed extensions for HelloRetryRequest */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_HRR                                 \
    (MBEDTLS_SSL_EXT_MASK(KEY_SHARE)                              | \
     MBEDTLS_SSL_EXT_MASK(COOKIE)                                 | \
     MBEDTLS_SSL_EXT_MASK(SUPPORTED_VERSIONS))

/* RFC 8446 section 4.2. Allowed extensions for NewSessionTicket */
#define MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_NST                                 \
    (MBEDTLS_SSL_EXT_MASK(EARLY_DATA)                             | \
     MBEDTLS_SSL_TLS1_3_EXT_MASK_UNRECOGNIZED)

/*
 * Helper macros for function call with return check.
 */
/*
 * Exit when return non-zero value
 */
#define MBEDTLS_SSL_PROC_CHK(f)                               \
    do {                                                        \
        ret = (f);                                            \
        if (ret != 0)                                          \
        {                                                       \
            goto cleanup;                                       \
        }                                                       \
    } while (0)
/*
 * Exit when return negative value
 */
#define MBEDTLS_SSL_PROC_CHK_NEG(f)                           \
    do {                                                        \
        ret = (f);                                            \
        if (ret < 0)                                           \
        {                                                       \
            goto cleanup;                                       \
        }                                                       \
    } while (0)

/*
 * DTLS retransmission states, see RFC 6347 4.2.4
 *
 * The SENDING state is merged in PREPARING for initial sends,
 * but is distinct for resends.
 *
 * Note: initial state is wrong for server, but is not used anyway.
 */
#define MBEDTLS_SSL_RETRANS_PREPARING       0
#define MBEDTLS_SSL_RETRANS_SENDING         1
#define MBEDTLS_SSL_RETRANS_WAITING         2
#define MBEDTLS_SSL_RETRANS_FINISHED        3

/*
 * Allow extra bytes for record, authentication and encryption overhead:
 * counter (8) + header (5) + IV(16) + MAC (16-48) + padding (0-256).
 */

/* This macro determines whether CBC is supported. */

/* This macro determines whether a ciphersuite using a
 * stream cipher can be used. */

/* This macro determines whether the CBC construct used in TLS 1.2 is supported. */

/* This macro determines whether a ciphersuite uses Encrypt-then-MAC with CBC */

/* AEAD ciphersuites: GCM and CCM use a 128 bits tag */
#define MBEDTLS_SSL_MAC_ADD                 16

#define MBEDTLS_SSL_PADDING_ADD              0

#define MBEDTLS_SSL_MAX_CID_EXPANSION        0

#define MBEDTLS_SSL_PAYLOAD_OVERHEAD (MBEDTLS_MAX_IV_LENGTH +          \
                                      MBEDTLS_SSL_MAC_ADD +            \
                                      MBEDTLS_SSL_PADDING_ADD +        \
                                      MBEDTLS_SSL_MAX_CID_EXPANSION    \
                                      )

#define MBEDTLS_SSL_IN_PAYLOAD_LEN (MBEDTLS_SSL_PAYLOAD_OVERHEAD + \
                                    (MBEDTLS_SSL_IN_CONTENT_LEN))

#define MBEDTLS_SSL_OUT_PAYLOAD_LEN (MBEDTLS_SSL_PAYLOAD_OVERHEAD + \
                                     (MBEDTLS_SSL_OUT_CONTENT_LEN))

/* The maximum number of buffered handshake messages. */
#define MBEDTLS_SSL_MAX_BUFFERED_HS 4

/* Maximum length we can advertise as our max content length for
   RFC 6066 max_fragment_length extension negotiation purposes
   (the lesser of both sizes, if they are unequal.)
 */
#define MBEDTLS_TLS_EXT_ADV_CONTENT_LEN (                            \
        (MBEDTLS_SSL_IN_CONTENT_LEN > MBEDTLS_SSL_OUT_CONTENT_LEN)   \
        ? (MBEDTLS_SSL_OUT_CONTENT_LEN)                            \
        : (MBEDTLS_SSL_IN_CONTENT_LEN)                             \
        )

/* Maximum size in bytes of list in signature algorithms ext., RFC 5246/8446 */
#define MBEDTLS_SSL_MAX_SIG_ALG_LIST_LEN       65534

/* Minimum size in bytes of list in signature algorithms ext., RFC 5246/8446 */
#define MBEDTLS_SSL_MIN_SIG_ALG_LIST_LEN       2

/* Maximum size in bytes of list in supported elliptic curve ext., RFC 4492 */
#define MBEDTLS_SSL_MAX_CURVE_LIST_LEN         65535

#define MBEDTLS_RECEIVED_SIG_ALGS_SIZE         20

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#define MBEDTLS_TLS_SIG_NONE MBEDTLS_TLS1_3_SIG_NONE

#define MBEDTLS_SSL_TLS12_SIG_AND_HASH_ALG(sig, hash) ((hash << 8) | sig)
#define MBEDTLS_SSL_TLS12_SIG_ALG_FROM_SIG_AND_HASH_ALG(alg) (alg & 0xFF)
#define MBEDTLS_SSL_TLS12_HASH_ALG_FROM_SIG_AND_HASH_ALG(alg) (alg >> 8)

#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/*
 * Check that we obey the standard's message size bounds
 */

#if MBEDTLS_SSL_IN_CONTENT_LEN > 16384
#error "Bad configuration - incoming record content too large."
#endif

#if MBEDTLS_SSL_OUT_CONTENT_LEN > 16384
#error "Bad configuration - outgoing record content too large."
#endif

#if MBEDTLS_SSL_IN_PAYLOAD_LEN > MBEDTLS_SSL_IN_CONTENT_LEN + 2048
#error "Bad configuration - incoming protected record payload too large."
#endif

#if MBEDTLS_SSL_OUT_PAYLOAD_LEN > MBEDTLS_SSL_OUT_CONTENT_LEN + 2048
#error "Bad configuration - outgoing protected record payload too large."
#endif

/* Calculate buffer sizes */

/* Note: Even though the TLS record header is only 5 bytes
   long, we're internally using 8 bytes to store the
   implicit sequence number. */
#define MBEDTLS_SSL_HEADER_LEN 13

#define MBEDTLS_SSL_IN_BUFFER_LEN  \
    ((MBEDTLS_SSL_HEADER_LEN) + (MBEDTLS_SSL_IN_PAYLOAD_LEN))

#define MBEDTLS_SSL_OUT_BUFFER_LEN  \
    ((MBEDTLS_SSL_HEADER_LEN) + (MBEDTLS_SSL_OUT_PAYLOAD_LEN))

#define MBEDTLS_CLIENT_HELLO_RANDOM_LEN 32
#define MBEDTLS_SERVER_HELLO_RANDOM_LEN 32

/** Get `ssl->badmac_seen`. This field is encoded as
 * mbedtls_ssl_context::badmac_seen_or_in_hsfraglen in DTLS contexts,
 * and doesn't exist in TLS contexts.
 *
 * \param[in] ssl       The SSL context to read.
 *
 * \return In DTLS, the value of `badmac_seen`. In TLS, 0 (there can't have
 *         been a record with a bad MAC in TLS, since those abort the
 *         connection immediately).
 */
static inline unsigned mbedtls_ssl_get_badmac_seen(const mbedtls_ssl_context *ssl)
{
    (void) ssl;
    return 0;
}

/** Get `ssl->in_hsfraglen`. This field is encoded as
 * mbedtls_ssl_context::badmac_seen_or_in_hsfraglen in TLS contexts,
 * and doesn't exist in DTLS contexts.
 *
 * \param[in] ssl       The SSL context to read.
 *
 * \return In TLS, the value of `in_hsfraglen`. In DTLS, 0 (handshake
 *         message defragmentation is handled different in DTLS, and
 *         does not use this field).
 */
static inline unsigned mbedtls_ssl_get_in_hsfraglen(const mbedtls_ssl_context *ssl)
{
    return ssl->badmac_seen_or_in_hsfraglen;
}

/** Set `ssl->in_hsfraglen`. This field is encoded as
 * mbedtls_ssl_context::badmac_seen_or_in_hsfraglen in TLS contexts,
 * and doesn't exist in DTLS contexts.
 *
 * \param[in,out] ssl   The SSL context to modify.
 * \param in_hsfraglen  The new value of `in_hsfraglen`.
 *
 * \return 0 in TLS, #MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED in DTLS.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
static inline int mbedtls_ssl_set_in_hsfraglen(mbedtls_ssl_context *ssl,
                                               unsigned in_hsfraglen)
{
    ssl->badmac_seen_or_in_hsfraglen = in_hsfraglen;
    return 0;
}

/*
 * TLS extension flags (for extensions with outgoing ServerHello content
 * that need it (e.g. for RENEGOTIATION_INFO the server already knows because
 * of state of the renegotiation flag, so no indicator is required)
 */
#define MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS_PRESENT (1 << 0)
#define MBEDTLS_TLS_EXT_ECJPAKE_KKPP_OK                 (1 << 1)

/**
 * \brief        This function checks if the remaining size in a buffer is
 *               greater or equal than a needed space.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed space in bytes.
 *
 * \return       Zero if the needed space is available in the buffer, non-zero
 *               otherwise.
 */
static inline int mbedtls_ssl_chk_buf_ptr(const uint8_t *cur,
                                          const uint8_t *end, size_t need)
{
    return (cur > end) || (need > (size_t) (end - cur));
}

/**
 * \brief        This macro checks if the remaining size in a buffer is
 *               greater or equal than a needed space. If it is not the case,
 *               it returns an SSL_BUFFER_TOO_SMALL error.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed space in bytes.
 *
 */
#define MBEDTLS_SSL_CHK_BUF_PTR(cur, end, need)                        \
    do {                                                                 \
        if (mbedtls_ssl_chk_buf_ptr((cur), (end), (need)) != 0) \
        {                                                                \
            return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;                  \
        }                                                                \
    } while (0)

/**
 * \brief        This macro checks if the remaining length in an input buffer is
 *               greater or equal than a needed length. If it is not the case, it
 *               returns #MBEDTLS_ERR_SSL_DECODE_ERROR error and pends a
 *               #MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR alert message.
 *
 *               This is a function-like macro. It is guaranteed to evaluate each
 *               argument exactly once.
 *
 * \param cur    Pointer to the current position in the buffer.
 * \param end    Pointer to one past the end of the buffer.
 * \param need   Needed length in bytes.
 *
 */
#define MBEDTLS_SSL_CHK_BUF_READ_PTR(cur, end, need)                          \
    do {                                                                        \
        if (mbedtls_ssl_chk_buf_ptr((cur), (end), (need)) != 0)        \
        {                                                                       \
            MBEDTLS_SSL_DEBUG_MSG(1,                                           \
                                  ("missing input data in %s", __func__));  \
            MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,   \
                                         MBEDTLS_ERR_SSL_DECODE_ERROR);       \
            return MBEDTLS_ERR_SSL_DECODE_ERROR;                             \
        }                                                                       \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

typedef int  mbedtls_ssl_tls_prf_cb(const unsigned char *secret, size_t slen,
                                    const char *label,
                                    const unsigned char *random, size_t rlen,
                                    unsigned char *dstbuf, size_t dlen);

/* cipher.h exports the maximum IV, key and block length from
 * all ciphers enabled in the config, regardless of whether those
 * ciphers are actually usable in SSL/TLS. Notably, XTS is enabled
 * in the default configuration and uses 64 Byte keys, but it is
 * not used for record protection in SSL/TLS.
 *
 * In order to prevent unnecessary inflation of key structures,
 * we introduce SSL-specific variants of the max-{key,block,IV}
 * macros here which are meant to only take those ciphers into
 * account which can be negotiated in SSL/TLS.
 *
 * Since the current definitions of MBEDTLS_MAX_{KEY|BLOCK|IV}_LENGTH
 * in cipher.h are rough overapproximations of the real maxima, here
 * we content ourselves with replicating those overapproximations
 * for the maximum block and IV length, and excluding XTS from the
 * computation of the maximum key length. */
#define MBEDTLS_SSL_MAX_BLOCK_LENGTH 16
#define MBEDTLS_SSL_MAX_IV_LENGTH    16
#define MBEDTLS_SSL_MAX_KEY_LENGTH   32

/**
 * \brief   The data structure holding the cryptographic material (key and IV)
 *          used for record protection in TLS 1.3.
 */
struct mbedtls_ssl_key_set {
    /*! The key for client->server records. */
    unsigned char client_write_key[MBEDTLS_SSL_MAX_KEY_LENGTH];
    /*! The key for server->client records. */
    unsigned char server_write_key[MBEDTLS_SSL_MAX_KEY_LENGTH];
    /*! The IV  for client->server records. */
    unsigned char client_write_iv[MBEDTLS_SSL_MAX_IV_LENGTH];
    /*! The IV  for server->client records. */
    unsigned char server_write_iv[MBEDTLS_SSL_MAX_IV_LENGTH];

    size_t key_len; /*!< The length of client_write_key and
                     *   server_write_key, in Bytes. */
    size_t iv_len;  /*!< The length of client_write_iv and
                     *   server_write_iv, in Bytes. */
};
typedef struct mbedtls_ssl_key_set mbedtls_ssl_key_set;

/*
 * This structure contains the parameters only needed during handshake.
 */
struct mbedtls_ssl_handshake_params {
    /* Frequently-used boolean or byte fields (placed early to take
     * advantage of smaller code size for indirect access on Arm Thumb) */
    uint8_t resume;                     /*!<  session resume indicator*/
    uint8_t cli_exts;                   /*!< client extension presence*/

    uint8_t sni_authmode;               /*!< authmode from SNI callback     */

    /** Minimum TLS version to be negotiated.
     *
     * It is set up in the ClientHello writing preparation stage and used
     * throughout the ClientHello writing. Not relevant anymore as soon as
     * the protocol version has been negotiated thus as soon as the
     * ServerHello is received.
     * For a fresh handshake not linked to any previous handshake, it is
     * equal to the configured minimum minor version to be negotiated. When
     * renegotiating or resuming a session, it is equal to the previously
     * negotiated minor version.
     *
     * There is no maximum TLS version field in this handshake context.
     * From the start of the handshake, we need to define a current protocol
     * version for the record layer which we define as the maximum TLS
     * version to be negotiated. The `tls_version` field of the SSL context is
     * used to store this maximum value until it contains the actual
     * negotiated value.
     */
    mbedtls_ssl_protocol_version min_tls_version;

    uint8_t extended_ms;                /*!< use Extended Master Secret? */

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    unsigned char group_list_heap_allocated;
    unsigned char sig_algs_heap_allocated;
#endif

    mbedtls_ssl_ciphersuite_t const *ciphersuite_info;

    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*update_checksum)(mbedtls_ssl_context *, const unsigned char *, size_t);
    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*calc_verify)(const mbedtls_ssl_context *, unsigned char *, size_t *);
    MBEDTLS_CHECK_RETURN_CRITICAL
    int (*calc_finished)(mbedtls_ssl_context *, unsigned char *, int);
    mbedtls_ssl_tls_prf_cb *tls_prf;

    /*
     * Handshake specific crypto variables
     */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
    uint16_t received_sig_algs[MBEDTLS_RECEIVED_SIG_ALGS_SIZE];
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    const uint16_t *group_list;
    const uint16_t *sig_algs;
#endif

#if !defined(MBEDTLS_USE_PSA_CRYPTO) && \
    defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED)
    mbedtls_ecdh_context ecdh_ctx;              /*!<  ECDH key exchange       */
#endif /* !MBEDTLS_USE_PSA_CRYPTO &&
          MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_1_2_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ANY_ALLOWED_ENABLED) || \
    defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    uint16_t *curves_tls_id;      /*!<  List of TLS IDs of supported elliptic curves */
#endif

    mbedtls_ssl_key_cert *key_cert;     /*!< chosen key/cert pair (server)  */
    mbedtls_ssl_key_cert *sni_key_cert; /*!< key/cert list from SNI         */
    mbedtls_x509_crt *sni_ca_chain;     /*!< trusted CAs from SNI callback  */
    mbedtls_x509_crl *sni_ca_crl;       /*!< trusted CAs CRLs from SNI      */

    struct {
        size_t total_bytes_buffered; /*!< Cumulative size of heap allocated
                                      *   buffers used for message buffering. */

        uint8_t seen_ccs;               /*!< Indicates if a CCS message has
                                         *   been seen in the current flight. */

        struct mbedtls_ssl_hs_buffer {
            unsigned is_valid      : 1;
            unsigned is_fragmented : 1;
            unsigned is_complete   : 1;
            unsigned char *data;
            size_t data_len;
        } hs[MBEDTLS_SSL_MAX_BUFFERED_HS];

        struct {
            unsigned char *data;
            size_t len;
            unsigned epoch;
        } future_record;

    } buffering;

    /*
     * Checksum contexts
     */
    mbedtls_md_context_t fin_sha256;
    mbedtls_md_context_t fin_sha384;

    uint8_t client_auth;       /*!< used to check if CertificateRequest has been
                                    received from server side. If CertificateRequest
                                    has been received, Certificate and CertificateVerify
                                    should be sent to server */
    unsigned char randbytes[MBEDTLS_CLIENT_HELLO_RANDOM_LEN +
                            MBEDTLS_SERVER_HELLO_RANDOM_LEN];
    /*!<  random bytes            */
    unsigned char premaster[MBEDTLS_PREMASTER_SIZE];
    /*!<  premaster secret        */
    size_t pmslen;                      /*!<  premaster length        */

    const unsigned char *sni_name;      /*!< raw SNI                        */
    size_t sni_name_len;                /*!< raw SNI len                    */
#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
    const mbedtls_x509_crt *dn_hints;   /*!< acceptable client cert issuers */
#endif
};

typedef struct mbedtls_ssl_hs_buffer mbedtls_ssl_hs_buffer;

/*
 * Representation of decryption/encryption transformations on records
 *
 * There are the following general types of record transformations:
 * - Stream transformations (TLS versions == 1.2 only)
 *   Transformation adding a MAC and applying a stream-cipher
 *   to the authenticated message.
 * - CBC block cipher transformations ([D]TLS versions == 1.2 only)
 *   For TLS 1.2, no IV is generated at key extraction time, but every
 *   encrypted record is explicitly prefixed by the IV with which it was
 *   encrypted.
 * - AEAD transformations ([D]TLS versions == 1.2 only)
 *   These come in two fundamentally different versions, the first one
 *   used in TLS 1.2, excluding ChaChaPoly ciphersuites, and the second
 *   one used for ChaChaPoly ciphersuites in TLS 1.2 as well as for TLS 1.3.
 *   In the first transformation, the IV to be used for a record is obtained
 *   as the concatenation of an explicit, static 4-byte IV and the 8-byte
 *   record sequence number, and explicitly prepending this sequence number
 *   to the encrypted record. In contrast, in the second transformation
 *   the IV is obtained by XOR'ing a static IV obtained at key extraction
 *   time with the 8-byte record sequence number, without prepending the
 *   latter to the encrypted record.
 *
 * Additionally, DTLS 1.2 + CID as well as TLS 1.3 use an inner plaintext
 * which allows to add flexible length padding and to hide a record's true
 * content type.
 *
 * In addition to type and version, the following parameters are relevant:
 * - The symmetric cipher algorithm to be used.
 * - The (static) encryption/decryption keys for the cipher.
 * - For stream/CBC, the type of message digest to be used.
 * - For stream/CBC, (static) encryption/decryption keys for the digest.
 * - For AEAD transformations, the size (potentially 0) of an explicit,
 *   random initialization vector placed in encrypted records.
 * - For some transformations (currently AEAD) an implicit IV. It is static
 *   and (if present) is combined with the explicit IV in a transformation-
 *   -dependent way (e.g. appending in TLS 1.2 and XOR'ing in TLS 1.3).
 * - For stream/CBC, a flag determining the order of encryption and MAC.
 * - The details of the transformation depend on the SSL/TLS version.
 * - The length of the authentication tag.
 *
 * The struct below refines this abstract view as follows:
 * - The cipher underlying the transformation is managed in
 *   cipher contexts cipher_ctx_{enc/dec}, which must have the
 *   same cipher type. The mode of these cipher contexts determines
 *   the type of the transformation in the sense above: e.g., if
 *   the type is MBEDTLS_CIPHER_AES_256_CBC resp. MBEDTLS_CIPHER_AES_192_GCM
 *   then the transformation has type CBC resp. AEAD.
 * - The cipher keys are never stored explicitly but
 *   are maintained within cipher_ctx_{enc/dec}.
 * - For stream/CBC transformations, the message digest contexts
 *   used for the MAC's are stored in md_ctx_{enc/dec}. These contexts
 *   are unused for AEAD transformations.
 * - For stream/CBC transformations, the MAC keys are not stored explicitly
 *   but maintained within md_ctx_{enc/dec}.
 * - The mac_enc and mac_dec fields are unused for EAD transformations.
 * - For transformations using an implicit IV maintained within
 *   the transformation context, its contents are stored within
 *   iv_{enc/dec}.
 * - The value of ivlen indicates the length of the IV.
 *   This is redundant in case of stream/CBC transformations
 *   which always use 0 resp. the cipher's block length as the
 *   IV length, but is needed for AEAD ciphers and may be
 *   different from the underlying cipher's block length
 *   in this case.
 * - The field fixed_ivlen is nonzero for AEAD transformations only
 *   and indicates the length of the static part of the IV which is
 *   constant throughout the communication, and which is stored in
 *   the first fixed_ivlen bytes of the iv_{enc/dec} arrays.
 * - tls_version denotes the 2-byte TLS version
 * - For stream/CBC transformations, maclen denotes the length of the
 *   authentication tag, while taglen is unused and 0.
 * - For AEAD transformations, taglen denotes the length of the
 *   authentication tag, while maclen is unused and 0.
 * - For CBC transformations, encrypt_then_mac determines the
 *   order of encryption and authentication. This field is unused
 *   in other transformations.
 *
 */
struct mbedtls_ssl_transform {
    /*
     * Session specific crypto layer
     */
    size_t minlen;                      /*!<  min. ciphertext length  */
    size_t ivlen;                       /*!<  IV length               */
    size_t fixed_ivlen;                 /*!<  Fixed part of IV (AEAD) */
    size_t maclen;                      /*!<  MAC(CBC) len            */
    size_t taglen;                      /*!<  TAG(AEAD) len           */

    unsigned char iv_enc[16];           /*!<  IV (encryption)         */
    unsigned char iv_dec[16];           /*!<  IV (decryption)         */

    mbedtls_ssl_protocol_version tls_version;

    mbedtls_cipher_context_t cipher_ctx_enc;    /*!<  encryption context      */
    mbedtls_cipher_context_t cipher_ctx_dec;    /*!<  decryption context      */

#if defined(MBEDTLS_SSL_KEEP_RANDBYTES)
    /* We need the Hello random bytes in order to re-derive keys from the
     * Master Secret and other session info and for the keying material
     * exporter in TLS 1.2.
     * See ssl_tls12_populate_transform() */
    unsigned char randbytes[MBEDTLS_SERVER_HELLO_RANDOM_LEN +
                            MBEDTLS_CLIENT_HELLO_RANDOM_LEN];
    /*!< ServerHello.random+ClientHello.random */
#endif /* defined(MBEDTLS_SSL_KEEP_RANDBYTES) */
};

/*
 * Return 1 if the transform uses an AEAD cipher, 0 otherwise.
 * Equivalently, return 0 if a separate MAC is used, 1 otherwise.
 */
static inline int mbedtls_ssl_transform_uses_aead(
    const mbedtls_ssl_transform *transform)
{
    (void) transform;
    return 1;
}

/*
 * Internal representation of record frames
 *
 * Instances come in two flavors:
 * (1) Encrypted
 *     These always have data_offset = 0
 * (2) Unencrypted
 *     These have data_offset set to the amount of
 *     pre-expansion during record protection. Concretely,
 *     this is the length of the fixed part of the explicit IV
 *     used for encryption, or 0 if no explicit IV is used
 *     (e.g. for stream ciphers).
 *
 * The reason for the data_offset in the unencrypted case
 * is to allow for in-place conversion of an unencrypted to
 * an encrypted record. If the offset wasn't included, the
 * encrypted content would need to be shifted afterwards to
 * make space for the fixed IV.
 *
 */
#if MBEDTLS_SSL_CID_OUT_LEN_MAX > MBEDTLS_SSL_CID_IN_LEN_MAX
#define MBEDTLS_SSL_CID_LEN_MAX MBEDTLS_SSL_CID_OUT_LEN_MAX
#else
#define MBEDTLS_SSL_CID_LEN_MAX MBEDTLS_SSL_CID_IN_LEN_MAX
#endif

typedef struct {
    uint8_t ctr[MBEDTLS_SSL_SEQUENCE_NUMBER_LEN];  /* In TLS:  The implicit record sequence number.
                                                    * In DTLS: The 2-byte epoch followed by
                                                    *          the 6-byte sequence number.
                                                    * This is stored as a raw big endian byte array
                                                    * as opposed to a uint64_t because we rarely
                                                    * need to perform arithmetic on this, but do
                                                    * need it as a Byte array for the purpose of
                                                    * MAC computations.                             */
    uint8_t type;           /* The record content type.                      */
    uint8_t ver[2];         /* SSL/TLS version as present on the wire.
                             * Convert to internal presentation of versions
                             * using mbedtls_ssl_read_version() and
                             * mbedtls_ssl_write_version().
                             * Keep wire-format for MAC computations.        */

    unsigned char *buf;     /* Memory buffer enclosing the record content    */
    size_t buf_len;         /* Buffer length                                 */
    size_t data_offset;     /* Offset of record content                      */
    size_t data_len;        /* Length of record content                      */

} mbedtls_record;

/*
 * List of certificate + private key pairs
 */
struct mbedtls_ssl_key_cert {
    mbedtls_x509_crt *cert;                 /*!< cert                       */
    mbedtls_pk_context *key;                /*!< private key                */
    mbedtls_ssl_key_cert *next;             /*!< next key/cert pair         */
};

/**
 * \brief Given an SSL context and its associated configuration, write the TLS
 *        1.2 specific extensions of the ClientHello message.
 *
 * \param[in]   ssl     SSL context
 * \param[in]   buf     Base address of the buffer where to write the extensions
 * \param[in]   end     End address of the buffer where to write the extensions
 * \param       uses_ec Whether one proposed ciphersuite uses an elliptic curve
 *                      (<> 0) or not ( 0 ).
 * \param[out]  out_len Length of the data written into the buffer \p buf
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_tls12_write_client_hello_exts(mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              const unsigned char *end,
                                              int uses_ec,
                                              size_t *out_len);

#if defined(MBEDTLS_SSL_PROTO_TLS1_2) && \
    defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)

/**
 * \brief Find the preferred hash for a given signature algorithm.
 *
 * \param[in]   ssl     SSL context
 * \param[in]   sig_alg A signature algorithm identifier as defined in the
 *                      TLS 1.2 SignatureAlgorithm enumeration.
 *
 * \return  The preferred hash algorithm for \p sig_alg. It is a hash algorithm
 *          identifier as defined in the TLS 1.2 HashAlgorithm enumeration.
 */
unsigned int mbedtls_ssl_tls12_get_preferred_hash_for_sig_alg(
    mbedtls_ssl_context *ssl,
    unsigned int sig_alg);

#endif /* MBEDTLS_SSL_PROTO_TLS1_2 &&
          MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

/**
 * \brief           Free referenced items in an SSL transform context and clear
 *                  memory
 *
 * \param transform SSL transform context
 */
void mbedtls_ssl_transform_free(mbedtls_ssl_transform *transform);

/**
 * \brief           Free referenced items in an SSL handshake context and clear
 *                  memory
 *
 * \param ssl       SSL context
 */
void mbedtls_ssl_handshake_free(mbedtls_ssl_context *ssl);

/* set inbound transform of ssl context */
void mbedtls_ssl_set_inbound_transform(mbedtls_ssl_context *ssl,
                                       mbedtls_ssl_transform *transform);

/* set outbound transform of ssl context */
void mbedtls_ssl_set_outbound_transform(mbedtls_ssl_context *ssl,
                                        mbedtls_ssl_transform *transform);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handshake_client_step(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handshake_server_step(mbedtls_ssl_context *ssl);
void mbedtls_ssl_handshake_wrapup(mbedtls_ssl_context *ssl);

static inline void mbedtls_ssl_handshake_set_state(mbedtls_ssl_context *ssl,
                                                   mbedtls_ssl_states state)
{
    MBEDTLS_SSL_DEBUG_MSG(3, ("handshake state: %d (%s) -> %d (%s)",
                              ssl->state, mbedtls_ssl_states_str((mbedtls_ssl_states) ssl->state),
                              (int) state, mbedtls_ssl_states_str(state)));
    ssl->state = (int) state;
}

static inline void mbedtls_ssl_handshake_increment_state(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_handshake_set_state(ssl, (mbedtls_ssl_states) (ssl->state + 1));
}

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_send_fatal_handshake_failure(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_reset_checksum(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_derive_keys(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handle_message_type(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_prepare_handshake_record(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_update_handshake_status(mbedtls_ssl_context *ssl);

/**
 * \brief       Update record layer
 *
 *              This function roughly separates the implementation
 *              of the logic of (D)TLS from the implementation
 *              of the secure transport.
 *
 * \param  ssl              The SSL context to use.
 * \param  update_hs_digest This indicates if the handshake digest
 *                          should be automatically updated in case
 *                          a handshake message is found.
 *
 * \return      0 or non-zero error code.
 *
 * \note        A clarification on what is called 'record layer' here
 *              is in order, as many sensible definitions are possible:
 *
 *              The record layer takes as input an untrusted underlying
 *              transport (stream or datagram) and transforms it into
 *              a serially multiplexed, secure transport, which
 *              conceptually provides the following:
 *
 *              (1) Three datagram based, content-agnostic transports
 *                  for handshake, alert and CCS messages.
 *              (2) One stream- or datagram-based transport
 *                  for application data.
 *              (3) Functionality for changing the underlying transform
 *                  securing the contents.
 *
 *              The interface to this functionality is given as follows:
 *
 *              a Updating
 *                [Currently implemented by mbedtls_ssl_read_record]
 *
 *                Check if and on which of the four 'ports' data is pending:
 *                Nothing, a controlling datagram of type (1), or application
 *                data (2). In any case data is present, internal buffers
 *                provide access to the data for the user to process it.
 *                Consumption of type (1) datagrams is done automatically
 *                on the next update, invalidating that the internal buffers
 *                for previous datagrams, while consumption of application
 *                data (2) is user-controlled.
 *
 *              b Reading of application data
 *                [Currently manual adaption of ssl->in_offt pointer]
 *
 *                As mentioned in the last paragraph, consumption of data
 *                is different from the automatic consumption of control
 *                datagrams (1) because application data is treated as a stream.
 *
 *              c Tracking availability of application data
 *                [Currently manually through decreasing ssl->in_msglen]
 *
 *                For efficiency and to retain datagram semantics for
 *                application data in case of DTLS, the record layer
 *                provides functionality for checking how much application
 *                data is still available in the internal buffer.
 *
 *              d Changing the transformation securing the communication.
 *
 *              Given an opaque implementation of the record layer in the
 *              above sense, it should be possible to implement the logic
 *              of (D)TLS on top of it without the need to know anything
 *              about the record layer's internals. This is done e.g.
 *              in all the handshake handling functions, and in the
 *              application data reading function mbedtls_ssl_read.
 *
 * \note        The above tries to give a conceptual picture of the
 *              record layer, but the current implementation deviates
 *              from it in some places. For example, our implementation of
 *              the update functionality through mbedtls_ssl_read_record
 *              discards datagrams depending on the current state, which
 *              wouldn't fall under the record layer's responsibility
 *              following the above definition.
 *
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_read_record(mbedtls_ssl_context *ssl,
                            unsigned update_hs_digest);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_fetch_input(mbedtls_ssl_context *ssl, size_t nb_want);

/*
 * Write handshake message header
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_start_handshake_msg(mbedtls_ssl_context *ssl, unsigned char hs_type,
                                    unsigned char **buf, size_t *buf_len);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_handshake_msg_ext(mbedtls_ssl_context *ssl,
                                        int update_checksum,
                                        int force_flush);
static inline int mbedtls_ssl_write_handshake_msg(mbedtls_ssl_context *ssl)
{
    return mbedtls_ssl_write_handshake_msg_ext(ssl, 1 /* update checksum */, 1 /* force flush */);
}

/*
 * Write handshake message tail
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_finish_handshake_msg(mbedtls_ssl_context *ssl,
                                     size_t buf_len, size_t msg_len);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_record(mbedtls_ssl_context *ssl, int force_flush);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_flush_output(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_certificate(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_certificate(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_change_cipher_spec(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_change_cipher_spec(mbedtls_ssl_context *ssl);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_finished(mbedtls_ssl_context *ssl);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_finished(mbedtls_ssl_context *ssl);

void mbedtls_ssl_optimize_checksum(mbedtls_ssl_context *ssl,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info);

/*
 * Update checksum of handshake messages.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_add_hs_msg_to_checksum(mbedtls_ssl_context *ssl,
                                       unsigned hs_type,
                                       unsigned char const *msg,
                                       size_t msg_len);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_add_hs_hdr_to_checksum(mbedtls_ssl_context *ssl,
                                       unsigned hs_type,
                                       size_t total_hs_len);

unsigned char mbedtls_ssl_sig_from_pk(mbedtls_pk_context *pk);
unsigned char mbedtls_ssl_sig_from_pk_alg(mbedtls_pk_type_t type);
mbedtls_pk_type_t mbedtls_ssl_pk_alg_from_sig(unsigned char sig);

mbedtls_md_type_t mbedtls_ssl_md_alg_from_hash(unsigned char hash);
unsigned char mbedtls_ssl_hash_from_md_alg(int md);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_set_calc_verify_md(mbedtls_ssl_context *ssl, int md);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_curve_tls_id(const mbedtls_ssl_context *ssl, uint16_t tls_id);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_curve(const mbedtls_ssl_context *ssl, mbedtls_ecp_group_id grp_id);

/**
 * \brief Return \c mbedtls_ecp_group_id for the specified TLS ID.
 *
 * \param tls_id    The TLS ID to look for
 * \return          Proper \c mbedtls_ecp_group_id if the TLS ID is supported,
 *                  or MBEDTLS_ECP_DP_NONE otherwise
 */
mbedtls_ecp_group_id mbedtls_ssl_get_ecp_group_id_from_tls_id(uint16_t tls_id);

/**
 * \brief Return TLS ID for the specified \c mbedtls_ecp_group_id.
 *
 * \param grp_id    The \c mbedtls_ecp_group_id ID to look for
 * \return          Proper TLS ID if the \c mbedtls_ecp_group_id is supported,
 *                  or 0 otherwise
 */
uint16_t mbedtls_ssl_get_tls_id_from_ecp_group_id(mbedtls_ecp_group_id grp_id);

static inline mbedtls_pk_context *mbedtls_ssl_own_key(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_key_cert *key_cert;

    if (ssl->handshake != NULL && ssl->handshake->key_cert != NULL) {
        key_cert = ssl->handshake->key_cert;
    } else {
        key_cert = ssl->conf->key_cert;
    }

    return key_cert == NULL ? NULL : key_cert->key;
}

static inline mbedtls_x509_crt *mbedtls_ssl_own_cert(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_key_cert *key_cert;

    if (ssl->handshake != NULL && ssl->handshake->key_cert != NULL) {
        key_cert = ssl->handshake->key_cert;
    } else {
        key_cert = ssl->conf->key_cert;
    }

    return key_cert == NULL ? NULL : key_cert->cert;
}

/*
 * Verify a certificate.
 *
 * [in/out] ssl: misc. things read
 *               ssl->session_negotiate->verify_result updated
 * [in] authmode: one of MBEDTLS_SSL_VERIFY_{NONE,OPTIONAL,REQUIRED}
 * [in] chain: the certificate chain to verify (ie the peer's chain)
 * [in] ciphersuite_info: For TLS 1.2, this session's ciphersuite;
 *                        for TLS 1.3, may be left NULL.
 * [in] rs_ctx: restart context if restartable ECC is in use;
 *              leave NULL for no restartable behaviour.
 *
 * Return:
 * - 0 if the handshake should continue. Depending on the
 *   authmode it means:
 *   - REQUIRED: the certificate was found to be valid, trusted & acceptable.
 *     ssl->session_negotiate->verify_result is 0.
 *   - OPTIONAL: the certificate may or may not be acceptable, but
 *     ssl->session_negotiate->verify_result was updated with the result.
 *   - NONE: the certificate wasn't even checked.
 * - MBEDTLS_ERR_X509_CERT_VERIFY_FAILED or MBEDTLS_ERR_SSL_BAD_CERTIFICATE if
 *   the certificate was found to be invalid/untrusted/unacceptable and the
 *   handshake should be aborted (can only happen with REQUIRED).
 * - another error code if another error happened (out-of-memory, etc.)
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_verify_certificate(mbedtls_ssl_context *ssl,
                                   int authmode,
                                   mbedtls_x509_crt *chain,
                                   const mbedtls_ssl_ciphersuite_t *ciphersuite_info,
                                   void *rs_ctx);

/*
 * Check usage of a certificate wrt usage extensions:
 * keyUsage and extendedKeyUsage.
 * (Note: nSCertType is deprecated and not standard, we don't check it.)
 *
 * Note: if tls_version is 1.3, ciphersuite is ignored and can be NULL.
 *
 * Note: recv_endpoint is the receiver's endpoint.
 *
 * Return 0 if everything is OK, -1 if not.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_cert_usage(const mbedtls_x509_crt *cert,
                                 const mbedtls_ssl_ciphersuite_t *ciphersuite,
                                 int recv_endpoint,
                                 mbedtls_ssl_protocol_version tls_version,
                                 uint32_t *flags);

void mbedtls_ssl_write_version(unsigned char version[2], int transport,
                               mbedtls_ssl_protocol_version tls_version);
uint16_t mbedtls_ssl_read_version(const unsigned char version[2],
                                  int transport);

static inline size_t mbedtls_ssl_in_hdr_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);

    {
        return 5;
    }
}

static inline size_t mbedtls_ssl_out_hdr_len(const mbedtls_ssl_context *ssl)
{
    return (size_t) (ssl->out_iv - ssl->out_hdr);
}

static inline size_t mbedtls_ssl_hs_hdr_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);
    return 4;
}

/* Visible for testing purposes only */

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_session_copy(mbedtls_ssl_session *dst,
                             const mbedtls_ssl_session *src);

/* The hash buffer must have at least MBEDTLS_MD_MAX_SIZE bytes of length. */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_get_key_exchange_md_tls1_2(mbedtls_ssl_context *ssl,
                                           unsigned char *hash, size_t *hashlen,
                                           unsigned char *data, size_t data_len,
                                           mbedtls_md_type_t md_alg);

#ifdef __cplusplus
}
#endif

void mbedtls_ssl_transform_init(mbedtls_ssl_transform *transform);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_encrypt_buf(mbedtls_ssl_context *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_decrypt_buf(mbedtls_ssl_context const *ssl,
                            mbedtls_ssl_transform *transform,
                            mbedtls_record *rec);

/* Length of the "epoch" field in the record header */
static inline size_t mbedtls_ssl_ep_len(const mbedtls_ssl_context *ssl)
{
    ((void) ssl);
    return 0;
}

void mbedtls_ssl_set_timer(mbedtls_ssl_context *ssl, uint32_t millisecs);
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_check_timer(mbedtls_ssl_context *ssl);

void mbedtls_ssl_reset_in_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_update_in_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_reset_out_pointers(mbedtls_ssl_context *ssl);
void mbedtls_ssl_update_out_pointers(mbedtls_ssl_context *ssl,
                                     mbedtls_ssl_transform *transform);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_session_reset_int(mbedtls_ssl_context *ssl, int partial);
void mbedtls_ssl_session_reset_msg_layer(mbedtls_ssl_context *ssl,
                                         int partial);

/*
 * Send pending alert
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_handle_pending_alert(mbedtls_ssl_context *ssl);

/*
 * Set pending fatal alert flag.
 */
void mbedtls_ssl_pend_fatal_alert(mbedtls_ssl_context *ssl,
                                  unsigned char alert_type,
                                  int alert_reason);

/* Alias of mbedtls_ssl_pend_fatal_alert */
#define MBEDTLS_SSL_PEND_FATAL_ALERT(type, user_return_value)         \
    mbedtls_ssl_pend_fatal_alert(ssl, type, user_return_value)

void mbedtls_ssl_handshake_wrapup_free_hs_transform(mbedtls_ssl_context *ssl);

/**
 * ssl utils functions for checking configuration.
 */

static inline int mbedtls_ssl_conf_is_tls12_only(const mbedtls_ssl_config *conf)
{
    return conf->min_tls_version == MBEDTLS_SSL_VERSION_TLS1_2 &&
           conf->max_tls_version == MBEDTLS_SSL_VERSION_TLS1_2;
}

static inline int mbedtls_ssl_conf_is_tls13_enabled(const mbedtls_ssl_config *conf)
{
    ((void) conf);
    return 0;
}

static inline int mbedtls_ssl_conf_is_tls12_enabled(const mbedtls_ssl_config *conf)
{
    return conf->min_tls_version <= MBEDTLS_SSL_VERSION_TLS1_2 &&
           conf->max_tls_version >= MBEDTLS_SSL_VERSION_TLS1_2;
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
/*
 * Write Signature Algorithm extension
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_sig_alg_ext(mbedtls_ssl_context *ssl, unsigned char *buf,
                                  const unsigned char *end, size_t *out_len);
/*
 * Parse TLS Signature Algorithm extension
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_sig_alg_ext(mbedtls_ssl_context *ssl,
                                  const unsigned char *buf,
                                  const unsigned char *end);
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/* Get handshake transcript */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_get_handshake_transcript(mbedtls_ssl_context *ssl,
                                         const mbedtls_md_type_t md,
                                         unsigned char *dst,
                                         size_t dst_len,
                                         size_t *olen);

/*
 * Return supported groups.
 *
 * In future, invocations can be changed to ssl->conf->group_list
 * when mbedtls_ssl_conf_curves() is deleted.
 *
 * ssl->handshake->group_list is either a translation of curve_list to IANA TLS group
 * identifiers when mbedtls_ssl_conf_curves() has been used, or a pointer to
 * ssl->conf->group_list when mbedtls_ssl_conf_groups() has been more recently invoked.
 *
 */
static inline const void *mbedtls_ssl_get_groups(const mbedtls_ssl_context *ssl)
{
    #if defined(MBEDTLS_DEPRECATED_REMOVED) || !defined(MBEDTLS_ECP_C)
    return ssl->conf->group_list;
#else
    if ((ssl->handshake != NULL) && (ssl->handshake->group_list != NULL)) {
        return ssl->handshake->group_list;
    } else {
        return ssl->conf->group_list;
    }
    #endif
}

/*
 * Helper functions for NamedGroup.
 */
static inline int mbedtls_ssl_tls12_named_group_is_ecdhe(uint16_t named_group)
{
    /*
     * RFC 8422 section 5.1.1
     */
    return named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519    ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP256R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP384R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_BP512R1   ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X448      ||
           /* Below deprecated curves should be removed with notice to users */
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP192K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP192R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP224K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP224R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256K1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1;
}

static inline int mbedtls_ssl_tls13_named_group_is_ecdhe(uint16_t named_group)
{
    return named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X25519    ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1 ||
           named_group == MBEDTLS_SSL_IANA_TLS_GROUP_X448;
}

static inline int mbedtls_ssl_tls13_named_group_is_ffdh(uint16_t named_group)
{
    return named_group >= MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE2048 &&
           named_group <= MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE8192;
}

static inline int mbedtls_ssl_named_group_is_offered(
    const mbedtls_ssl_context *ssl, uint16_t named_group)
{
    const uint16_t *group_list = mbedtls_ssl_get_groups(ssl);

    if (group_list == NULL) {
        return 0;
    }

    for (; *group_list != 0; group_list++) {
        if (*group_list == named_group) {
            return 1;
        }
    }

    return 0;
}

static inline int mbedtls_ssl_named_group_is_supported(uint16_t named_group)
{
#if defined(PSA_WANT_ALG_ECDH)
    if (mbedtls_ssl_tls13_named_group_is_ecdhe(named_group)) {
        if (mbedtls_ssl_get_ecp_group_id_from_tls_id(named_group) !=
            MBEDTLS_ECP_DP_NONE) {
            return 1;
        }
    }
#endif
#if defined(PSA_WANT_ALG_FFDH)
    if (mbedtls_ssl_tls13_named_group_is_ffdh(named_group)) {
        return 1;
    }
#endif
#if !defined(PSA_WANT_ALG_ECDH) && !defined(PSA_WANT_ALG_FFDH)
    (void) named_group;
#endif
    return 0;
}

/*
 * Return supported signature algorithms.
 *
 * In future, invocations can be changed to ssl->conf->sig_algs when
 * mbedtls_ssl_conf_sig_hashes() is deleted.
 *
 * ssl->handshake->sig_algs is either a translation of sig_hashes to IANA TLS
 * signature algorithm identifiers when mbedtls_ssl_conf_sig_hashes() has been
 * used, or a pointer to ssl->conf->sig_algs when mbedtls_ssl_conf_sig_algs() has
 * been more recently invoked.
 *
 */
static inline const void *mbedtls_ssl_get_sig_algs(
    const mbedtls_ssl_context *ssl)
{
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    if (ssl->handshake != NULL &&
        ssl->handshake->sig_algs_heap_allocated == 1 &&
        ssl->handshake->sig_algs != NULL) {
        return ssl->handshake->sig_algs;
    }
#endif
    return ssl->conf->sig_algs;

#else

    ((void) ssl);
    return NULL;
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */
}

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
static inline int mbedtls_ssl_sig_alg_is_offered(const mbedtls_ssl_context *ssl,
                                                 uint16_t proposed_sig_alg)
{
    const uint16_t *sig_alg = mbedtls_ssl_get_sig_algs(ssl);
    if (sig_alg == NULL) {
        return 0;
    }

    for (; *sig_alg != MBEDTLS_TLS_SIG_NONE; sig_alg++) {
        if (*sig_alg == proposed_sig_alg) {
            return 1;
        }
    }
    return 0;
}

static inline int mbedtls_ssl_get_pk_type_and_md_alg_from_sig_alg(
    uint16_t sig_alg, mbedtls_pk_type_t *pk_type, mbedtls_md_type_t *md_alg)
{
    *pk_type = mbedtls_ssl_pk_alg_from_sig(sig_alg & 0xff);
    *md_alg = mbedtls_ssl_md_alg_from_hash((sig_alg >> 8) & 0xff);

    if (*pk_type != MBEDTLS_PK_NONE && *md_alg != MBEDTLS_MD_NONE) {
        return 0;
    }

    switch (sig_alg) {
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256:
            *md_alg = MBEDTLS_MD_SHA256;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384:
            *md_alg = MBEDTLS_MD_SHA384;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        case MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512:
            *md_alg = MBEDTLS_MD_SHA512;
            *pk_type = MBEDTLS_PK_RSASSA_PSS;
            break;
        default:
            return MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
    }
    return 0;
}

static inline int mbedtls_ssl_tls12_sig_alg_is_supported(
    const uint16_t sig_alg)
{
    /* High byte is hash */
    unsigned char hash = MBEDTLS_BYTE_1(sig_alg);
    unsigned char sig = MBEDTLS_BYTE_0(sig_alg);

    switch (hash) {

        case MBEDTLS_SSL_HASH_SHA1:
            break;

        case MBEDTLS_SSL_HASH_SHA256:
            break;

        case MBEDTLS_SSL_HASH_SHA384:
            break;

        case MBEDTLS_SSL_HASH_SHA512:
            break;

        default:
            return 0;
    }

    switch (sig) {
        case MBEDTLS_SSL_SIG_RSA:
            break;

#if defined(MBEDTLS_KEY_EXCHANGE_ECDSA_CERT_REQ_ALLOWED_ENABLED)
        case MBEDTLS_SSL_SIG_ECDSA:
            break;
#endif

        default:
            return 0;
    }

    return 1;
}

static inline int mbedtls_ssl_sig_alg_is_supported(
    const mbedtls_ssl_context *ssl,
    const uint16_t sig_alg)
{

    if (ssl->tls_version == MBEDTLS_SSL_VERSION_TLS1_2) {
        return mbedtls_ssl_tls12_sig_alg_is_supported(sig_alg);
    }

    ((void) ssl);
    ((void) sig_alg);
    return 0;
}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/**
 * \brief       TLS record protection modes
 */
typedef enum {
    MBEDTLS_SSL_MODE_STREAM = 0,
    MBEDTLS_SSL_MODE_CBC,
    MBEDTLS_SSL_MODE_CBC_ETM,
    MBEDTLS_SSL_MODE_AEAD
} mbedtls_ssl_mode_t;

mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_transform(
    const mbedtls_ssl_transform *transform);

#if defined(MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM)
mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_ciphersuite(
    int encrypt_then_mac,
    const mbedtls_ssl_ciphersuite_t *suite);
#else
mbedtls_ssl_mode_t mbedtls_ssl_get_mode_from_ciphersuite(
    const mbedtls_ssl_ciphersuite_t *suite);
#endif /* MBEDTLS_SSL_SOME_SUITES_USE_CBC_ETM */

#if defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_tls13_read_public_xxdhe_share(mbedtls_ssl_context *ssl,
                                              const unsigned char *buf,
                                              size_t buf_len);

#endif /* PSA_WANT_ALG_ECDH || PSA_WANT_ALG_FFDH */

static inline int mbedtls_ssl_tls13_cipher_suite_is_offered(
    mbedtls_ssl_context *ssl, int cipher_suite)
{
    const int *ciphersuite_list = ssl->conf->ciphersuite_list;

    /* Check whether we have offered this ciphersuite */
    for (size_t i = 0; ciphersuite_list[i] != 0; i++) {
        if (ciphersuite_list[i] == cipher_suite) {
            return 1;
        }
    }
    return 0;
}

/**
 * \brief Validate cipher suite against config in SSL context.
 *
 * \param ssl              SSL context
 * \param suite_info       Cipher suite to validate
 * \param min_tls_version  Minimal TLS version to accept a cipher suite
 * \param max_tls_version  Maximal TLS version to accept a cipher suite
 *
 * \return 0 if valid, negative value otherwise.
 */
MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_validate_ciphersuite(
    const mbedtls_ssl_context *ssl,
    const mbedtls_ssl_ciphersuite_t *suite_info,
    mbedtls_ssl_protocol_version min_tls_version,
    mbedtls_ssl_protocol_version max_tls_version);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_server_name_ext(mbedtls_ssl_context *ssl,
                                      const unsigned char *buf,
                                      const unsigned char *end);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_parse_alpn_ext(mbedtls_ssl_context *ssl,
                               const unsigned char *buf,
                               const unsigned char *end);

MBEDTLS_CHECK_RETURN_CRITICAL
int mbedtls_ssl_write_alpn_ext(mbedtls_ssl_context *ssl,
                               unsigned char *buf,
                               unsigned char *end,
                               size_t *out_len);

/** Get the host name from the SSL context.
 *
 * \param[in]   ssl     SSL context
 *
 * \return The \p hostname pointer from the SSL context.
 *         \c NULL if mbedtls_ssl_set_hostname() has never been called on
 *         \p ssl or if it was last called with \p NULL.
 */
const char *mbedtls_ssl_get_hostname_pointer(const mbedtls_ssl_context *ssl);

#endif /* ssl_misc.h */
