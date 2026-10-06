// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file ssl.h
 *
 * \brief SSL/TLS functions.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/ssl.h.
 */
#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H
#include "platform_util.h"
#include "private_access.h"

#include "build_info.h"

#include "bignum.h"
#include "ecp.h"

#include "ssl_ciphersuites.h"

#include "x509_crt.h"
#include "x509_crl.h"

#include "md.h"

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_ECDH_OR_ECDHE_ANY_ENABLED)
#include "ecdh.h"
#endif

#include "platform_time.h"

/*
 * SSL Error codes
 */
/** A cryptographic operation is in progress. Try again later. */
#define MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS                -0x7000
/** The requested feature is not available. */
#define MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE               -0x7080
/** Bad input parameters to function. */
#define MBEDTLS_ERR_SSL_BAD_INPUT_DATA                    -0x7100
/** Verification of the message MAC failed. */
#define MBEDTLS_ERR_SSL_INVALID_MAC                       -0x7180
/** An invalid SSL record was received. */
#define MBEDTLS_ERR_SSL_INVALID_RECORD                    -0x7200
/** The connection indicated an EOF. */
#define MBEDTLS_ERR_SSL_CONN_EOF                          -0x7280
/** A message could not be parsed due to a syntactic error. */
#define MBEDTLS_ERR_SSL_DECODE_ERROR                      -0x7300
/* Error space gap */
/** No RNG was provided to the SSL module. */
#define MBEDTLS_ERR_SSL_NO_RNG                            -0x7400
/** No client certification received from the client, but required by the authentication mode. */
#define MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE             -0x7480
/** Client received an extended server hello containing an unsupported extension */
#define MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION             -0x7500
/** No ALPN protocols supported that the client advertises */
#define MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL           -0x7580
/** The own private key or pre-shared key is not set, but needed. */
#define MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED              -0x7600
/** No CA Chain is set, but required to operate. */
#define MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED                 -0x7680
/** An unexpected message was received from our peer. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE                -0x7700
/** A fatal alert message was received from our peer. */
#define MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE               -0x7780
/** No server could be identified matching the client's SNI. */
#define MBEDTLS_ERR_SSL_UNRECOGNIZED_NAME                 -0x7800
/** The peer notified us that the connection is going to be closed. */
#define MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY                 -0x7880
/* Error space gap */
/* Error space gap */
/** Processing of the Certificate handshake message failed. */
#define MBEDTLS_ERR_SSL_BAD_CERTIFICATE                   -0x7A00
/* Error space gap */
/** A TLS 1.3 NewSessionTicket message has been received. */
#define MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET       -0x7B00
/** Not possible to read early data */
#define MBEDTLS_ERR_SSL_CANNOT_READ_EARLY_DATA            -0x7B80
/**
 * Early data has been received as part of an on-going handshake.
 * This error code can be returned only on server side if and only if early
 * data has been enabled by means of the mbedtls_ssl_conf_early_data() API.
 * This error code can then be returned by mbedtls_ssl_handshake(),
 * mbedtls_ssl_handshake_step(), mbedtls_ssl_read() or mbedtls_ssl_write() if
 * early data has been received as part of the handshake sequence they
 * triggered. To read the early data, call mbedtls_ssl_read_early_data().
 */
#define MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA               -0x7C00
/** Not possible to write early data */
#define MBEDTLS_ERR_SSL_CANNOT_WRITE_EARLY_DATA           -0x7C80
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/** Cache entry not found */
#define MBEDTLS_ERR_SSL_CACHE_ENTRY_NOT_FOUND             -0x7E80
/** Memory allocation failed */
#define MBEDTLS_ERR_SSL_ALLOC_FAILED                      -0x7F00
/** Hardware acceleration function returned with error */
#define MBEDTLS_ERR_SSL_HW_ACCEL_FAILED                   -0x7F80
/** Hardware acceleration function skipped / left alone data */
#define MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH              -0x6F80
/** Handshake protocol not within min/max boundaries */
#define MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION              -0x6E80
/** The handshake negotiation failed. */
#define MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE                 -0x6E00
/** Session ticket has expired. */
#define MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED            -0x6D80
/** Public key type mismatch (eg, asked for RSA key exchange and presented EC key) */
#define MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH                  -0x6D00
/** Unknown identity received (eg, PSK identity) */
#define MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY                  -0x6C80
/** Internal error (eg, unexpected failure in lower-level module) */
#define MBEDTLS_ERR_SSL_INTERNAL_ERROR                    -0x6C00
/** A counter would wrap (eg, too many messages exchanged). */
#define MBEDTLS_ERR_SSL_COUNTER_WRAPPING                  -0x6B80
/** Unexpected message at ServerHello in renegotiation. */
#define MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO       -0x6B00
/** DTLS client must retry for hello verification */
#define MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED             -0x6A80
/** A buffer is too small to receive or write a message */
#define MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL                  -0x6A00
/* Error space gap */
/** No data of requested type currently available on underlying transport. */
#define MBEDTLS_ERR_SSL_WANT_READ                         -0x6900
/** Connection requires a write call. */
#define MBEDTLS_ERR_SSL_WANT_WRITE                        -0x6880
/** The operation timed out. */
#define MBEDTLS_ERR_SSL_TIMEOUT                           -0x6800
/** The client initiated a reconnect from the same port. */
#define MBEDTLS_ERR_SSL_CLIENT_RECONNECT                  -0x6780
/** Record header looks valid but is not expected. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_RECORD                 -0x6700
/** The alert message received indicates a non-fatal error. */
#define MBEDTLS_ERR_SSL_NON_FATAL                         -0x6680
/** A field in a message was incorrect or inconsistent with other fields. */
#define MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER                 -0x6600
/** Internal-only message signaling that further message-processing should be done */
#define MBEDTLS_ERR_SSL_CONTINUE_PROCESSING               -0x6580
/** The asynchronous operation is not completed yet. */
#define MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS                 -0x6500
/** Internal-only message signaling that a message arrived early. */
#define MBEDTLS_ERR_SSL_EARLY_MESSAGE                     -0x6480
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/* Error space gap */
/** An encrypted DTLS-frame with an unexpected CID was received. */
#define MBEDTLS_ERR_SSL_UNEXPECTED_CID                    -0x6000
/** An operation failed due to an unexpected version or configuration. */
#define MBEDTLS_ERR_SSL_VERSION_MISMATCH                  -0x5F00
/** Invalid value in SSL config */
#define MBEDTLS_ERR_SSL_BAD_CONFIG                        -0x5E80
/* Error space gap */
/** Attempt to verify a certificate without an expected hostname.
 * This is usually insecure.
 *
 * In TLS clients, when a client authenticates a server through its
 * certificate, the client normally checks three things:
 * - the certificate chain must be valid;
 * - the chain must start from a trusted CA;
 * - the certificate must cover the server name that is expected by the client.
 *
 * Omitting any of these checks is generally insecure, and can allow a
 * malicious server to impersonate a legitimate server.
 *
 * The third check may be safely skipped in some unusual scenarios,
 * such as networks where eavesdropping is a risk but not active attacks,
 * or a private PKI where the client equally trusts all servers that are
 * accredited by the root CA.
 *
 * You should call mbedtls_ssl_set_hostname() with the expected server name
 * before starting a TLS handshake on a client (unless the client is
 * set up to only use PSK-based authentication, which does not rely on the
 * host name). If you have determined that server name verification is not
 * required for security in your scenario, call mbedtls_ssl_set_hostname()
 * with \p NULL as the server name.
 *
 * This error is raised if all of the following conditions are met:
 *
 * - A TLS client is configured with the authentication mode
 *   #MBEDTLS_SSL_VERIFY_REQUIRED (default).
 * - Certificate authentication is enabled.
 * - The client does not call mbedtls_ssl_set_hostname().
 * - The configuration option
 *   #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *   is not enabled.
 */
#define MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME  -0x5D80

/*
 * Constants from RFC 8446 for TLS 1.3 PSK modes
 *
 * Those are used in the Pre-Shared Key Exchange Modes extension.
 * See Section 4.2.9 in RFC 8446.
 */
#define MBEDTLS_SSL_TLS1_3_PSK_MODE_PURE  0 /* Pure PSK-based exchange  */
#define MBEDTLS_SSL_TLS1_3_PSK_MODE_ECDHE 1 /* PSK+ECDHE-based exchange */

/*
 * TLS 1.3 NamedGroup values
 *
 * From RF 8446
 *    enum {
 *         // Elliptic Curve Groups (ECDHE)
 *         secp256r1(0x0017), secp384r1(0x0018), secp521r1(0x0019),
 *         x25519(0x001D), x448(0x001E),
 *         // Finite Field Groups (DHE)
 *         ffdhe2048(0x0100), ffdhe3072(0x0101), ffdhe4096(0x0102),
 *         ffdhe6144(0x0103), ffdhe8192(0x0104),
 *         // Reserved Code Points
 *         ffdhe_private_use(0x01FC..0x01FF),
 *         ecdhe_private_use(0xFE00..0xFEFF),
 *         (0xFFFF)
 *     } NamedGroup;
 *
 */

/* Elliptic Curve Groups (ECDHE) */
#define MBEDTLS_SSL_IANA_TLS_GROUP_NONE               0
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP192K1     0x0012
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP192R1     0x0013
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP224K1     0x0014
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP224R1     0x0015
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP256K1     0x0016
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1     0x0017
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1     0x0018
#define MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1     0x0019
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP256R1       0x001A
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP384R1       0x001B
#define MBEDTLS_SSL_IANA_TLS_GROUP_BP512R1       0x001C
#define MBEDTLS_SSL_IANA_TLS_GROUP_X25519        0x001D
#define MBEDTLS_SSL_IANA_TLS_GROUP_X448          0x001E
/* Finite Field Groups (DHE) */
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE2048     0x0100
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE3072     0x0101
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE4096     0x0102
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE6144     0x0103
#define MBEDTLS_SSL_IANA_TLS_GROUP_FFDHE8192     0x0104

/*
 * TLS 1.3 Key Exchange Modes
 *
 * Mbed TLS internal identifiers for use with the SSL configuration API
 * mbedtls_ssl_conf_tls13_key_exchange_modes().
 */

#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK            (1u << 0)   /*!< Pure-PSK TLS 1.3 key exchange,
                                                                         *   encompassing both externally agreed PSKs
                                                                         *   as well as resumption PSKs. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL      (1u << 1)   /*!< Pure-Ephemeral TLS 1.3 key exchanges,
                                                                         *   including for example ECDHE and DHE
                                                                         *   key exchanges. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL  (1u << 2)   /*!< PSK-Ephemeral TLS 1.3 key exchanges,
                                                                         *   using both a PSK and an ephemeral
                                                                         *   key exchange. */

/* Convenience macros for sets of key exchanges. */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_ALL                         \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK              |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL    |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL)          /*!< All TLS 1.3 key exchanges           */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ALL                     \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK              |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL)      /*!< All PSK-based TLS 1.3 key exchanges */
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ALL               \
    (MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL        |            \
     MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL)      /*!< All ephemeral TLS 1.3 key exchanges */

#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_NONE   (0)

/*
 * Various constants
 */

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/* These are the high and low bytes of ProtocolVersion as defined by:
 * - RFC 5246: ProtocolVersion version = { 3, 3 };     // TLS v1.2
 * - RFC 8446: see section 4.2.1
 */
#define MBEDTLS_SSL_MAJOR_VERSION_3             3
#define MBEDTLS_SSL_MINOR_VERSION_3             3   /*!< TLS v1.2 */
#define MBEDTLS_SSL_MINOR_VERSION_4             4   /*!< TLS v1.3 */
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#define MBEDTLS_SSL_TRANSPORT_STREAM            0   /*!< TLS      */
#define MBEDTLS_SSL_TRANSPORT_DATAGRAM          1   /*!< DTLS     */

#define MBEDTLS_SSL_MAX_HOST_NAME_LEN           255 /*!< Maximum host name defined in RFC 1035 */
#define MBEDTLS_SSL_MAX_ALPN_NAME_LEN           255 /*!< Maximum size in bytes of a protocol name in alpn ext., RFC 7301 */

#define MBEDTLS_SSL_MAX_ALPN_LIST_LEN           65535 /*!< Maximum size in bytes of list in alpn ext., RFC 7301          */

/* RFC 6066 section 4, see also mfl_code_to_length in ssl_tls.c
 * NONE must be zero so that memset()ing structure to zero works */
#define MBEDTLS_SSL_MAX_FRAG_LEN_NONE           0   /*!< don't use this extension   */
#define MBEDTLS_SSL_MAX_FRAG_LEN_512            1   /*!< MaxFragmentLength 2^9      */
#define MBEDTLS_SSL_MAX_FRAG_LEN_1024           2   /*!< MaxFragmentLength 2^10     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_2048           3   /*!< MaxFragmentLength 2^11     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_4096           4   /*!< MaxFragmentLength 2^12     */
#define MBEDTLS_SSL_MAX_FRAG_LEN_INVALID        5   /*!< first invalid value        */

#define MBEDTLS_SSL_IS_CLIENT                   0
#define MBEDTLS_SSL_IS_SERVER                   1

#define MBEDTLS_SSL_EXTENDED_MS_DISABLED        0
#define MBEDTLS_SSL_EXTENDED_MS_ENABLED         1

#define MBEDTLS_SSL_CID_DISABLED                0
#define MBEDTLS_SSL_CID_ENABLED                 1

#define MBEDTLS_SSL_ETM_DISABLED                0
#define MBEDTLS_SSL_ETM_ENABLED                 1

#define MBEDTLS_SSL_COMPRESS_NULL               0

#define MBEDTLS_SSL_VERIFY_NONE                 0
#define MBEDTLS_SSL_VERIFY_OPTIONAL             1
#define MBEDTLS_SSL_VERIFY_REQUIRED             2
#define MBEDTLS_SSL_VERIFY_UNSET                3 /* Used only for sni_authmode */

#define MBEDTLS_SSL_LEGACY_RENEGOTIATION        0
#define MBEDTLS_SSL_SECURE_RENEGOTIATION        1

#define MBEDTLS_SSL_RENEGOTIATION_DISABLED      0
#define MBEDTLS_SSL_RENEGOTIATION_ENABLED       1

#define MBEDTLS_SSL_ANTI_REPLAY_DISABLED        0
#define MBEDTLS_SSL_ANTI_REPLAY_ENABLED         1

#define MBEDTLS_SSL_RENEGOTIATION_NOT_ENFORCED  -1
#define MBEDTLS_SSL_RENEGO_MAX_RECORDS_DEFAULT  16

#define MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION     0
#define MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION  1
#define MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE      2

#define MBEDTLS_SSL_TRUNC_HMAC_DISABLED         0
#define MBEDTLS_SSL_TRUNC_HMAC_ENABLED          1
#define MBEDTLS_SSL_TRUNCATED_HMAC_LEN          10  /* 80 bits, rfc 6066 section 7 */

#define MBEDTLS_SSL_SESSION_TICKETS_DISABLED     0
#define MBEDTLS_SSL_SESSION_TICKETS_ENABLED      1

#define MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_DISABLED  0
#define MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_ENABLED   1

#define MBEDTLS_SSL_PRESET_DEFAULT              0
#define MBEDTLS_SSL_PRESET_SUITEB               2

#define MBEDTLS_SSL_CERT_REQ_CA_LIST_ENABLED       1
#define MBEDTLS_SSL_CERT_REQ_CA_LIST_DISABLED      0

#define MBEDTLS_SSL_EARLY_DATA_DISABLED        0
#define MBEDTLS_SSL_EARLY_DATA_ENABLED         1

#define MBEDTLS_SSL_DTLS_SRTP_MKI_UNSUPPORTED    0
#define MBEDTLS_SSL_DTLS_SRTP_MKI_SUPPORTED      1

#define MBEDTLS_SSL_SRV_CIPHERSUITE_ORDER_CLIENT  1
#define MBEDTLS_SSL_SRV_CIPHERSUITE_ORDER_SERVER  0

/*
 * Default range for DTLS retransmission timer value, in milliseconds.
 * RFC 6347 4.2.4.1 says from 1 second to 60 seconds.
 */
#define MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MIN    1000
#define MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MAX   60000

/*
 * Whether early data record should be discarded or not and how.
 *
 * The client has indicated early data and the server has rejected them.
 * The server has then to skip past early data by either:
 * - attempting to deprotect received records using the handshake traffic
 *   key, discarding records which fail deprotection (up to the configured
 *   max_early_data_size). Once a record is deprotected successfully,
 *   it is treated as the start of the client's second flight and the
 *   server proceeds as with an ordinary 1-RTT handshake.
 * - skipping all records with an external content type of
 *   "application_data" (indicating that they are encrypted), up to the
 *   configured max_early_data_size. This is the expected behavior if the
 *   server has sent an HelloRetryRequest message. The server ignores
 *   application data message before 2nd ClientHello.
 */
#define MBEDTLS_SSL_EARLY_DATA_NO_DISCARD 0
#define MBEDTLS_SSL_EARLY_DATA_TRY_TO_DEPROTECT_AND_DISCARD 1
#define MBEDTLS_SSL_EARLY_DATA_DISCARD 2

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in mbedtls_config.h or define them on the compiler command line.
 * \{
 */

/*
 * Maximum fragment length in bytes,
 * determines the size of each of the two internal I/O buffers.
 *
 * Note: the RFC defines the default size of SSL / TLS messages. If you
 * change the value here, other clients / servers may not be able to
 * communicate with you anymore. Only change this value if you control
 * both sides of the connection and have it reduced at both sides, or
 * if you're using the Max Fragment Length extension and you know all your
 * peers are using it too!
 */

/*
 * Maximum number of heap-allocated bytes for the purpose of
 * DTLS handshake message reassembly and future message buffering.
 */
#if !defined(MBEDTLS_SSL_DTLS_MAX_BUFFERING)
#define MBEDTLS_SSL_DTLS_MAX_BUFFERING 32768
#endif

/*
 * Maximum length of CIDs for incoming and outgoing messages.
 */
#if !defined(MBEDTLS_SSL_CID_IN_LEN_MAX)
#define MBEDTLS_SSL_CID_IN_LEN_MAX          32
#endif

#if !defined(MBEDTLS_SSL_CID_OUT_LEN_MAX)
#define MBEDTLS_SSL_CID_OUT_LEN_MAX         32
#endif

#if !defined(MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY)
#define MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY 16
#endif

#if !defined(MBEDTLS_SSL_MAX_EARLY_DATA_SIZE)
#define MBEDTLS_SSL_MAX_EARLY_DATA_SIZE        1024
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_TICKET_AGE_TOLERANCE)
#define MBEDTLS_SSL_TLS1_3_TICKET_AGE_TOLERANCE 6000
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH)
#define MBEDTLS_SSL_TLS1_3_TICKET_NONCE_LENGTH 32
#endif

#if !defined(MBEDTLS_SSL_TLS1_3_DEFAULT_NEW_SESSION_TICKETS)
#define MBEDTLS_SSL_TLS1_3_DEFAULT_NEW_SESSION_TICKETS 1
#endif

/** \} name SECTION: Module settings */

/*
 * Default to standard CID mode
 */

/*
 * Length of the verify data for secure renegotiation
 */
#define MBEDTLS_SSL_VERIFY_DATA_MAX_LEN 12

/*
 * Signaling ciphersuite values (SCSV)
 */
#define MBEDTLS_SSL_EMPTY_RENEGOTIATION_INFO    0xFF   /**< renegotiation info ext */

/*
 * Supported Signature and Hash algorithms (For TLS 1.2)
 * RFC 5246 section 7.4.1.4.1
 */
#define MBEDTLS_SSL_HASH_NONE                0
#define MBEDTLS_SSL_HASH_MD5                 1
#define MBEDTLS_SSL_HASH_SHA1                2
#define MBEDTLS_SSL_HASH_SHA224              3
#define MBEDTLS_SSL_HASH_SHA256              4
#define MBEDTLS_SSL_HASH_SHA384              5
#define MBEDTLS_SSL_HASH_SHA512              6

#define MBEDTLS_SSL_SIG_ANON                 0
#define MBEDTLS_SSL_SIG_RSA                  1
#define MBEDTLS_SSL_SIG_ECDSA                3

/*
 * TLS 1.3 signature algorithms
 * RFC 8446, Section 4.2.3
 */

/* RSASSA-PKCS1-v1_5 algorithms */
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA256 0x0401
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA384 0x0501
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA512 0x0601

/* ECDSA algorithms */
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256 0x0403
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP384R1_SHA384 0x0503
#define MBEDTLS_TLS1_3_SIG_ECDSA_SECP521R1_SHA512 0x0603

/* RSASSA-PSS algorithms with public key OID rsaEncryption */
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA256 0x0804
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA384 0x0805
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_RSAE_SHA512 0x0806

/* EdDSA algorithms */
#define MBEDTLS_TLS1_3_SIG_ED25519 0x0807
#define MBEDTLS_TLS1_3_SIG_ED448 0x0808

/* RSASSA-PSS algorithms with public key OID RSASSA-PSS  */
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA256 0x0809
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA384 0x080A
#define MBEDTLS_TLS1_3_SIG_RSA_PSS_PSS_SHA512 0x080B

/* LEGACY ALGORITHMS */
#define MBEDTLS_TLS1_3_SIG_RSA_PKCS1_SHA1 0x0201
#define MBEDTLS_TLS1_3_SIG_ECDSA_SHA1     0x0203

#define MBEDTLS_TLS1_3_SIG_NONE 0x0

/*
 * Client Certificate Types
 * RFC 5246 section 7.4.4 plus RFC 4492 section 5.5
 */
#define MBEDTLS_SSL_CERT_TYPE_RSA_SIGN       1
#define MBEDTLS_SSL_CERT_TYPE_ECDSA_SIGN    64

/*
 * Message, alert and handshake types
 */
#define MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC     20
#define MBEDTLS_SSL_MSG_ALERT                  21
#define MBEDTLS_SSL_MSG_HANDSHAKE              22
#define MBEDTLS_SSL_MSG_APPLICATION_DATA       23
#define MBEDTLS_SSL_MSG_CID                    25

#define MBEDTLS_SSL_ALERT_LEVEL_WARNING         1
#define MBEDTLS_SSL_ALERT_LEVEL_FATAL           2

#define MBEDTLS_SSL_ALERT_MSG_CLOSE_NOTIFY           0  /* 0x00 */
#define MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE    10  /* 0x0A */
#define MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC        20  /* 0x14 */
#define MBEDTLS_SSL_ALERT_MSG_DECRYPTION_FAILED     21  /* 0x15 */
#define MBEDTLS_SSL_ALERT_MSG_RECORD_OVERFLOW       22  /* 0x16 */
#define MBEDTLS_SSL_ALERT_MSG_DECOMPRESSION_FAILURE 30  /* 0x1E */
#define MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE     40  /* 0x28 */
#define MBEDTLS_SSL_ALERT_MSG_NO_CERT               41  /* 0x29 */
#define MBEDTLS_SSL_ALERT_MSG_BAD_CERT              42  /* 0x2A */
#define MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_CERT      43  /* 0x2B */
#define MBEDTLS_SSL_ALERT_MSG_CERT_REVOKED          44  /* 0x2C */
#define MBEDTLS_SSL_ALERT_MSG_CERT_EXPIRED          45  /* 0x2D */
#define MBEDTLS_SSL_ALERT_MSG_CERT_UNKNOWN          46  /* 0x2E */
#define MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER     47  /* 0x2F */
#define MBEDTLS_SSL_ALERT_MSG_UNKNOWN_CA            48  /* 0x30 */
#define MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED         49  /* 0x31 */
#define MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR          50  /* 0x32 */
#define MBEDTLS_SSL_ALERT_MSG_DECRYPT_ERROR         51  /* 0x33 */
#define MBEDTLS_SSL_ALERT_MSG_EXPORT_RESTRICTION    60  /* 0x3C */
#define MBEDTLS_SSL_ALERT_MSG_PROTOCOL_VERSION      70  /* 0x46 */
#define MBEDTLS_SSL_ALERT_MSG_INSUFFICIENT_SECURITY 71  /* 0x47 */
#define MBEDTLS_SSL_ALERT_MSG_INTERNAL_ERROR        80  /* 0x50 */
#define MBEDTLS_SSL_ALERT_MSG_INAPROPRIATE_FALLBACK 86  /* 0x56 */
#define MBEDTLS_SSL_ALERT_MSG_USER_CANCELED         90  /* 0x5A */
#define MBEDTLS_SSL_ALERT_MSG_NO_RENEGOTIATION     100  /* 0x64 */
#define MBEDTLS_SSL_ALERT_MSG_MISSING_EXTENSION    109  /* 0x6d -- new in TLS 1.3 */
#define MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT      110  /* 0x6E */
#define MBEDTLS_SSL_ALERT_MSG_UNRECOGNIZED_NAME    112  /* 0x70 */
#define MBEDTLS_SSL_ALERT_MSG_UNKNOWN_PSK_IDENTITY 115  /* 0x73 */
#define MBEDTLS_SSL_ALERT_MSG_CERT_REQUIRED        116  /* 0x74 */
#define MBEDTLS_SSL_ALERT_MSG_NO_APPLICATION_PROTOCOL 120 /* 0x78 */

#define MBEDTLS_SSL_HS_HELLO_REQUEST            0
#define MBEDTLS_SSL_HS_CLIENT_HELLO             1
#define MBEDTLS_SSL_HS_SERVER_HELLO             2
#define MBEDTLS_SSL_HS_HELLO_VERIFY_REQUEST     3
#define MBEDTLS_SSL_HS_NEW_SESSION_TICKET       4
#define MBEDTLS_SSL_HS_END_OF_EARLY_DATA        5
#define MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS     8
#define MBEDTLS_SSL_HS_CERTIFICATE             11
#define MBEDTLS_SSL_HS_SERVER_KEY_EXCHANGE     12
#define MBEDTLS_SSL_HS_CERTIFICATE_REQUEST     13
#define MBEDTLS_SSL_HS_SERVER_HELLO_DONE       14
#define MBEDTLS_SSL_HS_CERTIFICATE_VERIFY      15
#define MBEDTLS_SSL_HS_CLIENT_KEY_EXCHANGE     16
#define MBEDTLS_SSL_HS_FINISHED                20
#define MBEDTLS_SSL_HS_MESSAGE_HASH           254

/*
 * TLS extensions
 */
#define MBEDTLS_TLS_EXT_SERVERNAME                   0
#define MBEDTLS_TLS_EXT_SERVERNAME_HOSTNAME          0

#define MBEDTLS_TLS_EXT_MAX_FRAGMENT_LENGTH          1

#define MBEDTLS_TLS_EXT_TRUNCATED_HMAC               4
#define MBEDTLS_TLS_EXT_STATUS_REQUEST               5 /* RFC 6066 TLS 1.2 and 1.3 */

#define MBEDTLS_TLS_EXT_SUPPORTED_ELLIPTIC_CURVES   10
#define MBEDTLS_TLS_EXT_SUPPORTED_GROUPS            10 /* RFC 8422,7919 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_SUPPORTED_POINT_FORMATS     11

#define MBEDTLS_TLS_EXT_SIG_ALG                     13 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_USE_SRTP                    14
#define MBEDTLS_TLS_EXT_HEARTBEAT                   15 /* RFC 6520 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_ALPN                        16

#define MBEDTLS_TLS_EXT_SCT                         18 /* RFC 6962 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_CLI_CERT_TYPE               19 /* RFC 7250 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_SERV_CERT_TYPE              20 /* RFC 7250 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_PADDING                     21 /* RFC 7685 TLS 1.2 and 1.3 */
#define MBEDTLS_TLS_EXT_ENCRYPT_THEN_MAC            22 /* 0x16 */
#define MBEDTLS_TLS_EXT_EXTENDED_MASTER_SECRET  0x0017 /* 23 */

#define MBEDTLS_TLS_EXT_RECORD_SIZE_LIMIT           28 /* RFC 8449 (implemented for TLS 1.3 only) */

#define MBEDTLS_TLS_EXT_SESSION_TICKET              35

#define MBEDTLS_TLS_EXT_PRE_SHARED_KEY              41 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_EARLY_DATA                  42 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS          43 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_COOKIE                      44 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_PSK_KEY_EXCHANGE_MODES      45 /* RFC 8446 TLS 1.3 */

#define MBEDTLS_TLS_EXT_CERT_AUTH                   47 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_OID_FILTERS                 48 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_POST_HANDSHAKE_AUTH         49 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_SIG_ALG_CERT                50 /* RFC 8446 TLS 1.3 */
#define MBEDTLS_TLS_EXT_KEY_SHARE                   51 /* RFC 8446 TLS 1.3 */

#if MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT == 0
#define MBEDTLS_TLS_EXT_CID                         54 /* RFC 9146 DTLS 1.2 CID */
#else
#define MBEDTLS_TLS_EXT_CID                        254 /* Pre-RFC 9146 DTLS 1.2 CID */
#endif

#define MBEDTLS_TLS_EXT_ECJPAKE_KKPP               256 /* experimental */

#define MBEDTLS_TLS_EXT_RENEGOTIATION_INFO      0xFF01

/*
 * Size defines
 */
#if !defined(MBEDTLS_PSK_MAX_LEN)
/*
 * If the library supports TLS 1.3 tickets and the cipher suite
 * TLS1-3-AES-256-GCM-SHA384, set the PSK maximum length to 48 instead of 32.
 * That way, the TLS 1.3 client and server are able to resume sessions where
 * the cipher suite is TLS1-3-AES-256-GCM-SHA384 (pre-shared keys are 48
 * bytes long in that case).
 */
#define MBEDTLS_PSK_MAX_LEN 32 /* 256 bits */
#endif /* !MBEDTLS_PSK_MAX_LEN */

/* Dummy type used only for its size */
union mbedtls_ssl_premaster_secret {
    unsigned char dummy; /* Make the union non-empty even with SSL disabled */
    unsigned char _pms_ecdh[MBEDTLS_ECP_MAX_BYTES];    /* RFC 4492 5.10 */
};

#define MBEDTLS_PREMASTER_SIZE     sizeof(union mbedtls_ssl_premaster_secret)

/* Length in number of bytes of the TLS sequence number */
#define MBEDTLS_SSL_SEQUENCE_NUMBER_LEN 8

/* Helper to state that client_random and server_random need to be stored
 * after the handshake is complete. This is required for context serialization
 * and for the keying material exporter in TLS 1.2. */
#if defined(MBEDTLS_SSL_CONTEXT_SERIALIZATION) || \
    (defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT) && defined(MBEDTLS_SSL_PROTO_TLS1_2))
#define MBEDTLS_SSL_KEEP_RANDBYTES
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SSL state machine
 */
typedef enum {
    MBEDTLS_SSL_HELLO_REQUEST,
    MBEDTLS_SSL_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_HELLO,
    MBEDTLS_SSL_SERVER_CERTIFICATE,
    MBEDTLS_SSL_SERVER_KEY_EXCHANGE,
    MBEDTLS_SSL_CERTIFICATE_REQUEST,
    MBEDTLS_SSL_SERVER_HELLO_DONE,
    MBEDTLS_SSL_CLIENT_CERTIFICATE,
    MBEDTLS_SSL_CLIENT_KEY_EXCHANGE,
    MBEDTLS_SSL_CERTIFICATE_VERIFY,
    MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC,
    MBEDTLS_SSL_CLIENT_FINISHED,
    MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC,
    MBEDTLS_SSL_SERVER_FINISHED,
    MBEDTLS_SSL_FLUSH_BUFFERS,
    MBEDTLS_SSL_HANDSHAKE_WRAPUP,
    MBEDTLS_SSL_NEW_SESSION_TICKET,
    MBEDTLS_SSL_SERVER_HELLO_VERIFY_REQUEST_SENT,
    MBEDTLS_SSL_HELLO_RETRY_REQUEST,
    MBEDTLS_SSL_ENCRYPTED_EXTENSIONS,
    MBEDTLS_SSL_END_OF_EARLY_DATA,
    MBEDTLS_SSL_CLIENT_CERTIFICATE_VERIFY,
    MBEDTLS_SSL_CLIENT_CCS_AFTER_SERVER_FINISHED,
    MBEDTLS_SSL_CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_CCS_AFTER_SERVER_HELLO,
    MBEDTLS_SSL_CLIENT_CCS_AFTER_CLIENT_HELLO,
    MBEDTLS_SSL_SERVER_CCS_AFTER_HELLO_RETRY_REQUEST,
    MBEDTLS_SSL_HANDSHAKE_OVER,
    MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET,
    MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET_FLUSH,
}
mbedtls_ssl_states;

/*
 * Early data status, client side only.
 */

/**
 * \brief          Callback type: send data on the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the send callback (typically a file descriptor)
 * \param buf      Buffer holding the data to send
 * \param len      Length of the data to send
 *
 * \return         The callback must return the number of bytes sent if any,
 *                 or a non-zero error code.
 *                 If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_WRITE
 *                 must be returned when the operation would block.
 *
 * \note           The callback is allowed to send fewer bytes than requested.
 *                 It must always return the number of bytes actually sent.
 */
typedef int mbedtls_ssl_send_t(void *ctx,
                               const unsigned char *buf,
                               size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Callback type: receive data from the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the receive callback (typically a file
 *                 descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 *
 * \returns        If data has been received, the positive number of bytes received.
 * \returns        \c 0 if the connection has been closed.
 * \returns        If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_READ
 *                 must be returned when the operation would block.
 * \returns        Another negative error code on other kinds of failures.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
typedef int mbedtls_ssl_recv_t(void *ctx,
                               unsigned char *buf,
                               size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Callback type: receive data from the network, with timeout
 *
 * \note           That callback must block until data is received, or the
 *                 timeout delay expires, or the operation is interrupted by a
 *                 signal.
 *
 * \param ctx      Context for the receive callback (typically a file descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 * \param timeout  Maximum number of milliseconds to wait for data
 *                 0 means no timeout (potentially waiting forever)
 *
 * \return         The callback must return the number of bytes received,
 *                 or a non-zero error code:
 *                 \c MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out,
 *                 \c MBEDTLS_ERR_SSL_WANT_READ if interrupted by a signal.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
typedef int mbedtls_ssl_recv_timeout_t(void *ctx,
                                       unsigned char *buf,
                                       size_t len,
                                       uint32_t timeout, MinTlsDiagnostics* diagnostics);
/**
 * \brief          Callback type: set a pair of timers/delays to watch
 *
 * \param ctx      Context pointer
 * \param int_ms   Intermediate delay in milliseconds
 * \param fin_ms   Final delay in milliseconds
 *                 0 cancels the current timer.
 *
 * \note           This callback must at least store the necessary information
 *                 for the associated \c mbedtls_ssl_get_timer_t callback to
 *                 return correct information.
 *
 * \note           If using an event-driven style of programming, an event must
 *                 be generated when the final delay is passed. The event must
 *                 cause a call to \c mbedtls_ssl_handshake() with the proper
 *                 SSL context to be scheduled. Care must be taken to ensure
 *                 that at most one such call happens at a time.
 *
 * \note           Only one timer at a time must be running. Calling this
 *                 function while a timer is running must cancel it. Cancelled
 *                 timers must not generate any event.
 */
typedef void mbedtls_ssl_set_timer_t(void *ctx,
                                     uint32_t int_ms,
                                     uint32_t fin_ms);

/**
 * \brief          Callback type: get status of timers/delays
 *
 * \param ctx      Context pointer
 *
 * \return         This callback must return:
 *                 -1 if cancelled (fin_ms == 0),
 *                  0 if none of the delays have passed,
 *                  1 if only the intermediate delay has passed,
 *                  2 if the final delay has passed.
 */
typedef int mbedtls_ssl_get_timer_t(void *ctx);

/* Defined below */
typedef struct mbedtls_ssl_session mbedtls_ssl_session;
typedef struct mbedtls_ssl_context mbedtls_ssl_context;
typedef struct mbedtls_ssl_config  mbedtls_ssl_config;

/* Defined in library/ssl_misc.h */
typedef struct mbedtls_ssl_transform mbedtls_ssl_transform;
typedef struct mbedtls_ssl_handshake_params mbedtls_ssl_handshake_params;
typedef struct mbedtls_ssl_sig_hash_set_t mbedtls_ssl_sig_hash_set_t;
typedef struct mbedtls_ssl_key_cert mbedtls_ssl_key_cert;

/**
 * \brief          Callback type: server-side session cache getter
 *
 *                 The session cache is logically a key value store, with
 *                 keys being session IDs and values being instances of
 *                 mbedtls_ssl_session.
 *
 *                 This callback retrieves an entry in this key-value store.
 *
 * \param data            The address of the session cache structure to query.
 * \param session_id      The buffer holding the session ID to query.
 * \param session_id_len  The length of \p session_id in Bytes.
 * \param session         The address of the session structure to populate.
 *                        It is initialized with mbdtls_ssl_session_init(),
 *                        and the callback must always leave it in a state
 *                        where it can safely be freed via
 *                        mbedtls_ssl_session_free() independent of the
 *                        return code of this function.
 *
 * \return                \c 0 on success
 * \return                A non-zero return value on failure.
 *
 */
typedef int mbedtls_ssl_cache_get_t(void *data,
                                    unsigned char const *session_id,
                                    size_t session_id_len,
                                    mbedtls_ssl_session *session);
/**
 * \brief          Callback type: server-side session cache setter
 *
 *                 The session cache is logically a key value store, with
 *                 keys being session IDs and values being instances of
 *                 mbedtls_ssl_session.
 *
 *                 This callback sets an entry in this key-value store.
 *
 * \param data            The address of the session cache structure to modify.
 * \param session_id      The buffer holding the session ID to query.
 * \param session_id_len  The length of \p session_id in Bytes.
 * \param session         The address of the session to be stored in the
 *                        session cache.
 *
 * \return                \c 0 on success
 * \return                A non-zero return value on failure.
 */
typedef int mbedtls_ssl_cache_set_t(void *data,
                                    unsigned char const *session_id,
                                    size_t session_id_len,
                                    const mbedtls_ssl_session *session);

/** Human-friendly representation of the (D)TLS protocol version. */
typedef enum {
    MBEDTLS_SSL_VERSION_UNKNOWN, /*!< Context not in use or version not yet negotiated. */
    MBEDTLS_SSL_VERSION_TLS1_2 = 0x0303, /*!< (D)TLS 1.2 */
    MBEDTLS_SSL_VERSION_TLS1_3 = 0x0304, /*!< (D)TLS 1.3 */
} mbedtls_ssl_protocol_version;

/*
 * This structure is used for storing current session data.
 *
 * Note: when changing this definition, we need to check and update:
 *  - in tests/suites/test_suite_ssl.function:
 *      ssl_populate_session() and ssl_serialize_session_save_load()
 *  - in library/ssl_tls.c:
 *      mbedtls_ssl_session_init() and mbedtls_ssl_session_free()
 *      mbedtls_ssl_session_save() and ssl_session_load()
 *      ssl_session_copy()
 */
struct mbedtls_ssl_session {

/*!< RecordSizeLimit received from the peer */

    unsigned char MBEDTLS_PRIVATE(exported);
    uint8_t MBEDTLS_PRIVATE(endpoint);          /*!< 0: client, 1: server */

    /** TLS version negotiated in the session. Used if and when renegotiating
     *  or resuming a session instead of the configured minor TLS version.
     */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(tls_version);

    mbedtls_time_t MBEDTLS_PRIVATE(start);       /*!< start time of current session */
    int MBEDTLS_PRIVATE(ciphersuite);            /*!< chosen ciphersuite */
    size_t MBEDTLS_PRIVATE(id_len);              /*!< session id length  */
    unsigned char MBEDTLS_PRIVATE(id)[32];       /*!< session identifier */
    unsigned char MBEDTLS_PRIVATE(master)[48];   /*!< the master secret  */

    mbedtls_x509_crt *MBEDTLS_PRIVATE(peer_cert);       /*!< peer X.509 cert chain */
    uint32_t MBEDTLS_PRIVATE(verify_result);          /*!<  verification result     */

};

/*
 * Identifiers for PRFs used in various versions of TLS.
 */
typedef enum {
    MBEDTLS_SSL_TLS_PRF_NONE,
    MBEDTLS_SSL_TLS_PRF_SHA384,
    MBEDTLS_SSL_TLS_PRF_SHA256,
    MBEDTLS_SSL_HKDF_EXPAND_SHA384,
    MBEDTLS_SSL_HKDF_EXPAND_SHA256
}
mbedtls_tls_prf_types;

typedef enum {
    MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET = 0,
} mbedtls_ssl_key_export_type;

/**
 * \brief           Callback type: Export key alongside random values for
 *                                 session identification, and PRF for
 *                                 implementation of TLS key exporters.
 *
 * \param p_expkey   Context for the callback.
 * \param type       The type of the key that is being exported.
 * \param secret     The address of the buffer holding the secret
 *                   that's being exporterd.
 * \param secret_len The length of \p secret in bytes.
 * \param client_random The client random bytes.
 * \param server_random The server random bytes.
 * \param tls_prf_type The identifier for the PRF used in the handshake
 *                     to which the key belongs.
 */
typedef void mbedtls_ssl_export_keys_t(void *p_expkey,
                                       mbedtls_ssl_key_export_type type,
                                       const unsigned char *secret,
                                       size_t secret_len,
                                       const unsigned char client_random[32],
                                       const unsigned char server_random[32],
                                       mbedtls_tls_prf_types tls_prf_type);

/* A type for storing user data in a library structure.
 *
 * The representation of type may change in future versions of the library.
 * Only the behaviors guaranteed by documented accessor functions are
 * guaranteed to remain stable.
 */
typedef union {
    uintptr_t n;                /* typically a handle to an associated object */
    void *p;                    /* typically a pointer to extra data */
} mbedtls_ssl_user_data_t;

/**
 * SSL/TLS configuration to be shared between mbedtls_ssl_context structures.
 */
struct mbedtls_ssl_config {
    /* Group items mostly by size. This helps to reduce memory wasted to
     * padding. It also helps to keep smaller fields early in the structure,
     * so that elements tend to be in the 128-element direct access window
     * on Arm Thumb, which reduces the code size. */

    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(max_tls_version);  /*!< max. TLS version used */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(min_tls_version);  /*!< min. TLS version used */

    /*
     * Flags (could be bit-fields to save RAM, but separate bytes make
     * the code smaller on architectures with an instruction for direct
     * byte access).
     */

    uint8_t MBEDTLS_PRIVATE(endpoint);      /*!< 0: client, 1: server               */
    uint8_t MBEDTLS_PRIVATE(transport);     /*!< 0: stream (TLS), 1: datagram (DTLS)    */
    uint8_t MBEDTLS_PRIVATE(authmode);      /*!< MBEDTLS_SSL_VERIFY_XXX             */
    /* needed even with renego disabled for LEGACY_BREAK_HANDSHAKE          */
    uint8_t MBEDTLS_PRIVATE(allow_legacy_renegotiation); /*!< MBEDTLS_LEGACY_XXX   */
    uint8_t MBEDTLS_PRIVATE(extended_ms);   /*!< negotiate extended master secret?  */

    /*
     * Pointers
     */

    /** Allowed ciphersuites for (D)TLS 1.2 (0-terminated)                  */
    const int *MBEDTLS_PRIVATE(ciphersuite_list);

    /** Callback for printing debug output                                  */
    void(*MBEDTLS_PRIVATE(f_dbg))(void *, int, const char *, int, const char *);
    void *MBEDTLS_PRIVATE(p_dbg);                    /*!< context for the debug function     */

    /** Callback for getting (pseudo-)random numbers                        */
    int(*MBEDTLS_PRIVATE(f_rng))(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_rng);                    /*!< context for the RNG function       */

    /** Callback to retrieve a session from the cache                       */
    mbedtls_ssl_cache_get_t *MBEDTLS_PRIVATE(f_get_cache);
    /** Callback to store a session into the cache                          */
    mbedtls_ssl_cache_set_t *MBEDTLS_PRIVATE(f_set_cache);
    void *MBEDTLS_PRIVATE(p_cache);                  /*!< context for cache callbacks        */

    /** Callback for setting cert according to SNI extension                */
    int(*MBEDTLS_PRIVATE(f_sni))(void *, mbedtls_ssl_context *, const unsigned char *, size_t, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_sni);                    /*!< context for SNI callback           */

    /** Callback to customize X.509 certificate chain verification          */
    int(*MBEDTLS_PRIVATE(f_vrfy))(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_vrfy);                   /*!< context for X.509 verify calllback */

    const mbedtls_x509_crt_profile *MBEDTLS_PRIVATE(cert_profile); /*!< verification profile */
    mbedtls_ssl_key_cert *MBEDTLS_PRIVATE(key_cert); /*!< own certificate/key pair(s)        */
    mbedtls_x509_crt *MBEDTLS_PRIVATE(ca_chain);     /*!< trusted CAs                        */
    mbedtls_x509_crl *MBEDTLS_PRIVATE(ca_crl);       /*!< trusted CAs CRLs                   */
#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
    mbedtls_x509_crt_ca_cb_t MBEDTLS_PRIVATE(f_ca_cb);
    void *MBEDTLS_PRIVATE(p_ca_cb);
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
    const int *MBEDTLS_PRIVATE(sig_hashes);         /*!< allowed signature hashes           */
#endif
    const uint16_t *MBEDTLS_PRIVATE(sig_algs);      /*!< allowed signature algorithms       */
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

#if defined(MBEDTLS_ECP_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
    const mbedtls_ecp_group_id *MBEDTLS_PRIVATE(curve_list); /*!< allowed curves             */
#endif

    const uint16_t *MBEDTLS_PRIVATE(group_list);     /*!< allowed IANA NamedGroups */

    const char **MBEDTLS_PRIVATE(alpn_list);         /*!< ordered list of protocols          */

    /*
     * Numerical settings (int)
     */

    uint32_t MBEDTLS_PRIVATE(read_timeout);          /*!< timeout for mbedtls_ssl_read (ms)  */

    unsigned int MBEDTLS_PRIVATE(badmac_limit);      /*!< limit of records with a bad MAC    */

    /** User data pointer or handle.
     *
     * The library sets this to \p 0 when creating a context and does not
     * access it afterwards.
     */
    mbedtls_ssl_user_data_t MBEDTLS_PRIVATE(user_data);

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
    const mbedtls_x509_crt *MBEDTLS_PRIVATE(dn_hints);/*!< acceptable client cert issuers    */
#endif
};

struct mbedtls_ssl_context {
    const mbedtls_ssl_config *MBEDTLS_PRIVATE(conf); /*!< configuration information          */

    /*
     * Miscellaneous
     */
    int MBEDTLS_PRIVATE(state);                  /*!< SSL handshake: current state     */

    /**
     * Maximum TLS version to be negotiated, then negotiated TLS version.
     *
     * It is initialized as the configured maximum TLS version to be
     * negotiated by mbedtls_ssl_setup().
     *
     * When renegotiating or resuming a session, it is overwritten in the
     * ClientHello writing preparation stage with the previously negotiated
     * TLS version.
     *
     * On client side, it is updated to the TLS version selected by the server
     * for the handshake when the ServerHello is received.
     *
     * On server side, it is updated to the TLS version the server selects for
     * the handshake when the ClientHello is received.
     */
    mbedtls_ssl_protocol_version MBEDTLS_PRIVATE(tls_version);

    /** Multipurpose field.
     *
     * - DTLS: records with a bad MAC received.
     * - TLS: accumulated length of handshake fragments (up to \c in_hslen).
     *
     * This field is multipurpose in order to preserve the ABI in the
     * Mbed TLS 3.6 LTS branch. Until 3.6.2, it was only used in DTLS
     * and called `badmac_seen`.
     */
    unsigned MBEDTLS_PRIVATE(badmac_seen_or_in_hsfraglen);

    /** Callback to customize X.509 certificate chain verification          */
    int(*MBEDTLS_PRIVATE(f_vrfy))(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics);
    void *MBEDTLS_PRIVATE(p_vrfy);                   /*!< context for X.509 verify callback */

    mbedtls_ssl_send_t *MBEDTLS_PRIVATE(f_send); /*!< Callback for network send */
    mbedtls_ssl_recv_t *MBEDTLS_PRIVATE(f_recv); /*!< Callback for network receive */
    mbedtls_ssl_recv_timeout_t *MBEDTLS_PRIVATE(f_recv_timeout);
    /*!< Callback for network receive with timeout */

    void *MBEDTLS_PRIVATE(p_bio);                /*!< context for I/O operations   */

    /*
     * Session layer
     */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_in);            /*!<  current session data (in)   */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_out);           /*!<  current session data (out)  */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session);               /*!<  negotiated session data     */
    mbedtls_ssl_session *MBEDTLS_PRIVATE(session_negotiate);     /*!<  session data in negotiation */

    mbedtls_ssl_handshake_params *MBEDTLS_PRIVATE(handshake);    /*!<  params required only during
                                                                    the handshake process        */

    /*
     * Record layer transformations
     */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_in);        /*!<  current transform params (in)
                                                                  *    This is always a reference,
                                                                  *    never an owning pointer.        */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_out);       /*!<  current transform params (out)
                                                                  *    This is always a reference,
                                                                  *    never an owning pointer.        */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform);           /*!<  negotiated transform params
                                                                  *    This pointer owns the transform
                                                                  *    it references.                  */
    mbedtls_ssl_transform *MBEDTLS_PRIVATE(transform_negotiate); /*!<  transform params in negotiation
                                                                  *    This pointer owns the transform
                                                                  *    it references.                  */

    /*
     * Timers
     */
    void *MBEDTLS_PRIVATE(p_timer);              /*!< context for the timer callbacks */

    mbedtls_ssl_set_timer_t *MBEDTLS_PRIVATE(f_set_timer);       /*!< set timer callback */
    mbedtls_ssl_get_timer_t *MBEDTLS_PRIVATE(f_get_timer);       /*!< get timer callback */

    /*
     * Record layer (incoming data)
     */
    unsigned char *MBEDTLS_PRIVATE(in_buf);      /*!< input buffer                     */
    unsigned char *MBEDTLS_PRIVATE(in_ctr);      /*!< 64-bit incoming message counter
                                                    TLS: maintained by us
                                                    DTLS: read from peer             */
    unsigned char *MBEDTLS_PRIVATE(in_hdr);      /*!< start of record header           */
    unsigned char *MBEDTLS_PRIVATE(in_len);      /*!< two-bytes message length field   */
    unsigned char *MBEDTLS_PRIVATE(in_iv);       /*!< ivlen-byte IV                    */
    unsigned char *MBEDTLS_PRIVATE(in_msg);      /*!< message contents (in_iv+ivlen)   */
    unsigned char *MBEDTLS_PRIVATE(in_offt);     /*!< read offset in application data  */

    int MBEDTLS_PRIVATE(in_msgtype);             /*!< record header: message type      */
    size_t MBEDTLS_PRIVATE(in_msglen);           /*!< record header: message length    */
    size_t MBEDTLS_PRIVATE(in_left);             /*!< amount of data read so far       */

    size_t MBEDTLS_PRIVATE(in_hslen);            /*!< current handshake message length,
                                                    including the handshake header   */
    int MBEDTLS_PRIVATE(nb_zero);                /*!< # of 0-length encrypted messages */

    int MBEDTLS_PRIVATE(keep_current_message);   /*!< drop or reuse current message
                                                    on next call to record layer? */

    /* The following three variables indicate if and, if yes,
     * what kind of alert is pending to be sent.
     */
    unsigned char MBEDTLS_PRIVATE(send_alert);   /*!< Determines if a fatal alert
                                                    should be sent. Values:
                                                    - \c 0 , no alert is to be sent.
                                                    - \c 1 , alert is to be sent. */
    unsigned char MBEDTLS_PRIVATE(alert_type);   /*!< Type of alert if send_alert
                                                    != 0 */
    int MBEDTLS_PRIVATE(alert_reason);           /*!< The error code to be returned
                                                    to the user once the fatal alert
                                                    has been sent. */

    /*
     * Record layer (outgoing data)
     */
    unsigned char *MBEDTLS_PRIVATE(out_buf);     /*!< output buffer                    */
    unsigned char *MBEDTLS_PRIVATE(out_ctr);     /*!< 64-bit outgoing message counter  */
    unsigned char *MBEDTLS_PRIVATE(out_hdr);     /*!< start of record header           */
    unsigned char *MBEDTLS_PRIVATE(out_len);     /*!< two-bytes message length field   */
    unsigned char *MBEDTLS_PRIVATE(out_iv);      /*!< ivlen-byte IV                    */
    unsigned char *MBEDTLS_PRIVATE(out_msg);     /*!< message contents (out_iv+ivlen)  */

    int MBEDTLS_PRIVATE(out_msgtype);            /*!< record header: message type      */
    size_t MBEDTLS_PRIVATE(out_msglen);          /*!< record header: message length    */
    size_t MBEDTLS_PRIVATE(out_left);            /*!< amount of data not yet written   */

    unsigned char MBEDTLS_PRIVATE(cur_out_ctr)[MBEDTLS_SSL_SEQUENCE_NUMBER_LEN]; /*!<  Outgoing record sequence  number. */

    /*
     * User settings
     */
    /** Expected peer CN for verification.
     *
     * Also used on clients for SNI,
     * and for TLS 1.3 session resumption using tickets.
     *
     * The value of this field can be:
     * - \p NULL in a newly initialized or reset context.
     * - A heap-allocated copy of the last value passed to
     *   mbedtls_ssl_set_hostname(), if the last call had a non-null
     *  \p hostname argument.
     * - A special value to indicate that mbedtls_ssl_set_hostname()
     *   was called with \p NULL (as opposed to never having been called).
     *   See `mbedtls_ssl_get_hostname_pointer()` in `ssl_tls.c`.
     *
     * If this field contains the value \p NULL and the configuration option
     * #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
     * is unset, on a TLS client, attempting to verify a server certificate
     * results in the error
     * #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME.
     *
     * If this field contains the special value described above, or if
     * the value is \p NULL and the configuration option
     * #MBEDTLS_SSL_CLI_ALLOW_WEAK_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
     * is set, then the peer name verification is skipped, which may be
     * insecure, especially on a client. Furthermore, on a client, the
     * server_name extension is not sent, and the server name is ignored
     * in TLS 1.3 session resumption using tickets.
     */
    char *MBEDTLS_PRIVATE(hostname);

    const char *MBEDTLS_PRIVATE(alpn_chosen);    /*!<  negotiated protocol                   */

    /*
     * Information for DTLS hello verify
     */

    /*
     * Secure renegotiation
     */
    /* needed to know when to send extension on server */
    int MBEDTLS_PRIVATE(secure_renegotiation);           /*!<  does peer support legacy or
                                                            secure renegotiation           */

    /** Callback to export key block and master secret                      */
    mbedtls_ssl_export_keys_t *MBEDTLS_PRIVATE(f_export_keys);
    void *MBEDTLS_PRIVATE(p_export_keys);            /*!< context for key export callback    */

    /** User data pointer or handle.
     *
     * The library sets this to \p 0 when creating a context and does not
     * access it afterwards.
     *
     * \warning Serializing and restoring an SSL context with
     *          mbedtls_ssl_context_save() and mbedtls_ssl_context_load()
     *          does not currently restore the user data.
     */
    mbedtls_ssl_user_data_t MBEDTLS_PRIVATE(user_data);
};

/**
 * \brief               Return the name of the ciphersuite associated with the
 *                      given ID
 *
 * \param ciphersuite_id SSL ciphersuite ID
 *
 * \return              a string containing the ciphersuite name
 */
const char *mbedtls_ssl_get_ciphersuite_name(const int ciphersuite_id);

/**
 * \brief               Return the ID of the ciphersuite associated with the
 *                      given name
 *
 * \param ciphersuite_name SSL ciphersuite name
 *
 * \return              the ID with the ciphersuite or 0 if not found
 */
int mbedtls_ssl_get_ciphersuite_id(const char *ciphersuite_name);

/**
 * \brief          Initialize an SSL context
 *                 Just makes the context ready for mbedtls_ssl_setup() or
 *                 mbedtls_ssl_free()
 *
 * \param ssl      SSL context
 */
void mbedtls_ssl_init(mbedtls_ssl_context *ssl);

/**
 * \brief          Set up an SSL context for use
 *
 * \note           No copy of the configuration context is made, it can be
 *                 shared by many mbedtls_ssl_context structures.
 *
 * \warning        The conf structure will be accessed during the session.
 *                 It must not be modified or freed as long as the session
 *                 is active.
 *
 * \warning        This function must be called exactly once per context.
 *                 Calling mbedtls_ssl_setup again is not supported, even
 *                 if no session is active.
 *
 * \warning        After setting up a client context, if certificate-based
 *                 authentication is enabled, you should call
 *                 mbedtls_ssl_set_hostname() to specifiy the expected
 *                 name of the server. Without this, in most scenarios,
 *                 the TLS connection is insecure. See
 *                 #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *                 for more information.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *
 * \param ssl      SSL context
 * \param conf     SSL configuration to use
 *
 * \return         0 if successful, or MBEDTLS_ERR_SSL_ALLOC_FAILED if
 *                 memory allocation failed
 */
int mbedtls_ssl_setup(mbedtls_ssl_context *ssl,
                      const mbedtls_ssl_config *conf, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Reset an already initialized SSL context for re-use
 *                 while retaining application-set variables, function
 *                 pointers and data.
 *
 * \param ssl      SSL context
 * \return         0 if successful, or MBEDTLS_ERR_SSL_ALLOC_FAILED or
                   MBEDTLS_ERR_SSL_HW_ACCEL_FAILED
 */
int mbedtls_ssl_session_reset(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Set the current endpoint type
 *
 * \param conf     SSL configuration
 * \param endpoint must be MBEDTLS_SSL_IS_CLIENT or MBEDTLS_SSL_IS_SERVER
 */
void mbedtls_ssl_conf_endpoint(mbedtls_ssl_config *conf, int endpoint);

/**
 * \brief          Get the current endpoint type
 *
 * \param conf     SSL configuration
 *
 * \return         Endpoint type, either MBEDTLS_SSL_IS_CLIENT
 *                 or MBEDTLS_SSL_IS_SERVER
 */
static inline int mbedtls_ssl_conf_get_endpoint(const mbedtls_ssl_config *conf)
{
    return conf->MBEDTLS_PRIVATE(endpoint);
}

/**
 * \brief           Set the transport type (TLS or DTLS).
 *                  Default: TLS
 *
 * \note            For DTLS, you must either provide a recv callback that
 *                  doesn't block, or one that handles timeouts, see
 *                  \c mbedtls_ssl_set_bio(). You also need to provide timer
 *                  callbacks with \c mbedtls_ssl_set_timer_cb().
 *
 * \param conf      SSL configuration
 * \param transport transport type:
 *                  MBEDTLS_SSL_TRANSPORT_STREAM for TLS,
 *                  MBEDTLS_SSL_TRANSPORT_DATAGRAM for DTLS.
 */
void mbedtls_ssl_conf_transport(mbedtls_ssl_config *conf, int transport);

/**
 * \brief          Set the certificate verification mode
 *                 Default: NONE on server, REQUIRED on client
 *
 * \param conf     SSL configuration
 * \param authmode can be:
 *
 *  MBEDTLS_SSL_VERIFY_NONE:      peer certificate is not checked
 *                        (default on server)
 *                        (insecure on client)
 *
 *  MBEDTLS_SSL_VERIFY_OPTIONAL:  peer certificate is checked, however the
 *                        handshake continues even if verification failed;
 *                        mbedtls_ssl_get_verify_result() can be called after the
 *                        handshake is complete.
 *
 *  MBEDTLS_SSL_VERIFY_REQUIRED:  peer *must* present a valid certificate,
 *                        handshake is aborted if verification failed.
 *                        (default on client)
 *
 * \note On client, MBEDTLS_SSL_VERIFY_REQUIRED is the recommended mode.
 * With MBEDTLS_SSL_VERIFY_OPTIONAL, the user needs to call mbedtls_ssl_get_verify_result() at
 * the right time(s), which may not be obvious, while REQUIRED always perform
 * the verification as soon as possible. For example, REQUIRED was protecting
 * against the "triple handshake" attack even before it was found.
 */
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode);

/**
 * \brief          Set the verification callback (Optional).
 *
 *                 If set, the provided verify callback is called for each
 *                 certificate in the peer's CRT chain, including the trusted
 *                 root. For more information, please see the documentation of
 *                 \c mbedtls_x509_crt_verify().
 *
 * \note           For per context callbacks and contexts, please use
 *                 mbedtls_ssl_set_verify() instead.
 *
 * \param conf     The SSL configuration to use.
 * \param f_vrfy   The verification callback to use during CRT verification.
 * \param p_vrfy   The opaque context to be passed to the callback.
 */
void mbedtls_ssl_conf_verify(mbedtls_ssl_config *conf,
                             int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                             void *p_vrfy);

/**
 * \brief          Set the random number generator callback
 *
 * \note           The callback with its parameter must remain valid as
 *                 long as there is an SSL context that uses the
 *                 SSL configuration.
 *
 * \param conf     SSL configuration
 * \param f_rng    RNG function (mandatory)
 * \param p_rng    RNG parameter
 */
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          mbedtls_f_rng_t *f_rng,
                          void *p_rng);

/**
 * \brief          Set the debug callback
 *
 *                 The callback has the following argument:
 *                 void *           opaque context for the callback
 *                 int              debug level
 *                 const char *     file name
 *                 int              line number
 *                 const char *     message
 *
 * \param conf     SSL configuration
 * \param f_dbg    debug function
 * \param p_dbg    debug parameter
 */
void mbedtls_ssl_conf_dbg(mbedtls_ssl_config *conf,
                          void (*f_dbg)(void *, int, const char *, int, const char *),
                          void  *p_dbg);

/**
 * \brief          Return the SSL configuration structure associated
 *                 with the given SSL context.
 *
 * \note           The pointer returned by this function is guaranteed to
 *                 remain valid until the context is freed.
 *
 * \param ssl      The SSL context to query.
 * \return         Pointer to the SSL configuration associated with \p ssl.
 */
static inline const mbedtls_ssl_config *mbedtls_ssl_context_get_config(
    const mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(conf);
}

/**
 * \brief          Set the underlying BIO callbacks for write, read and
 *                 read-with-timeout.
 *
 * \param ssl      SSL context
 * \param p_bio    parameter (context) shared by BIO callbacks
 * \param f_send   write callback
 * \param f_recv   read callback
 * \param f_recv_timeout blocking read callback with timeout.
 *
 * \note           One of f_recv or f_recv_timeout can be NULL, in which case
 *                 the other is used. If both are non-NULL, f_recv_timeout is
 *                 used and f_recv is ignored (as if it were NULL).
 *
 * \note           The two most common use cases are:
 *                 - non-blocking I/O, f_recv != NULL, f_recv_timeout == NULL
 *                 - blocking I/O, f_recv == NULL, f_recv_timeout != NULL
 *
 * \note           For DTLS, you need to provide either a non-NULL
 *                 f_recv_timeout callback, or a f_recv that doesn't block.
 *
 * \note           See the documentations of \c mbedtls_ssl_send_t,
 *                 \c mbedtls_ssl_recv_t and \c mbedtls_ssl_recv_timeout_t for
 *                 the conventions those callbacks must follow.
 *
 * \note           On some platforms, net_sockets.c provides
 *                 \c mbedtls_net_send(), \c mbedtls_net_recv() and
 *                 \c mbedtls_net_recv_timeout() that are suitable to be used
 *                 here.
 */
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,
                         void *p_bio,
                         mbedtls_ssl_send_t *f_send,
                         mbedtls_ssl_recv_t *f_recv,
                         mbedtls_ssl_recv_timeout_t *f_recv_timeout);

/**
 * \brief          Set a connection-specific verification callback (optional).
 *
 *                 If set, the provided verify callback is called for each
 *                 certificate in the peer's CRT chain, including the trusted
 *                 root. For more information, please see the documentation of
 *                 \c mbedtls_x509_crt_verify().
 *
 * \note           This call is analogous to mbedtls_ssl_conf_verify() but
 *                 binds the verification callback and context to an SSL context
 *                 as opposed to an SSL configuration.
 *                 If mbedtls_ssl_conf_verify() and mbedtls_ssl_set_verify()
 *                 are both used, mbedtls_ssl_set_verify() takes precedence.
 *
 * \param ssl      The SSL context to use.
 * \param f_vrfy   The verification callback to use during CRT verification.
 * \param p_vrfy   The opaque context to be passed to the callback.
 */
void mbedtls_ssl_set_verify(mbedtls_ssl_context *ssl,
                            int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *, MinTlsDiagnostics* diagnostics),
                            void *p_vrfy);

/**
 * \brief          Set the timeout period for mbedtls_ssl_read()
 *                 (Default: no timeout.)
 *
 * \param conf     SSL configuration context
 * \param timeout  Timeout value in milliseconds.
 *                 Use 0 for no timeout (default).
 *
 * \note           With blocking I/O, this will only work if a non-NULL
 *                 \c f_recv_timeout was set with \c mbedtls_ssl_set_bio().
 *                 With non-blocking I/O, this will only work if timer
 *                 callbacks were set with \c mbedtls_ssl_set_timer_cb().
 *
 * \note           With non-blocking I/O, you may also skip this function
 *                 altogether and handle timeouts at the application layer.
 */
void mbedtls_ssl_conf_read_timeout(mbedtls_ssl_config *conf, uint32_t timeout);

/**
 * \brief          Check whether a buffer contains a valid and authentic record
 *                 that has not been seen before. (DTLS only).
 *
 *                 This function does not change the user-visible state
 *                 of the SSL context. Its sole purpose is to provide
 *                 an indication of the legitimacy of an incoming record.
 *
 *                 This can be useful e.g. in distributed server environments
 *                 using the DTLS Connection ID feature, in which connections
 *                 might need to be passed between service instances on a change
 *                 of peer address, but where such disruptive operations should
 *                 only happen after the validity of incoming records has been
 *                 confirmed.
 *
 * \param ssl      The SSL context to use.
 * \param buf      The address of the buffer holding the record to be checked.
 *                 This must be a read/write buffer of length \p buflen Bytes.
 * \param buflen   The length of \p buf in Bytes.
 *
 * \note           This routine only checks whether the provided buffer begins
 *                 with a valid and authentic record that has not been seen
 *                 before, but does not check potential data following the
 *                 initial record. In particular, it is possible to pass DTLS
 *                 datagrams containing multiple records, in which case only
 *                 the first record is checked.
 *
 * \note           This function modifies the input buffer \p buf. If you need
 *                 to preserve the original record, you have to maintain a copy.
 *
 * \return         \c 0 if the record is valid and authentic and has not been
 *                 seen before.
 * \return         MBEDTLS_ERR_SSL_INVALID_MAC if the check completed
 *                 successfully but the record was found to be not authentic.
 * \return         MBEDTLS_ERR_SSL_INVALID_RECORD if the check completed
 *                 successfully but the record was found to be invalid for
 *                 a reason different from authenticity checking.
 * \return         MBEDTLS_ERR_SSL_UNEXPECTED_RECORD if the check completed
 *                 successfully but the record was found to be unexpected
 *                 in the state of the SSL context, including replayed records.
 * \return         Another negative error code on different kinds of failure.
 *                 In this case, the SSL context becomes unusable and needs
 *                 to be freed or reset before reuse.
 */
int mbedtls_ssl_check_record(mbedtls_ssl_context const *ssl,
                             unsigned char *buf,
                             size_t buflen, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Set the timer callbacks (Mandatory for DTLS.)
 *
 * \param ssl      SSL context
 * \param p_timer  parameter (context) shared by timer callbacks
 * \param f_set_timer   set timer callback
 * \param f_get_timer   get timer callback. Must return:
 *
 * \note           See the documentation of \c mbedtls_ssl_set_timer_t and
 *                 \c mbedtls_ssl_get_timer_t for the conventions this pair of
 *                 callbacks must follow.
 *
 * \note           On some platforms, timing.c provides
 *                 \c mbedtls_timing_set_delay() and
 *                 \c mbedtls_timing_get_delay() that are suitable for using
 *                 here, except if using an event-driven style.
 *
 * \note           See also the "DTLS tutorial" article in our knowledge base.
 *                 https://mbed-tls.readthedocs.io/en/latest/kb/how-to/dtls-tutorial
 */
void mbedtls_ssl_set_timer_cb(mbedtls_ssl_context *ssl,
                              void *p_timer,
                              mbedtls_ssl_set_timer_t *f_set_timer,
                              mbedtls_ssl_get_timer_t *f_get_timer);

/**
 * \brief           Callback type: generate and write session ticket
 *
 * \note            This describes what a callback implementation should do.
 *                  This callback should generate an encrypted and
 *                  authenticated ticket for the session and write it to the
 *                  output buffer. Here, ticket means the opaque ticket part
 *                  of the NewSessionTicket structure of RFC 5077.
 *
 * \param p_ticket  Context for the callback
 * \param session   SSL session to be written in the ticket
 * \param start     Start of the output buffer
 * \param end       End of the output buffer
 * \param tlen      On exit, holds the length written
 * \param lifetime  On exit, holds the lifetime of the ticket in seconds
 *
 * \return          0 if successful, or
 *                  a specific MBEDTLS_ERR_XXX code.
 */
typedef int mbedtls_ssl_ticket_write_t(void *p_ticket,
                                       const mbedtls_ssl_session *session,
                                       unsigned char *start,
                                       const unsigned char *end,
                                       size_t *tlen,
                                       uint32_t *lifetime);

/**
 * \brief           Callback type: parse and load session ticket
 *
 * \note            This describes what a callback implementation should do.
 *                  This callback should parse a session ticket as generated
 *                  by the corresponding mbedtls_ssl_ticket_write_t function,
 *                  and, if the ticket is authentic and valid, load the
 *                  session.
 *
 * \note            The implementation is allowed to modify the first len
 *                  bytes of the input buffer, eg to use it as a temporary
 *                  area for the decrypted ticket contents.
 *
 * \param p_ticket  Context for the callback
 * \param session   SSL session to be loaded
 * \param buf       Start of the buffer containing the ticket
 * \param len       Length of the ticket.
 *
 * \return          0 if successful, or
 *                  MBEDTLS_ERR_SSL_INVALID_MAC if not authentic, or
 *                  MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED if expired, or
 *                  any other non-zero code for other failures.
 */
typedef int mbedtls_ssl_ticket_parse_t(void *p_ticket,
                                       mbedtls_ssl_session *session,
                                       unsigned char *buf,
                                       size_t len);

/**
 * \brief          Get the session-id buffer.
 *
 * \param session  SSL session.
 *
 * \return         The address of the session-id buffer.
 */
static inline unsigned const char (*mbedtls_ssl_session_get_id(const mbedtls_ssl_session *
                                                               session))[32]
{
    return &session->MBEDTLS_PRIVATE(id);
}

/**
 * \brief          Get the size of the session-id.
 *
 * \param session  SSL session.
 *
 * \return         size_t size of session-id buffer.
 */
static inline size_t mbedtls_ssl_session_get_id_len(const mbedtls_ssl_session *session)
{
    return session->MBEDTLS_PRIVATE(id_len);
}

/**
 * \brief          Get the ciphersuite-id.
 *
 * \param session  SSL session.
 *
 * \return         int represetation for ciphersuite.
 */
static inline int mbedtls_ssl_session_get_ciphersuite_id(const mbedtls_ssl_session *session)
{
    return session->MBEDTLS_PRIVATE(ciphersuite);
}

/**
 * \brief   Configure a key export callback.
 *          (Default: none.)
 *
 *          This API can be used for two purposes:
 *          - Debugging: Use this API to e.g. generate an NSSKeylog
 *            file and use it to inspect encrypted traffic in tools
 *            such as Wireshark.
 *          - Application-specific export: Use this API to implement
 *            key exporters, e.g. for EAP-TLS or DTLS-SRTP.
 *
 *
 * \param ssl            The SSL context to which the export
 *                       callback should be attached.
 * \param f_export_keys  The callback for the key export.
 * \param p_export_keys  The opaque context pointer to be passed to the
 *                       callback \p f_export_keys.
 */
void mbedtls_ssl_set_export_keys_cb(mbedtls_ssl_context *ssl,
                                    mbedtls_ssl_export_keys_t *f_export_keys,
                                    void *p_export_keys);

/** \brief Set the user data in an SSL configuration to a pointer.
 *
 * You can retrieve this value later with mbedtls_ssl_conf_get_user_data_p().
 *
 * \note The library stores \c p without accessing it. It is the responsibility
 *       of the caller to ensure that the pointer remains valid.
 *
 * \param conf           The SSL configuration context to modify.
 * \param p              The new value of the user data.
 */
static inline void mbedtls_ssl_conf_set_user_data_p(
    mbedtls_ssl_config *conf,
    void *p)
{
    conf->MBEDTLS_PRIVATE(user_data).p = p;
}

/** \brief Set the user data in an SSL configuration to an integer.
 *
 * You can retrieve this value later with mbedtls_ssl_conf_get_user_data_n().
 *
 * \param conf           The SSL configuration context to modify.
 * \param n              The new value of the user data.
 */
static inline void mbedtls_ssl_conf_set_user_data_n(
    mbedtls_ssl_config *conf,
    uintptr_t n)
{
    conf->MBEDTLS_PRIVATE(user_data).n = n;
}

/** \brief Retrieve the user data in an SSL configuration as a pointer.
 *
 * This is the value last set with mbedtls_ssl_conf_set_user_data_p(), or
 * \c NULL if mbedtls_ssl_conf_set_user_data_p() has not previously been
 * called. The value is undefined if mbedtls_ssl_conf_set_user_data_n() has
 * been called without a subsequent call to mbedtls_ssl_conf_set_user_data_p().
 *
 * \param conf           The SSL configuration context to modify.
 * \return               The current value of the user data.
 */
static inline void *mbedtls_ssl_conf_get_user_data_p(
    mbedtls_ssl_config *conf)
{
    return conf->MBEDTLS_PRIVATE(user_data).p;
}

/** \brief Retrieve the user data in an SSL configuration as an integer.
 *
 * This is the value last set with mbedtls_ssl_conf_set_user_data_n(), or
 * \c 0 if mbedtls_ssl_conf_set_user_data_n() has not previously been
 * called. The value is undefined if mbedtls_ssl_conf_set_user_data_p() has
 * been called without a subsequent call to mbedtls_ssl_conf_set_user_data_n().
 *
 * \param conf           The SSL configuration context to modify.
 * \return               The current value of the user data.
 */
static inline uintptr_t mbedtls_ssl_conf_get_user_data_n(
    mbedtls_ssl_config *conf)
{
    return conf->MBEDTLS_PRIVATE(user_data).n;
}

/** \brief Set the user data in an SSL context to a pointer.
 *
 * You can retrieve this value later with mbedtls_ssl_get_user_data_p().
 *
 * \note The library stores \c p without accessing it. It is the responsibility
 *       of the caller to ensure that the pointer remains valid.
 *
 * \param ssl            The SSL context to modify.
 * \param p              The new value of the user data.
 */
static inline void mbedtls_ssl_set_user_data_p(
    mbedtls_ssl_context *ssl,
    void *p)
{
    ssl->MBEDTLS_PRIVATE(user_data).p = p;
}

/** \brief Set the user data in an SSL context to an integer.
 *
 * You can retrieve this value later with mbedtls_ssl_get_user_data_n().
 *
 * \param ssl            The SSL context to modify.
 * \param n              The new value of the user data.
 */
static inline void mbedtls_ssl_set_user_data_n(
    mbedtls_ssl_context *ssl,
    uintptr_t n)
{
    ssl->MBEDTLS_PRIVATE(user_data).n = n;
}

/** \brief Retrieve the user data in an SSL context as a pointer.
 *
 * This is the value last set with mbedtls_ssl_set_user_data_p(), or
 * \c NULL if mbedtls_ssl_set_user_data_p() has not previously been
 * called. The value is undefined if mbedtls_ssl_set_user_data_n() has
 * been called without a subsequent call to mbedtls_ssl_set_user_data_p().
 *
 * \param ssl            The SSL context to modify.
 * \return               The current value of the user data.
 */
static inline void *mbedtls_ssl_get_user_data_p(
    mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(user_data).p;
}

/** \brief Retrieve the user data in an SSL context as an integer.
 *
 * This is the value last set with mbedtls_ssl_set_user_data_n(), or
 * \c 0 if mbedtls_ssl_set_user_data_n() has not previously been
 * called. The value is undefined if mbedtls_ssl_set_user_data_p() has
 * been called without a subsequent call to mbedtls_ssl_set_user_data_n().
 *
 * \param ssl            The SSL context to modify.
 * \return               The current value of the user data.
 */
static inline uintptr_t mbedtls_ssl_get_user_data_n(
    mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(user_data).n;
}

/**
 * \brief          Callback type: generate a cookie
 *
 * \param ctx      Context for the callback
 * \param p        Buffer to write to,
 *                 must be updated to point right after the cookie
 * \param end      Pointer to one past the end of the output buffer
 * \param info     Client ID info that was passed to
 *                 \c mbedtls_ssl_set_client_transport_id()
 * \param ilen     Length of info in bytes
 *
 * \return         The callback must return 0 on success,
 *                 or a negative error code.
 */
typedef int mbedtls_ssl_cookie_write_t(void *ctx,
                                       unsigned char **p, unsigned char *end,
                                       const unsigned char *info, size_t ilen);

/**
 * \brief          Callback type: verify a cookie
 *
 * \param ctx      Context for the callback
 * \param cookie   Cookie to verify
 * \param clen     Length of cookie
 * \param info     Client ID info that was passed to
 *                 \c mbedtls_ssl_set_client_transport_id()
 * \param ilen     Length of info in bytes
 *
 * \return         The callback must return 0 if cookie is valid,
 *                 or a negative error code.
 */
typedef int mbedtls_ssl_cookie_check_t(void *ctx,
                                       const unsigned char *cookie, size_t clen,
                                       const unsigned char *info, size_t ilen);

/**
 * \brief          Set a limit on the number of records with a bad MAC
 *                 before terminating the connection.
 *                 (DTLS only, no effect on TLS.)
 *                 Default: 0 (disabled).
 *
 * \param conf     SSL configuration
 * \param limit    Limit, or 0 to disable.
 *
 * \note           If the limit is N, then the connection is terminated when
 *                 the Nth non-authentic record is seen.
 *
 * \note           Records with an invalid header are not counted, only the
 *                 ones going through the authentication-decryption phase.
 *
 * \note           This is a security trade-off related to the fact that it's
 *                 often relatively easy for an active attacker to inject UDP
 *                 datagrams. On one hand, setting a low limit here makes it
 *                 easier for such an attacker to forcibly terminated a
 *                 connection. On the other hand, a high limit or no limit
 *                 might make us waste resources checking authentication on
 *                 many bogus packets.
 */
void mbedtls_ssl_conf_dtls_badmac_limit(mbedtls_ssl_config *conf, unsigned limit);

/**
 * \brief          Load a session for session resumption.
 *
 *                 Sessions loaded through this call will be considered
 *                 for session resumption in the next handshake.
 *
 * \note           Even if this call succeeds, it is not guaranteed that
 *                 the next handshake will indeed be shortened through the
 *                 use of session resumption: The server is always free
 *                 to reject any attempt for resumption and fall back to
 *                 a full handshake.
 *
 * \note           This function can handle a variety of mechanisms for session
 *                 resumption: For TLS 1.2, both session ID-based resumption
 *                 and ticket-based resumption will be considered. For TLS 1.3,
 *                 sessions equate to tickets, and loading one session by
 *                 calling this function will lead to its corresponding ticket
 *                 being advertised as resumption PSK by the client. This
 *                 depends on session tickets being enabled (see
 *                 #MBEDTLS_SSL_SESSION_TICKETS configuration option) though.
 *                 If session tickets are disabled, a call to this function
 *                 with a TLS 1.3 session, will not have any effect on the next
 *                 handshake for the SSL context \p ssl.
 *
 * \param ssl      The SSL context representing the connection which should
 *                 be attempted to be setup using session resumption. This
 *                 must be initialized via mbedtls_ssl_init() and bound to
 *                 an SSL configuration via mbedtls_ssl_setup(), but
 *                 the handshake must not yet have been started.
 * \param session  The session to be considered for session resumption.
 *                 This must be a session previously exported via
 *                 mbedtls_ssl_get_session(), and potentially serialized and
 *                 deserialized through mbedtls_ssl_session_save() and
 *                 mbedtls_ssl_session_load() in the meantime.
 *
 * \return         \c 0 if successful.
 * \return         \c MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE if the session
 *                 could not be loaded because one session has already been
 *                 loaded. This error is non-fatal, and has no observable
 *                 effect on the SSL context or the session that was attempted
 *                 to be loaded.
 * \return         Another negative error code on other kinds of failure.
 *
 * \sa             mbedtls_ssl_get_session()
 * \sa             mbedtls_ssl_session_load()
 */
int mbedtls_ssl_set_session(mbedtls_ssl_context *ssl, const mbedtls_ssl_session *session, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Load serialized session data into a session structure.
 *                 On client, this can be used for loading saved sessions
 *                 before resuming them with mbedtls_ssl_set_session().
 *                 On server, this can be used for alternative implementations
 *                 of session cache or session tickets.
 *
 * \warning        The serialized data contains highly sensitive material,
 *                 including a resumption key (TLS 1.3) or the master secret
 *                 (TLS 1.2) from which the session's traffic keys are derived.
 *
 *                 The serialized data is not cryptographically protected.
 *                 It is the responsibility of the user of the
 *                 mbedtls_ssl_session_save() and
 *                 mbedtls_ssl_session_load() APIs to ensure both its
 *                 confidentiality and integrity while stored or transported.
 *
 *                 A breach of confidentiality could result in full compromise
 *                 of the associated TLS session, including loss of
 *                 confidentiality and integrity of past and future
 *                 application data protected under that session.
 *
 *                 A breach of integrity may allow modification of the
 *                 serialized data prior to restoration. As it represents
 *                 trusted internal context, tampering could potentially result
 *                 in arbitrary code execution or other severe compromise of
 *                 the hosting process.
 *
 * \warning        If a peer certificate chain is associated with the session,
 *                 the serialized state will only contain the peer's
 *                 end-entity certificate and the result of the chain
 *                 verification (unless verification was disabled), but not
 *                 the rest of the chain.
 *
 * \see            mbedtls_ssl_session_save()
 * \see            mbedtls_ssl_set_session()
 *
 * \param session  The session structure to be populated. It must have been
 *                 initialised with mbedtls_ssl_session_init() but not
 *                 populated yet.
 * \param buf      The buffer holding the serialized session data. It must be a
 *                 readable buffer of at least \p len bytes.
 * \param len      The size of the serialized data in bytes.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_SSL_ALLOC_FAILED if memory allocation failed.
 * \return         #MBEDTLS_ERR_SSL_BAD_INPUT_DATA if input data is invalid.
 * \return         #MBEDTLS_ERR_SSL_VERSION_MISMATCH if the serialized data
 *                 was generated in a different version or configuration of
 *                 Mbed TLS.
 * \return         Another negative value for other kinds of errors (for
 *                 example, unsupported features in the embedded certificate).
 */
int mbedtls_ssl_session_load(mbedtls_ssl_session *session,
                             const unsigned char *buf,
                             size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Save session structure as serialized data in a buffer.
 *                 On client, this can be used for saving session data,
 *                 potentially in non-volatile storage, for resuming later.
 *                 On server, this can be used for alternative implementations
 *                 of session cache or session tickets.
 *
 * \see            mbedtls_ssl_session_load()
 *
 * \warning        The serialized data contains highly sensitive material,
 *                 including a resumption key (TLS 1.3) or the master secret
 *                 (TLS 1.2) from which the session's traffic keys are derived.
 *
 *                 The serialized data is not cryptographically protected.
 *                 It is the responsibility of the user of the
 *                 mbedtls_ssl_session_save() and
 *                 mbedtls_ssl_session_load() APIs to ensure both its
 *                 confidentiality and integrity while stored or transported.
 *
 *                 See the mbedtls_ssl_session_load() documentation for
 *                 additional information.
 *
 * \param session  The session structure to be saved.
 * \param buf      The buffer to write the serialized data to. It must be a
 *                 writeable buffer of at least \p buf_len bytes, or may be \c
 *                 NULL if \p buf_len is \c 0.
 * \param buf_len  The number of bytes available for writing in \p buf.
 * \param olen     The size in bytes of the data that has been or would have
 *                 been written. It must point to a valid \c size_t.
 *
 * \note           \p olen is updated to the correct value regardless of
 *                 whether \p buf_len was large enough. This makes it possible
 *                 to determine the necessary size by calling this function
 *                 with \p buf set to \c NULL and \p buf_len to \c 0.
 *
 * \note           For TLS 1.3 sessions, this feature is supported only if the
 *                 MBEDTLS_SSL_SESSION_TICKETS configuration option is enabled,
 *                 as in TLS 1.3 session resumption is possible only with
 *                 tickets.
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL if \p buf is too small.
 * \return         #MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE if the
 *                 MBEDTLS_SSL_SESSION_TICKETS configuration option is disabled
 *                 and the session is a TLS 1.3 session.
 */
int mbedtls_ssl_session_save(const mbedtls_ssl_session *session,
                             unsigned char *buf,
                             size_t buf_len,
                             size_t *olen, MinTlsDiagnostics* diagnostics);

/**
 * \brief               Set the list of allowed ciphersuites and the preference
 *                      order. First in the list has the highest preference.
 *
 *                      For TLS 1.2, the notion of ciphersuite determines both
 *                      the key exchange mechanism and the suite of symmetric
 *                      algorithms to be used during and after the handshake.
 *
 *                      For TLS 1.3 (in development), the notion of ciphersuite
 *                      only determines the suite of symmetric algorithms to be
 *                      used during and after the handshake, while key exchange
 *                      mechanisms are configured separately.
 *
 *                      In Mbed TLS, ciphersuites for both TLS 1.2 and TLS 1.3
 *                      are configured via this function. For users of TLS 1.3,
 *                      there will be separate API for the configuration of key
 *                      exchange mechanisms.
 *
 *                      The list of ciphersuites passed to this function may
 *                      contain a mixture of TLS 1.2 and TLS 1.3 ciphersuite
 *                      identifiers. This is useful if negotiation of TLS 1.3
 *                      should be attempted, but a fallback to TLS 1.2 would
 *                      be tolerated.
 *
 * \note                By default, the server chooses its preferred
 *                      ciphersuite among those that the client supports. If
 *                      mbedtls_ssl_conf_preference_order() is called to prefer
 *                      the client's preferences, the server instead chooses
 *                      the client's preferred ciphersuite among those that
 *                      the server supports.
 *
 * \warning             The ciphersuites array \p ciphersuites is not copied.
 *                      It must remain valid for the lifetime of the SSL
 *                      configuration \p conf.
 *
 * \param conf          The SSL configuration to modify.
 * \param ciphersuites  A 0-terminated list of IANA identifiers of supported
 *                      ciphersuites, accessible through \c MBEDTLS_TLS_XXX
 *                      and \c MBEDTLS_TLS1_3_XXX macros defined in
 *                      ssl_ciphersuites.h.
 */
void mbedtls_ssl_conf_ciphersuites(mbedtls_ssl_config *conf,
                                   const int *ciphersuites);

/**
 * \brief          Set the X.509 security profile used for verification
 *
 * \note           The restrictions are enforced for all certificates in the
 *                 chain. However, signatures in the handshake are not covered
 *                 by this setting but by \c mbedtls_ssl_conf_sig_hashes().
 *
 * \param conf     SSL configuration
 * \param profile  Profile to use
 */
void mbedtls_ssl_conf_cert_profile(mbedtls_ssl_config *conf,
                                   const mbedtls_x509_crt_profile *profile);

/**
 * \brief          Set the data required to verify peer certificate
 *
 * \note           See \c mbedtls_x509_crt_verify() for notes regarding the
 *                 parameters ca_chain (maps to trust_ca for that function)
 *                 and ca_crl.
 *
 * \param conf     SSL configuration
 * \param ca_chain trusted CA chain (meaning all fully trusted top-level CAs)
 * \param ca_crl   trusted CA CRLs
 */
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf,
                               mbedtls_x509_crt *ca_chain,
                               mbedtls_x509_crl *ca_crl);

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
/**
 * \brief          Set DN hints sent to client in CertificateRequest message
 *
 * \note           If not set, subject distinguished names (DNs) are taken
 *                 from \c mbedtls_ssl_conf_ca_chain()
 *                 or \c mbedtls_ssl_set_hs_ca_chain())
 *
 * \param conf     SSL configuration
 * \param crt      crt chain whose subject DNs are issuer DNs of client certs
 *                 from which the client should select client peer certificate.
 */
static inline
void mbedtls_ssl_conf_dn_hints(mbedtls_ssl_config *conf,
                               const mbedtls_x509_crt *crt)
{
    conf->MBEDTLS_PRIVATE(dn_hints) = crt;
}
#endif /* MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED */

#if defined(MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK)
/**
 * \brief          Set the trusted certificate callback.
 *
 *                 This API allows to register the set of trusted certificates
 *                 through a callback, instead of a linked list as configured
 *                 by mbedtls_ssl_conf_ca_chain().
 *
 *                 This is useful for example in contexts where a large number
 *                 of CAs are used, and the inefficiency of maintaining them
 *                 in a linked list cannot be tolerated. It is also useful when
 *                 the set of trusted CAs needs to be modified frequently.
 *
 *                 See the documentation of `mbedtls_x509_crt_ca_cb_t` for
 *                 more information.
 *
 * \param conf     The SSL configuration to register the callback with.
 * \param f_ca_cb  The trusted certificate callback to use when verifying
 *                 certificate chains.
 * \param p_ca_cb  The context to be passed to \p f_ca_cb (for example,
 *                 a reference to a trusted CA database).
 *
 * \note           This API is incompatible with mbedtls_ssl_conf_ca_chain():
 *                 Any call to this function overwrites the values set through
 *                 earlier calls to mbedtls_ssl_conf_ca_chain() or
 *                 mbedtls_ssl_conf_ca_cb().
 *
 * \note           This API is incompatible with CA indication in
 *                 CertificateRequest messages: A server-side SSL context which
 *                 is bound to an SSL configuration that uses a CA callback
 *                 configured via mbedtls_ssl_conf_ca_cb(), and which requires
 *                 client authentication, will send an empty CA list in the
 *                 corresponding CertificateRequest message.
 *
 * \note           This API is incompatible with mbedtls_ssl_set_hs_ca_chain():
 *                 If an SSL context is bound to an SSL configuration which uses
 *                 CA callbacks configured via mbedtls_ssl_conf_ca_cb(), then
 *                 calls to mbedtls_ssl_set_hs_ca_chain() have no effect.
 *
 * \note           The use of this API disables the use of restartable ECC
 *                 during X.509 CRT signature verification (but doesn't affect
 *                 other uses).
 *
 * \warning        This API is incompatible with the use of CRLs. Any call to
 *                 mbedtls_ssl_conf_ca_cb() unsets CRLs configured through
 *                 earlier calls to mbedtls_ssl_conf_ca_chain().
 *
 * \warning        In multi-threaded environments, the callback \p f_ca_cb
 *                 must be thread-safe, and it is the user's responsibility
 *                 to guarantee this (for example through a mutex
 *                 contained in the callback context pointed to by \p p_ca_cb).
 */
void mbedtls_ssl_conf_ca_cb(mbedtls_ssl_config *conf,
                            mbedtls_x509_crt_ca_cb_t f_ca_cb,
                            void *p_ca_cb);
#endif /* MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK */

/**
 * \brief          Set own certificate chain and private key
 *
 * \note           own_cert should contain in order from the bottom up your
 *                 certificate chain. The top certificate (self-signed)
 *                 can be omitted.
 *
 * \note           On server, this function can be called multiple times to
 *                 provision more than one cert/key pair (eg one ECDSA, one
 *                 RSA with SHA-256, one RSA with SHA-1). An adequate
 *                 certificate will be selected according to the client's
 *                 advertised capabilities. In case multiple certificates are
 *                 adequate, preference is given to the one set by the first
 *                 call to this function, then second, etc.
 *
 * \note           On client, only the first call has any effect. That is,
 *                 only one client certificate can be provisioned. The
 *                 server's preferences in its CertificateRequest message will
 *                 be ignored and our only cert will be sent regardless of
 *                 whether it matches those preferences - the server can then
 *                 decide what it wants to do with it.
 *
 * \note           The provided \p pk_key needs to match the public key in the
 *                 first certificate in \p own_cert, or all handshakes using
 *                 that certificate will fail. It is your responsibility
 *                 to ensure that; this function will not perform any check.
 *                 You may use mbedtls_pk_check_pair() in order to perform
 *                 this check yourself, but be aware that this function can
 *                 be computationally expensive on some key types.
 *
 * \param conf     SSL configuration
 * \param own_cert own public certificate chain
 * \param pk_key   own private key
 *
 * \return         0 on success or MBEDTLS_ERR_SSL_ALLOC_FAILED
 */
int mbedtls_ssl_conf_own_cert(mbedtls_ssl_config *conf,
                              mbedtls_x509_crt *own_cert,
                              mbedtls_pk_context *pk_key, MinTlsDiagnostics* diagnostics);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/**
 * \brief          Set the allowed curves in order of preference.
 *
 *                 On server: this only affects selection of the ECDHE curve;
 *                 the curves used for ECDH and ECDSA are determined by the
 *                 list of available certificates instead.
 *
 *                 On client: this affects the list of curves offered for any
 *                 use. The server can override our preference order.
 *
 *                 Both sides: limits the set of curves accepted for use in
 *                 ECDHE and in the peer's end-entity certificate.
 *
 * \deprecated     Superseded by mbedtls_ssl_conf_groups().
 *
 * \note           This has no influence on which curves are allowed inside the
 *                 certificate chains, see \c mbedtls_ssl_conf_cert_profile()
 *                 for that. For the end-entity certificate however, the key
 *                 will be accepted only if it is allowed both by this list
 *                 and by the cert profile.
 *
 * \note           This list should be ordered by decreasing preference
 *                 (preferred curve first).
 *
 * \note           The default list is the same set of curves that
 *                 #mbedtls_x509_crt_profile_default allows, plus
 *                 ECDHE-only curves selected according to the same criteria.
 *                 The order favors curves with the lowest resource usage.
 *
 * \note           New minor versions of Mbed TLS may extend this list,
 *                 for example if new curves are added to the library.
 *                 New minor versions of Mbed TLS will not remove items
 *                 from this list unless serious security concerns require it.
 *                 New minor versions of Mbed TLS may change the order in
 *                 keeping with the general principle of favoring the lowest
 *                 resource usage.
 *
 * \param conf     SSL configuration
 * \param curves   Ordered list of allowed curves,
 *                 terminated by MBEDTLS_ECP_DP_NONE.
 */
void MBEDTLS_DEPRECATED mbedtls_ssl_conf_curves(mbedtls_ssl_config *conf,
                                                const mbedtls_ecp_group_id *curves);
#endif /* MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Set the allowed groups in order of preference.
 *
 *                 On server: This only affects the choice of key agreement mechanism
 *
 *                 On client: this affects the list of groups offered for any
 *                 use. The server can override our preference order.
 *
 *                 Both sides: limits the set of groups accepted for use in
 *                 key sharing.
 *
 * \note           This function replaces the deprecated mbedtls_ssl_conf_curves(),
 *                 which only allows ECP curves to be configured.
 *
 * \note           The most recent invocation of either mbedtls_ssl_conf_curves()
 *                 or mbedtls_ssl_conf_groups() nullifies all previous invocations
 *                 of both.
 *
 * \note           This list should be ordered by decreasing preference
 *                 (preferred group first).
 *
 * \note           When this function is not called, a default list is used,
 *                 consisting of all supported curves at 255 bits and above,
 *                 and all supported finite fields at 2048 bits and above.
 *                 The order favors groups with the lowest resource usage.
 *
 * \note           New minor versions of Mbed TLS will not remove items
 *                 from the default list unless serious security concerns require it.
 *                 New minor versions of Mbed TLS may change the order in
 *                 keeping with the general principle of favoring the lowest
 *                 resource usage.
 *
 * \param conf     SSL configuration
 * \param groups   List of allowed groups ordered by preference, terminated by 0.
 *                 Must contain valid IANA NamedGroup IDs (provided via either an integer
 *                 or using MBEDTLS_TLS1_3_NAMED_GROUP_XXX macros).
 */
void mbedtls_ssl_conf_groups(mbedtls_ssl_config *conf,
                             const uint16_t *groups);

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED)
#if !defined(MBEDTLS_DEPRECATED_REMOVED) && defined(MBEDTLS_SSL_PROTO_TLS1_2)
/**
 * \brief          Set the allowed hashes for signatures during the handshake.
 *
 * \note           This only affects which hashes are offered and can be used
 *                 for signatures during the handshake. Hashes for message
 *                 authentication and the TLS PRF are controlled by the
 *                 ciphersuite, see \c mbedtls_ssl_conf_ciphersuites(). Hashes
 *                 used for certificate signature are controlled by the
 *                 verification profile, see \c mbedtls_ssl_conf_cert_profile().
 *
 * \deprecated     Superseded by mbedtls_ssl_conf_sig_algs().
 *
 * \note           This list should be ordered by decreasing preference
 *                 (preferred hash first).
 *
 * \note           By default, all supported hashes whose length is at least
 *                 256 bits are allowed. This is the same set as the default
 *                 for certificate verification
 *                 (#mbedtls_x509_crt_profile_default).
 *                 The preference order is currently unspecified and may
 *                 change in future versions.
 *
 * \note           New minor versions of Mbed TLS may extend this list,
 *                 for example if new curves are added to the library.
 *                 New minor versions of Mbed TLS will not remove items
 *                 from this list unless serious security concerns require it.
 *
 * \param conf     SSL configuration
 * \param hashes   Ordered list of allowed signature hashes,
 *                 terminated by \c MBEDTLS_MD_NONE.
 */
void MBEDTLS_DEPRECATED mbedtls_ssl_conf_sig_hashes(mbedtls_ssl_config *conf,
                                                    const int *hashes);
#endif /* !MBEDTLS_DEPRECATED_REMOVED && MBEDTLS_SSL_PROTO_TLS1_2 */

/**
 * \brief          Configure allowed signature algorithms for use in TLS key
 *                 exchange.
 *
 * \note           This only covers signature algorithms used in the key
 *                 exchange. To also enforce restrictions in certificate verification
 *                 refer to \c mbedtls_ssl_conf_cert_profile().
 *
 * \param conf     The SSL configuration to use.
 * \param sig_algs List of allowed IANA values for TLS 1.3 signature algorithms,
 *                 terminated by #MBEDTLS_TLS1_3_SIG_NONE. The list must remain
 *                 available throughout the lifetime of the conf object.
 *                 - For TLS 1.3, values of \c MBEDTLS_TLS1_3_SIG_XXXX should be
 *                   used.
 *                 - For TLS 1.2, values should be given as
 *                   "(HashAlgorithm << 8) | SignatureAlgorithm".
 */
void mbedtls_ssl_conf_sig_algs(mbedtls_ssl_config *conf,
                               const uint16_t *sig_algs);
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_CERT_ENABLED */

/**
 * \brief          Set or reset the hostname to check against the received
 *                 peer certificate. On a client, this also sets the
 *                 ServerName TLS extension, if that extension is enabled.
 *                 On a TLS 1.3 client, this also sets the server name in
 *                 the session resumption ticket, if that feature is enabled.
 *
 * \param ssl      SSL context
 * \param hostname The server hostname. This may be \c NULL to clear
 *                 the hostname.
 *
 * \note           Maximum hostname length #MBEDTLS_SSL_MAX_HOST_NAME_LEN.
 *
 * \note           If the hostname is \c NULL on a client, then the server
 *                 is not authenticated: it only needs to have a valid
 *                 certificate, not a certificate matching its name.
 *                 Therefore you should always call this function on a client,
 *                 unless the connection is set up to only allow
 *                 pre-shared keys, or in scenarios where server
 *                 impersonation is not a concern. See the documentation of
 *                 #MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
 *                 for more details.
 *
 * \return         0 if successful, #MBEDTLS_ERR_SSL_ALLOC_FAILED on
 *                 allocation failure, #MBEDTLS_ERR_SSL_BAD_INPUT_DATA on
 *                 too long input hostname.
 *
 *                 Hostname set to the one provided on success (cleared
 *                 when NULL). On allocation failure hostname is cleared.
 *                 On too long input failure, old hostname is unchanged.
 */
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Get the hostname that checked against the received
 *                 server certificate. It is used to set the ServerName
 *                 TLS extension, too, if that extension is enabled.
 *                 (client-side only)
 *
 * \param ssl      SSL context
 *
 * \return         const pointer to the hostname value
 */
static inline const char *mbedtls_ssl_get_hostname(mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(hostname);
}

/**
 * \brief          Retrieve SNI extension value for the current handshake.
 *                 Available in \c f_cert_cb of \c mbedtls_ssl_conf_cert_cb(),
 *                 this is the same value passed to \c f_sni callback of
 *                 \c mbedtls_ssl_conf_sni() and may be used instead of
 *                 \c mbedtls_ssl_conf_sni().
 *
 * \param ssl      SSL context
 * \param name_len pointer into which to store length of returned value.
 *                 0 if SNI extension is not present or not yet processed.
 *
 * \return         const pointer to SNI extension value.
 *                 - value is valid only when called in \c f_cert_cb
 *                   registered with \c mbedtls_ssl_conf_cert_cb().
 *                 - value is NULL if SNI extension is not present.
 *                 - value is not '\0'-terminated. Use \c name_len for len.
 *                 - value must not be freed.
 */
const unsigned char *mbedtls_ssl_get_hs_sni(mbedtls_ssl_context *ssl,
                                            size_t *name_len);

/**
 * \brief          Set own certificate and key for the current handshake
 *
 * \note           Same as \c mbedtls_ssl_conf_own_cert() but for use within
 *                 the SNI callback or the certificate selection callback.
 *
 * \note           Passing null \c own_cert clears the certificate list for
 *                 the current handshake.
 *
 * \param ssl      SSL context
 * \param own_cert own public certificate chain
 * \param pk_key   own private key
 *
 * \return         0 on success or MBEDTLS_ERR_SSL_ALLOC_FAILED
 */
int mbedtls_ssl_set_hs_own_cert(mbedtls_ssl_context *ssl,
                                mbedtls_x509_crt *own_cert,
                                mbedtls_pk_context *pk_key, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Set the data required to verify peer certificate for the
 *                 current handshake
 *
 * \note           Same as \c mbedtls_ssl_conf_ca_chain() but for use within
 *                 the SNI callback or the certificate selection callback.
 *
 * \param ssl      SSL context
 * \param ca_chain trusted CA chain (meaning all fully trusted top-level CAs)
 * \param ca_crl   trusted CA CRLs
 */
void mbedtls_ssl_set_hs_ca_chain(mbedtls_ssl_context *ssl,
                                 mbedtls_x509_crt *ca_chain,
                                 mbedtls_x509_crl *ca_crl);

#if defined(MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED)
/**
 * \brief          Set DN hints sent to client in CertificateRequest message
 *
 * \note           Same as \c mbedtls_ssl_conf_dn_hints() but for use within
 *                 the SNI callback or the certificate selection callback.
 *
 * \param ssl      SSL context
 * \param crt      crt chain whose subject DNs are issuer DNs of client certs
 *                 from which the client should select client peer certificate.
 */
void mbedtls_ssl_set_hs_dn_hints(mbedtls_ssl_context *ssl,
                                 const mbedtls_x509_crt *crt);
#endif /* MBEDTLS_KEY_EXCHANGE_CERT_REQ_ALLOWED_ENABLED */

/**
 * \brief          Set authmode for the current handshake.
 *
 * \note           Same as \c mbedtls_ssl_conf_authmode() but for use within
 *                 the SNI callback or the certificate selection callback.
 *
 * \param ssl      SSL context
 * \param authmode MBEDTLS_SSL_VERIFY_NONE, MBEDTLS_SSL_VERIFY_OPTIONAL or
 *                 MBEDTLS_SSL_VERIFY_REQUIRED
 */
void mbedtls_ssl_set_hs_authmode(mbedtls_ssl_context *ssl,
                                 int authmode);

/**
 * \brief          Set server side ServerName TLS extension callback
 *                 (optional, server-side only).
 *
 *                 If set, the ServerName callback is called whenever the
 *                 server receives a ServerName TLS extension from the client
 *                 during a handshake. The ServerName callback has the
 *                 following parameters: (void *parameter, mbedtls_ssl_context *ssl,
 *                 const unsigned char *hostname, size_t len). If a suitable
 *                 certificate is found, the callback must set the
 *                 certificate(s) and key(s) to use with \c
 *                 mbedtls_ssl_set_hs_own_cert() (can be called repeatedly),
 *                 and may optionally adjust the CA and associated CRL with \c
 *                 mbedtls_ssl_set_hs_ca_chain() as well as the client
 *                 authentication mode with \c mbedtls_ssl_set_hs_authmode(),
 *                 then must return 0. If no matching name is found, the
 *                 callback may return non-zero to abort the handshake.
 *
 * \param conf     SSL configuration
 * \param f_sni    verification function
 * \param p_sni    verification parameter
 */
void mbedtls_ssl_conf_sni(mbedtls_ssl_config *conf,
                          int (*f_sni)(void *, mbedtls_ssl_context *, const unsigned char *,
                                       size_t, MinTlsDiagnostics* diagnostics),
                          void *p_sni);

/**
 * \brief          Set the supported Application Layer Protocols.
 *
 * \param conf     SSL configuration
 * \param protos   Pointer to a NULL-terminated list of supported protocols,
 *                 in decreasing preference order. The pointer to the list is
 *                 recorded by the library for later reference as required, so
 *                 the lifetime of the table must be at least as long as the
 *                 lifetime of the SSL configuration structure.
 *
 * \return         0 on success, or MBEDTLS_ERR_SSL_BAD_INPUT_DATA.
 */
int mbedtls_ssl_conf_alpn_protocols(mbedtls_ssl_config *conf, const char **protos, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Get the name of the negotiated Application Layer Protocol.
 *                 This function should be called after the handshake is
 *                 completed.
 *
 * \param ssl      SSL context
 *
 * \return         Protocol name, or NULL if no protocol was negotiated.
 */
const char *mbedtls_ssl_get_alpn_protocol(const mbedtls_ssl_context *ssl);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/**
 * \brief          Set the maximum supported version sent from the client side
 *                 and/or accepted at the server side.
 *
 *                 See also the documentation of mbedtls_ssl_conf_min_version().
 *
 * \note           This ignores ciphersuites from higher versions.
 *
 * \note           This function is deprecated and has been replaced by
 *                 \c mbedtls_ssl_conf_max_tls_version().
 *
 * \param conf     SSL configuration
 * \param major    Major version number (#MBEDTLS_SSL_MAJOR_VERSION_3)
 * \param minor    Minor version number
 *                 (#MBEDTLS_SSL_MINOR_VERSION_3 for (D)TLS 1.2,
 *                 #MBEDTLS_SSL_MINOR_VERSION_4 for TLS 1.3)
 */
void MBEDTLS_DEPRECATED mbedtls_ssl_conf_max_version(mbedtls_ssl_config *conf, int major,
                                                     int minor);
#endif /* MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Set the maximum supported version sent from the client side
 *                 and/or accepted at the server side.
 *
 * \note           After the handshake, you can call
 *                 mbedtls_ssl_get_version_number() to see what version was
 *                 negotiated.
 *
 * \param conf         SSL configuration
 * \param tls_version  TLS protocol version number (\c mbedtls_ssl_protocol_version)
 *                     (#MBEDTLS_SSL_VERSION_UNKNOWN is not valid)
 */
static inline void mbedtls_ssl_conf_max_tls_version(mbedtls_ssl_config *conf,
                                                    mbedtls_ssl_protocol_version tls_version)
{
    conf->MBEDTLS_PRIVATE(max_tls_version) = tls_version;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
/**
 * \brief          Set the minimum accepted SSL/TLS protocol version
 *
 * \note           By default, all supported versions are accepted.
 *                 Future versions of the library may disable older
 *                 protocol versions by default if they become deprecated.
 *
 * \note           The following versions are supported (if enabled at
 *                 compile time):
 *                 - (D)TLS 1.2: \p major = #MBEDTLS_SSL_MAJOR_VERSION_3,
 *                   \p minor = #MBEDTLS_SSL_MINOR_VERSION_3
 *                 - TLS 1.3: \p major = #MBEDTLS_SSL_MAJOR_VERSION_3,
 *                   \p minor = #MBEDTLS_SSL_MINOR_VERSION_4
 *
 *                 Note that the numbers in the constant names are the
 *                 TLS internal protocol numbers, and the minor versions
 *                 differ by one from the human-readable versions!
 *
 * \note           Input outside of the SSL_MAX_XXXXX_VERSION and
 *                 SSL_MIN_XXXXX_VERSION range is ignored.
 *
 * \note           After the handshake, you can call
 *                 mbedtls_ssl_get_version_number() to see what version was
 *                 negotiated.
 *
 * \note           This function is deprecated and has been replaced by
 *                 \c mbedtls_ssl_conf_min_tls_version().
 *
 * \param conf     SSL configuration
 * \param major    Major version number (#MBEDTLS_SSL_MAJOR_VERSION_3)
 * \param minor    Minor version number
 *                 (#MBEDTLS_SSL_MINOR_VERSION_3 for (D)TLS 1.2,
 *                 #MBEDTLS_SSL_MINOR_VERSION_4 for TLS 1.3)
 */
void MBEDTLS_DEPRECATED mbedtls_ssl_conf_min_version(mbedtls_ssl_config *conf, int major,
                                                     int minor);
#endif /* MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Set the minimum supported version sent from the client side
 *                 and/or accepted at the server side.
 *
 * \note           After the handshake, you can call
 *                 mbedtls_ssl_get_version_number() to see what version was
 *                 negotiated.
 *
 * \param conf         SSL configuration
 * \param tls_version  TLS protocol version number (\c mbedtls_ssl_protocol_version)
 *                     (#MBEDTLS_SSL_VERSION_UNKNOWN is not valid)
 */
static inline void mbedtls_ssl_conf_min_tls_version(mbedtls_ssl_config *conf,
                                                    mbedtls_ssl_protocol_version tls_version)
{
    conf->MBEDTLS_PRIVATE(min_tls_version) = tls_version;
}

/**
 * \brief           Enable or disable Extended Master Secret negotiation.
 *                  (Default: MBEDTLS_SSL_EXTENDED_MS_ENABLED)
 *
 * \note            This should always be enabled, it is a security fix to the
 *                  protocol, and should not cause any interoperability issue
 *                  (used only if the peer supports it too).
 *
 * \param conf      SSL configuration
 * \param ems       MBEDTLS_SSL_EXTENDED_MS_ENABLED or MBEDTLS_SSL_EXTENDED_MS_DISABLED
 */
void mbedtls_ssl_conf_extended_master_secret(mbedtls_ssl_config *conf, char ems);

/**
 * \brief          Prevent or allow legacy renegotiation.
 *                 (Default: MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION)
 *
 *                 MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION allows connections to
 *                 be established even if the peer does not support
 *                 secure renegotiation, but does not allow renegotiation
 *                 to take place if not secure.
 *                 (Interoperable and secure option)
 *
 *                 MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION allows renegotiations
 *                 with non-upgraded peers. Allowing legacy renegotiation
 *                 makes the connection vulnerable to specific man in the
 *                 middle attacks. (See RFC 5746)
 *                 (Most interoperable and least secure option)
 *
 *                 MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE breaks off connections
 *                 if peer does not support secure renegotiation. Results
 *                 in interoperability issues with non-upgraded peers
 *                 that do not support renegotiation altogether.
 *                 (Most secure option, interoperability issues)
 *
 * \param conf     SSL configuration
 * \param allow_legacy  Prevent or allow (SSL_NO_LEGACY_RENEGOTIATION,
 *                                        SSL_ALLOW_LEGACY_RENEGOTIATION or
 *                                        MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE)
 */
void mbedtls_ssl_conf_legacy_renegotiation(mbedtls_ssl_config *conf, int allow_legacy);

/**
 * \brief          Check if there is data already read from the
 *                 underlying transport but not yet processed.
 *
 * \param ssl      SSL context
 *
 * \return         0 if nothing's pending, 1 otherwise.
 *
 * \note           This is different in purpose and behaviour from
 *                 \c mbedtls_ssl_get_bytes_avail in that it considers
 *                 any kind of unprocessed data, not only unread
 *                 application data. If \c mbedtls_ssl_get_bytes
 *                 returns a non-zero value, this function will
 *                 also signal pending data, but the converse does
 *                 not hold. For example, in DTLS there might be
 *                 further records waiting to be processed from
 *                 the current underlying transport's datagram.
 *
 * \note           If this function returns 1 (data pending), this
 *                 does not imply that a subsequent call to
 *                 \c mbedtls_ssl_read will provide any data;
 *                 e.g., the unprocessed data might turn out
 *                 to be an alert or a handshake message.
 *
 * \note           This function is useful in the following situation:
 *                 If the SSL/TLS module successfully returns from an
 *                 operation - e.g. a handshake or an application record
 *                 read - and you're awaiting incoming data next, you
 *                 must not immediately idle on the underlying transport
 *                 to have data ready, but you need to check the value
 *                 of this function first. The reason is that the desired
 *                 data might already be read but not yet processed.
 *                 If, in contrast, a previous call to the SSL/TLS module
 *                 returned MBEDTLS_ERR_SSL_WANT_READ, it is not necessary
 *                 to call this function, as the latter error code entails
 *                 that all internal data has been processed.
 *
 */
int mbedtls_ssl_check_pending(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the number of application data bytes
 *                 remaining to be read from the current record.
 *
 * \param ssl      SSL context
 *
 * \return         How many bytes are available in the application
 *                 data record read buffer.
 *
 * \note           When working over a datagram transport, this is
 *                 useful to detect the current datagram's boundary
 *                 in case \c mbedtls_ssl_read has written the maximal
 *                 amount of data fitting into the input buffer.
 *
 */
size_t mbedtls_ssl_get_bytes_avail(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the result of the certificate verification
 *
 * \param ssl      The SSL context to use.
 *
 * \return         \c 0 if the certificate verification was successful.
 * \return         \c -1u if the result is not available. This may happen
 *                 e.g. if the handshake aborts early, or a verification
 *                 callback returned a fatal error.
 * \return         A bitwise combination of \c MBEDTLS_X509_BADCERT_XXX
 *                 and \c MBEDTLS_X509_BADCRL_XXX failure flags; see x509.h.
 */
uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the id of the current ciphersuite
 *
 * \param ssl      SSL context
 *
 * \return         a ciphersuite id
 */
int mbedtls_ssl_get_ciphersuite_id_from_ssl(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the name of the current ciphersuite
 *
 * \param ssl      SSL context
 *
 * \return         a string containing the ciphersuite name
 */
const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the (D)TLS protocol version negotiated in the
 *                 given connection.
 *
 * \note           If you call this function too early during the initial
 *                 handshake, before the two sides have agreed on a version,
 *                 this function returns #MBEDTLS_SSL_VERSION_UNKNOWN.
 *
 * \param ssl      The SSL context to query.
 * \return         The negotiated protocol version.
 */
static inline mbedtls_ssl_protocol_version mbedtls_ssl_get_version_number(
    const mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(tls_version);
}

/**
 * \brief          Return the current TLS version
 *
 * \param ssl      SSL context
 *
 * \return         a string containing the TLS version
 */
const char *mbedtls_ssl_get_version(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the (maximum) number of bytes added by the record
 *                 layer: header + encryption/MAC overhead (inc. padding)
 *
 * \param ssl      SSL context
 *
 * \return         Current maximum record expansion in bytes
 */
int mbedtls_ssl_get_record_expansion(const mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Return the current maximum outgoing record payload in bytes.
 *
 * \note           The logic to determine the maximum outgoing record payload is
 *                 version-specific. It takes into account various factors, such as
 *                 the mbedtls_config.h setting \c MBEDTLS_SSL_OUT_CONTENT_LEN, extensions
 *                 such as the max fragment length or record size limit extension if
 *                 used, and for DTLS the path MTU as configured and current
 *                 record expansion.
 *
 * \note           With DTLS, \c mbedtls_ssl_write() will return an error if
 *                 called with a larger length value.
 *                 With TLS, \c mbedtls_ssl_write() will fragment the input if
 *                 necessary and return the number of bytes written; it is up
 *                 to the caller to call \c mbedtls_ssl_write() again in
 *                 order to send the remaining bytes if any.
 *
 * \sa             mbedtls_ssl_get_max_out_record_payload()
 * \sa             mbedtls_ssl_get_record_expansion()
 *
 * \param ssl      SSL context
 *
 * \return         Current maximum payload for an outgoing record,
 *                 or a negative error code.
 */
int mbedtls_ssl_get_max_out_record_payload(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the current maximum incoming record payload in bytes.
 *
 * \note           The logic to determine the maximum incoming record payload is
 *                 version-specific. It takes into account various factors, such as
 *                 the mbedtls_config.h setting \c MBEDTLS_SSL_IN_CONTENT_LEN, extensions
 *                 such as the max fragment length extension or record size limit
 *                 extension if used, and the current record expansion.
 *
 * \sa             mbedtls_ssl_set_mtu()
 * \sa             mbedtls_ssl_get_max_in_record_payload()
 * \sa             mbedtls_ssl_get_record_expansion()
 *
 * \param ssl      SSL context
 *
 * \return         Current maximum payload for an incoming record,
 *                 or a negative error code.
 */
int mbedtls_ssl_get_max_in_record_payload(const mbedtls_ssl_context *ssl);

/**
 * \brief          Return the peer certificate from the current connection.
 *
 * \param  ssl     The SSL context to use. This must be initialized and setup.
 *
 * \return         The current peer certificate, if available.
 *                 The returned certificate is owned by the SSL context and
 *                 is valid only until the next call to the SSL API.
 * \return         \c NULL if no peer certificate is available. This might
 *                 be because the chosen ciphersuite doesn't use CRTs
 *                 (PSK-based ciphersuites, for example), or because
 *                 #MBEDTLS_SSL_KEEP_PEER_CERTIFICATE has been disabled,
 *                 allowing the stack to free the peer's CRT to save memory.
 *
 * \note           For one-time inspection of the peer's certificate during
 *                 the handshake, consider registering an X.509 CRT verification
 *                 callback through mbedtls_ssl_conf_verify() instead of calling
 *                 this function. Using mbedtls_ssl_conf_verify() also comes at
 *                 the benefit of allowing you to influence the verification
 *                 process, for example by masking expected and tolerated
 *                 verification failures.
 *
 * \warning        You must not use the pointer returned by this function
 *                 after any further call to the SSL API, including
 *                 mbedtls_ssl_read() and mbedtls_ssl_write(); this is
 *                 because the pointer might change during renegotiation,
 *                 which happens transparently to the user.
 *                 If you want to use the certificate across API calls,
 *                 you must make a copy.
 */
const mbedtls_x509_crt *mbedtls_ssl_get_peer_cert(const mbedtls_ssl_context *ssl);

/**
 * \brief          Export a session in order to resume it later.
 *
 * \param ssl      The SSL context representing the connection for which to
 *                 to export a session structure for later resumption.
 * \param session  The target structure in which to store the exported session.
 *                 This must have been initialized with mbedtls_ssl_session_init()
 *                 but otherwise be unused.
 *
 * \note           This function can handle a variety of mechanisms for session
 *                 resumption: For TLS 1.2, both session ID-based resumption and
 *                 ticket-based resumption will be considered. For TLS 1.3,
 *                 sessions equate to tickets, and if session tickets are
 *                 enabled (see #MBEDTLS_SSL_SESSION_TICKETS configuration
 *                 option), this function exports the last received ticket and
 *                 the exported session may be used to resume the TLS 1.3
 *                 session. If session tickets are disabled, exported sessions
 *                 cannot be used to resume a TLS 1.3 session.
 *
 * \return         \c 0 if successful. In this case, \p session can be used for
 *                 session resumption by passing it to mbedtls_ssl_set_session(),
 *                 and serialized for storage via mbedtls_ssl_session_save().
 * \return         Another negative error code on other kinds of failure.
 *
 * \sa             mbedtls_ssl_set_session()
 * \sa             mbedtls_ssl_session_save()
 */
int mbedtls_ssl_get_session(const mbedtls_ssl_context *ssl,
                            mbedtls_ssl_session *session, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Perform the SSL handshake
 *
 * \param ssl      SSL context
 *
 * \return         \c 0 if successful.
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED if DTLS is in use
 *                 and the client did not demonstrate reachability yet - in
 *                 this case you must stop using the context (see below).
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received. See the
 *                 documentation of mbedtls_ssl_read() for more information
 *                 about this error code.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 \c 0,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           If DTLS is in use, then you may choose to handle
 *                 #MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED specially for logging
 *                 purposes, as it is an expected return value rather than an
 *                 actual error, but you still need to reset/free the context.
 *
 * \note           Remarks regarding event-driven DTLS:
 *                 If the function returns #MBEDTLS_ERR_SSL_WANT_READ, no datagram
 *                 from the underlying transport layer is currently being processed,
 *                 and it is safe to idle until the timer or the underlying transport
 *                 signal a new event. This is not true for a successful handshake,
 *                 in which case the datagram of the underlying transport that is
 *                 currently being processed might or might not contain further
 *                 DTLS records.
 *
 * \note           If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                 subsystem must have been initialized by calling
 *                 psa_crypto_init() before calling this function.
 *                 Otherwise, the handshake may call psa_crypto_init()
 *                 if a negotiation involving TLS 1.3 takes place (this may
 *                 be the case even if TLS 1.3 is offered but eventually
 *                 not selected).
 *
 * \note           In TLS, reception of fragmented handshake messages is
 *                 supported with some limitations (those limitations do
 *                 not apply to DTLS, where defragmentation is fully
 *                 supported):
 *                 - The first fragment of a handshake message must be
 *                   at least 4 bytes long.
 *                 - Non-handshake records must not be interleaved between
 *                   the fragments of a handshake message. (This is permitted
 *                   in TLS 1.2 but not in TLS 1.3, but Mbed TLS rejects it
 *                   even in TLS 1.2.)
 */
int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          After calling mbedtls_ssl_handshake() to start the SSL
 *                 handshake you can call this function to check whether the
 *                 handshake is over for a given SSL context. This function
 *                 should be also used to determine when to stop calling
 *                 mbedtls_handshake_step() for that context.
 *
 * \param ssl      SSL context
 *
 * \return         \c 1 if handshake is over, \c 0 if it is still ongoing.
 */
static inline int mbedtls_ssl_is_handshake_over(mbedtls_ssl_context *ssl)
{
    return ssl->MBEDTLS_PRIVATE(state) >= MBEDTLS_SSL_HANDSHAKE_OVER;
}

/**
 * \brief          Perform a single step of the SSL handshake
 *
 * \note           The state of the context (ssl->state) will be at
 *                 the next state after this function returns \c 0. Do not
 *                 call this function if mbedtls_ssl_is_handshake_over()
 *                 returns \c 1.
 *
 * \warning        Whilst in the past you may have used direct access to the
 *                 context state (ssl->state) in order to ascertain when to
 *                 stop calling this function and although you can still do
 *                 so with something like ssl->MBEDTLS_PRIVATE(state) or by
 *                 defining MBEDTLS_ALLOW_PRIVATE_ACCESS, this is now
 *                 considered deprecated and could be broken in any future
 *                 release. If you still find you have good reason for such
 *                 direct access, then please do contact the team to explain
 *                 this (raise an issue or post to the mailing list), so that
 *                 we can add a solution to your problem that will be
 *                 guaranteed to work in the future.
 *
 * \param ssl      SSL context
 *
 * \return         See mbedtls_ssl_handshake().
 *
 * \warning        If this function returns something other than \c 0,
 *                 #MBEDTLS_ERR_SSL_WANT_READ, #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA, you must stop using
 *                 the SSL context for reading or writing, and either free it
 *                 or call \c mbedtls_ssl_session_reset() on it before
 *                 re-using it for a new connection; the current connection
 *                 must be closed.
 */
int mbedtls_ssl_handshake_step(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Read at most 'len' application data bytes
 *
 * \param ssl      SSL context
 * \param buf      buffer that will hold the data
 * \param len      maximum number of bytes to read
 *
 * \return         The (positive) number of bytes read if successful.
 * \return         \c 0 if the read end of the underlying transport was closed
 *                 without sending a CloseNotify beforehand, which might happen
 *                 because of various reasons (internal error of an underlying
 *                 stack, non-conformant peer not sending a CloseNotify and
 *                 such) - in this case you must stop using the context
 *                 (see below).
 * \return         #MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY if the underlying
 *                 transport is still functional, but the peer has
 *                 acknowledged to not send anything anymore.
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_CLIENT_RECONNECT if we're at the server
 *                 side of a DTLS connection and the client is initiating a
 *                 new connection using the same source port. See below.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received.
 *                 This error code is only returned on the client side. It is
 *                 only returned if handling of TLS 1.3 NewSessionTicket
 *                 messages has been enabled through
 *                 mbedtls_ssl_conf_tls13_enable_signal_new_session_tickets().
 *                 This error code indicates that a TLS 1.3 NewSessionTicket
 *                 message has been received and parsed successfully by the
 *                 client. The ticket data can be retrieved from the SSL
 *                 context by calling mbedtls_ssl_get_session(). It remains
 *                 available until the next call to mbedtls_ssl_read().
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 a positive value,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CLIENT_RECONNECT or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           When this function returns #MBEDTLS_ERR_SSL_CLIENT_RECONNECT
 *                 (which can only happen server-side), it means that a client
 *                 is initiating a new connection using the same source port.
 *                 You can either treat that as a connection close and wait
 *                 for the client to resend a ClientHello, or directly
 *                 continue with \c mbedtls_ssl_handshake() with the same
 *                 context (as it has been reset internally). Either way, you
 *                 must make sure this is seen by the application as a new
 *                 connection: application state, if any, should be reset, and
 *                 most importantly the identity of the client must be checked
 *                 again. WARNING: not validating the identity of the client
 *                 again, or not transmitting the new identity to the
 *                 application layer, would allow authentication bypass!
 *
 * \note           Remarks regarding event-driven DTLS:
 *                 - If the function returns #MBEDTLS_ERR_SSL_WANT_READ, no datagram
 *                   from the underlying transport layer is currently being processed,
 *                   and it is safe to idle until the timer or the underlying transport
 *                   signal a new event.
 *                 - This function may return MBEDTLS_ERR_SSL_WANT_READ even if data was
 *                   initially available on the underlying transport, as this data may have
 *                   been only e.g. duplicated messages or a renegotiation request.
 *                   Therefore, you must be prepared to receive MBEDTLS_ERR_SSL_WANT_READ even
 *                   when reacting to an incoming-data event from the underlying transport.
 *                 - On success, the datagram of the underlying transport that is currently
 *                   being processed may contain further DTLS records. You should call
 *                   \c mbedtls_ssl_check_pending to check for remaining records.
 *
 */
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Try to write exactly 'len' application data bytes
 *
 * \warning        This function will do partial writes in some cases. If the
 *                 return value is non-negative but less than length, the
 *                 function must be called again with updated arguments:
 *                 buf + ret, len - ret (if ret is the return value) until
 *                 it returns a value equal to the last 'len' argument.
 *
 * \param ssl      SSL context
 * \param buf      buffer holding the data
 * \param len      how many bytes must be written
 *
 * \return         The (non-negative) number of bytes actually written if
 *                 successful (may be less than \p len).
 * \return         #MBEDTLS_ERR_SSL_WANT_READ or #MBEDTLS_ERR_SSL_WANT_WRITE
 *                 if the handshake is incomplete and waiting for data to
 *                 be available for reading from or writing to the underlying
 *                 transport - in this case you must call this function again
 *                 when the underlying transport is ready for the operation.
 * \return         #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS if an asynchronous
 *                 operation is in progress (see
 *                 mbedtls_ssl_conf_async_private_cb()) - in this case you
 *                 must call this function again when the operation is ready.
 * \return         #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS if a cryptographic
 *                 operation is in progress (see mbedtls_ecp_set_max_ops()) -
 *                 in this case you must call this function again to complete
 *                 the handshake when you're done attending other tasks.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET if a TLS 1.3
 *                 NewSessionTicket message has been received. See the
 *                 documentation of mbedtls_ssl_read() for more information
 *                 about this error code.
 * \return         #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA if early data, as
 *                 defined in RFC 8446 (TLS 1.3 specification), has been
 *                 received as part of the handshake. This is server specific
 *                 and may occur only if the early data feature has been
 *                 enabled on server (see mbedtls_ssl_conf_early_data()
 *                 documentation). You must call mbedtls_ssl_read_early_data()
 *                 to read the early data before resuming the handshake.
 * \return         Another SSL error code - in this case you must stop using
 *                 the context (see below).
 *
 * \warning        If this function returns something other than
 *                 a non-negative value,
 *                 #MBEDTLS_ERR_SSL_WANT_READ,
 *                 #MBEDTLS_ERR_SSL_WANT_WRITE,
 *                 #MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS,
 *                 #MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET or
 *                 #MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA,
 *                 you must stop using the SSL context for reading or writing,
 *                 and either free it or call \c mbedtls_ssl_session_reset()
 *                 on it before re-using it for a new connection; the current
 *                 connection must be closed.
 *
 * \note           When this function returns #MBEDTLS_ERR_SSL_WANT_WRITE/READ,
 *                 it must be called later with the *same* arguments,
 *                 until it returns a value greater than or equal to 0. When
 *                 the function returns #MBEDTLS_ERR_SSL_WANT_WRITE there may be
 *                 some partial data in the output buffer, however this is not
 *                 yet sent.
 *
 * \note           If the requested length is greater than the maximum
 *                 fragment length (either the built-in limit or the one set
 *                 or negotiated with the peer), then:
 *                 - with TLS, less bytes than requested are written.
 *                 - with DTLS, MBEDTLS_ERR_SSL_BAD_INPUT_DATA is returned.
 *                 \c mbedtls_ssl_get_max_out_record_payload() may be used to
 *                 query the active maximum fragment length.
 *
 * \note           Attempting to write 0 bytes will result in an empty TLS
 *                 application record being sent.
 */
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Send an alert message
 *
 * \param ssl       SSL context
 * \param level     The alert level of the message
 *                  (MBEDTLS_SSL_ALERT_LEVEL_WARNING or MBEDTLS_SSL_ALERT_LEVEL_FATAL)
 * \param message   The alert message (SSL_ALERT_MSG_*)
 *
 * \return          0 if successful, or a specific SSL error code.
 *
 * \note           If this function returns something other than 0 or
 *                 MBEDTLS_ERR_SSL_WANT_READ/WRITE, you must stop using
 *                 the SSL context for reading or writing, and either free it or
 *                 call \c mbedtls_ssl_session_reset() on it before re-using it
 *                 for a new connection; the current connection must be closed.
 */
int mbedtls_ssl_send_alert_message(mbedtls_ssl_context *ssl,
                                   unsigned char level,
                                   unsigned char message, MinTlsDiagnostics* diagnostics);
/**
 * \brief          Notify the peer that the connection is being closed
 *
 * \param ssl      SSL context
 *
 * \return          0 if successful, or a specific SSL error code.
 *
 * \note           If this function returns something other than 0 or
 *                 MBEDTLS_ERR_SSL_WANT_READ/WRITE, you must stop using
 *                 the SSL context for reading or writing, and either free it or
 *                 call \c mbedtls_ssl_session_reset() on it before re-using it
 *                 for a new connection; the current connection must be closed.
 */
int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl, MinTlsDiagnostics* diagnostics);

/**
 * \brief          Free referenced items in an SSL context and clear memory
 *
 * \param ssl      SSL context
 */
void mbedtls_ssl_free(mbedtls_ssl_context *ssl);

/**
 * \brief          Initialize an SSL configuration context
 *                 Just makes the context ready for
 *                 mbedtls_ssl_config_defaults() or mbedtls_ssl_config_free().
 *
 * \note           You need to call mbedtls_ssl_config_defaults() unless you
 *                 manually set all of the relevant fields yourself.
 *
 * \param conf     SSL configuration context
 */
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);

/**
 * \brief          Load reasonable default SSL configuration values.
 *                 (You need to call mbedtls_ssl_config_init() first.)
 *
 * \param conf     SSL configuration context
 * \param endpoint MBEDTLS_SSL_IS_CLIENT or MBEDTLS_SSL_IS_SERVER
 * \param transport MBEDTLS_SSL_TRANSPORT_STREAM for TLS, or
 *                  MBEDTLS_SSL_TRANSPORT_DATAGRAM for DTLS
 * \param preset   a MBEDTLS_SSL_PRESET_XXX value
 *
 * \note           See \c mbedtls_ssl_conf_transport() for notes on DTLS.
 *
 * \return         0 if successful, or
 *                 MBEDTLS_ERR_XXX_ALLOC_FAILED on memory allocation error.
 */
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,
                                int endpoint, int transport, int preset);

/**
 * \brief          Free an SSL configuration context
 *
 * \param conf     SSL configuration context
 */
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);

/**
 * \brief          Initialize SSL session structure
 *
 * \param session  SSL session
 */
void mbedtls_ssl_session_init(mbedtls_ssl_session *session);

/**
 * \brief          Free referenced items in an SSL session including the
 *                 peer certificate and clear memory
 *
 * \note           A session object can be freed even if the SSL context
 *                 that was used to retrieve the session is still in use.
 *
 * \param session  SSL session
 */
void mbedtls_ssl_session_free(mbedtls_ssl_session *session);

/**
 * \brief          TLS-PRF function for key derivation.
 *
 * \param prf      The tls_prf type function type to be used.
 * \param secret   Secret for the key derivation function.
 * \param slen     Length of the secret.
 * \param label    String label for the key derivation function,
 *                 terminated with null character.
 * \param random   Random bytes.
 * \param rlen     Length of the random bytes buffer.
 * \param dstbuf   The buffer holding the derived key.
 * \param dlen     Length of the output buffer.
 *
 * \return         0 on success. An SSL specific error on failure.
 */
int  mbedtls_ssl_tls_prf(const mbedtls_tls_prf_types prf,
                         const unsigned char *secret, size_t slen,
                         const char *label,
                         const unsigned char *random, size_t rlen,
                         unsigned char *dstbuf, size_t dlen, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_SSL_KEYING_MATERIAL_EXPORT)
/* Maximum value for key_len in mbedtls_ssl_export_keying material. Depending on the TLS
 * version and the negotiated ciphersuite, larger keys could in principle be exported,
 * but for simplicity, we define one limit that works in all cases. TLS 1.3 with SHA256
 * has the strictest limit: 255 blocks of SHA256 output, or 8160 bytes. */
#define MBEDTLS_SSL_EXPORT_MAX_KEY_LEN 8160

/**
 * \brief             TLS-Exporter to derive shared symmetric keys between server and client.
 *
 * \param ssl         SSL context from which to export keys. Must have finished the handshake.
 * \param out         Output buffer of length at least key_len bytes.
 * \param key_len     Length of the key to generate in bytes, must be at most
 *                    MBEDTLS_SSL_EXPORT_MAX_KEY_LEN (8160).
 * \param label       Label for which to generate the key of length label_len.
 * \param label_len   Length of label in bytes. Must be at most 249 in TLS 1.3.
 * \param context     Context of the key. Can be NULL if context_len or use_context is 0.
 * \param context_len Length of context. Must be < 2^16 in TLS 1.2.
 * \param use_context Indicates if a context should be used in deriving the key.
 *
 * \note TLS 1.2 makes a distinction between a 0-length context and no context.
 *       This is why the use_context argument exists. TLS 1.3 does not make
 *       this distinction. If use_context is 0 and TLS 1.3 is used, context and
 *       context_len are ignored and a 0-length context is used.
 *
 * \return            0 on success.
 * \return            MBEDTLS_ERR_SSL_BAD_INPUT_DATA if the handshake is not yet completed.
 * \return            An SSL-specific error on failure.
 */
int mbedtls_ssl_export_keying_material(mbedtls_ssl_context *ssl,
                                       uint8_t *out, const size_t key_len,
                                       const char *label, const size_t label_len,
                                       const unsigned char *context, const size_t context_len,
                                       const int use_context, MinTlsDiagnostics* diagnostics);
#endif
#ifdef __cplusplus
}
#endif

#endif /* ssl.h */
