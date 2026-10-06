// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file pk.h
 *
 * \brief Public Key abstraction layer
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: include/mbedtls/pk.h.
 */

#ifndef MBEDTLS_PK_H
#define MBEDTLS_PK_H
#include "private_access.h"

#include "build_info.h"

#include "md.h"

#include "rsa.h"

#include "ecp.h"

#include "ecdsa.h"

/** Memory allocation failed. */
#define MBEDTLS_ERR_PK_ALLOC_FAILED        -0x3F80
/** Type mismatch, eg attempt to encrypt with an ECDSA key */
#define MBEDTLS_ERR_PK_TYPE_MISMATCH       -0x3F00
/** Bad input parameters to function. */
#define MBEDTLS_ERR_PK_BAD_INPUT_DATA      -0x3E80
/** Read/write of file failed. */
#define MBEDTLS_ERR_PK_FILE_IO_ERROR       -0x3E00
/** Unsupported key version */
#define MBEDTLS_ERR_PK_KEY_INVALID_VERSION -0x3D80
/** Invalid key tag or value. */
#define MBEDTLS_ERR_PK_KEY_INVALID_FORMAT  -0x3D00
/** Key algorithm is unsupported (only RSA and EC are supported). */
#define MBEDTLS_ERR_PK_UNKNOWN_PK_ALG      -0x3C80
/** Private key password can't be empty. */
#define MBEDTLS_ERR_PK_PASSWORD_REQUIRED   -0x3C00
/** Given private key password does not allow for correct decryption. */
#define MBEDTLS_ERR_PK_PASSWORD_MISMATCH   -0x3B80
/** The pubkey tag or value is invalid (only RSA and EC are supported). */
#define MBEDTLS_ERR_PK_INVALID_PUBKEY      -0x3B00
/** The algorithm tag or value is invalid. */
#define MBEDTLS_ERR_PK_INVALID_ALG         -0x3A80
/** Elliptic curve is unsupported (only NIST curves are supported). */
#define MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE -0x3A00
/** Unavailable feature, e.g. RSA disabled for RSA key. */
#define MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE -0x3980
/** The buffer contains a valid signature followed by more data. */
#define MBEDTLS_ERR_PK_SIG_LEN_MISMATCH    -0x3900
/** The output buffer is too small. */
#define MBEDTLS_ERR_PK_BUFFER_TOO_SMALL    -0x3880

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Public key types
 */
typedef enum {
    MBEDTLS_PK_NONE=0,
    MBEDTLS_PK_RSA,
    MBEDTLS_PK_ECKEY,
    MBEDTLS_PK_ECKEY_DH,
    MBEDTLS_PK_ECDSA,
    MBEDTLS_PK_RSA_ALT,
    MBEDTLS_PK_RSASSA_PSS,
    MBEDTLS_PK_OPAQUE,
} mbedtls_pk_type_t;

/**
 * \brief           Options for RSASSA-PSS signature verification.
 *                  See \c mbedtls_rsa_rsassa_pss_verify_ext()
 */
typedef struct mbedtls_pk_rsassa_pss_options {
    /** The digest to use for MGF1 in PSS.
     *
     * \note When #MBEDTLS_USE_PSA_CRYPTO is enabled and #MBEDTLS_RSA_C is
     *       disabled, this must be equal to the \c md_alg argument passed
     *       to mbedtls_pk_verify_ext(). In a future version of the library,
     *       this constraint may apply whenever #MBEDTLS_USE_PSA_CRYPTO is
     *       enabled regardless of the status of #MBEDTLS_RSA_C.
     */
    mbedtls_md_type_t mgf1_hash_id;

    /** The expected length of the salt, in bytes. This may be
     * #MBEDTLS_RSA_SALT_LEN_ANY to accept any salt length.
     *
     * \note When #MBEDTLS_USE_PSA_CRYPTO is enabled, only
     *       #MBEDTLS_RSA_SALT_LEN_ANY is valid. Any other value may be
     *       ignored (allowing any salt length).
     */
    int expected_salt_len;

} mbedtls_pk_rsassa_pss_options;

/**
 * \brief           Maximum size of a signature made by mbedtls_pk_sign().
 */
/* We need to set MBEDTLS_PK_SIGNATURE_MAX_SIZE to the maximum signature
 * size among the supported signature types. Do it by starting at 0,
 * then incrementally increasing to be large enough for each supported
 * signature mechanism.
 *
 * The resulting value can be 0, for example if MBEDTLS_ECDH_C is enabled
 * (which allows the pk module to be included) but neither MBEDTLS_ECDSA_C
 * nor MBEDTLS_RSA_C nor any opaque signature mechanism (PSA or RSA_ALT).
 */
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE 0

#if (defined(MBEDTLS_RSA_C) || defined(MBEDTLS_PK_RSA_ALT_SUPPORT)) && \
    MBEDTLS_MPI_MAX_SIZE > MBEDTLS_PK_SIGNATURE_MAX_SIZE
/* For RSA, the signature can be as large as the bignum module allows.
 * For RSA_ALT, the signature size is not necessarily tied to what the
 * bignum module can do, but in the absence of any specific setting,
 * we use that (rsa_alt_sign_wrap in library/pk_wrap.h will check). */
#undef MBEDTLS_PK_SIGNATURE_MAX_SIZE
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE MBEDTLS_MPI_MAX_SIZE
#endif

#if defined(MBEDTLS_ECDSA_C) &&                                 \
    MBEDTLS_ECDSA_MAX_LEN > MBEDTLS_PK_SIGNATURE_MAX_SIZE
/* For ECDSA, the ecdsa module exports a constant for the maximum
 * signature size. */
#undef MBEDTLS_PK_SIGNATURE_MAX_SIZE
#define MBEDTLS_PK_SIGNATURE_MAX_SIZE MBEDTLS_ECDSA_MAX_LEN
#endif

/* Internal helper to define which fields in the pk_context structure below
 * should be used for EC keys: legacy ecp_keypair or the raw (PSA friendly)
 * format. It should be noted that this only affects how data is stored, not
 * which functions are used for various operations. The overall picture looks
 * like this:
 * - if USE_PSA is not defined and ECP_C is defined then use ecp_keypair data
 *   structure and legacy functions
 * - if USE_PSA is defined and
 *     - if ECP_C then use ecp_keypair structure, convert data to a PSA friendly
 *       format and use PSA functions
 *     - if !ECP_C then use new raw data and PSA functions directly.
 *
 * The main reason for the "intermediate" (USE_PSA + ECP_C) above is that as long
 * as ECP_C is defined mbedtls_pk_ec() gives the user a read/write access to the
 * ecp_keypair structure inside the pk_context so they can modify it using
 * ECP functions which are not under PK module's control.
 */

/**
 * \brief           Types for interfacing with the debug module
 */
typedef enum {
    MBEDTLS_PK_DEBUG_NONE = 0,
    MBEDTLS_PK_DEBUG_MPI,
    MBEDTLS_PK_DEBUG_ECP,
    MBEDTLS_PK_DEBUG_PSA_EC,
} mbedtls_pk_debug_type;

/**
 * \brief           Item to send to the debug module
 */
typedef struct mbedtls_pk_debug_item {
    mbedtls_pk_debug_type MBEDTLS_PRIVATE(type);
    const char *MBEDTLS_PRIVATE(name);
    void *MBEDTLS_PRIVATE(value);
} mbedtls_pk_debug_item;

/** Maximum number of item send for debugging, plus 1 */
#define MBEDTLS_PK_DEBUG_MAX_ITEMS 3

/**
 * \brief           Public key information and operations
 *
 * \note        The library does not support custom pk info structures,
 *              only built-in structures returned by
 *              mbedtls_cipher_info_from_type().
 */
typedef struct mbedtls_pk_info_t mbedtls_pk_info_t;

#define MBEDTLS_PK_MAX_EC_PUBKEY_RAW_LEN \
    PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)
/**
 * \brief           Public key container
 */
typedef struct mbedtls_pk_context {
    /** Method table */
    const mbedtls_pk_info_t *MBEDTLS_PRIVATE(pk_info);
    /** Underlying type-specific key context */
    void *MBEDTLS_PRIVATE(pk_ctx);

    /* The following field is used to store the ID of a private key in the
     * following cases:
     * - opaque key when MBEDTLS_USE_PSA_CRYPTO is defined
     * - normal key when MBEDTLS_PK_USE_PSA_EC_DATA is defined. In this case:
     *    - the pk_ctx above is not not used to store the private key anymore.
     *      Actually that field not populated at all in this case because also
     *      the public key will be stored in raw format as explained below
     *    - this ID is used for all private key operations (ex: sign, check
     *      key pair, key write, etc) using PSA functions
     *
     * Note: this private key storing solution only affects EC keys, not the
     *       other ones. The latters still use the pk_ctx to store their own
     *       context. */
    /* The following fields are meant for storing the public key in raw format
     * which is handy for:
     * - easily importing it into the PSA context
     * - reducing the ECP module dependencies in the PK one.
     *
     * When MBEDTLS_PK_USE_PSA_EC_DATA is enabled:
     * - the pk_ctx above is not used anymore for storing the public key
     *   inside the ecp_keypair structure
     * - the following fields are used for all public key operations: signature
     *   verify, key pair check and key write.
     * - For a key pair, priv_id contains the private key. For a public key,
     *   priv_id is null.
     * Of course, when MBEDTLS_PK_USE_PSA_EC_DATA is not enabled, the legacy
     * ecp_keypair structure is used for storing the public key and performing
     * all the operations.
     *
     * Note: This new public key storing solution only works for EC keys, not
     *       other ones. The latters still use pk_ctx to store their own
     *       context.
     */
} mbedtls_pk_context;

/* Now we can declare functions that take a pointer to that */
typedef void mbedtls_pk_restart_ctx;

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/**
 * \brief           Types for RSA-alt abstraction
 */
typedef int (*mbedtls_pk_rsa_alt_decrypt_func)(void *ctx, size_t *olen,
                                               const unsigned char *input, unsigned char *output,
                                               size_t output_max_len, MinTlsDiagnostics* diagnostics);
typedef int (*mbedtls_pk_rsa_alt_sign_func)(void *ctx,
                                            mbedtls_f_rng_t *f_rng,
                                            void *p_rng,
                                            mbedtls_md_type_t md_alg, unsigned int hashlen,
                                            const unsigned char *hash, unsigned char *sig, MinTlsDiagnostics* diagnostics);
typedef size_t (*mbedtls_pk_rsa_alt_key_len_func)(void *ctx);
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/**
 * \brief           Return information associated with the given PK type
 *
 * \param pk_type   PK type to search for.
 *
 * \return          The PK info associated with the type or NULL if not found.
 */
const mbedtls_pk_info_t *mbedtls_pk_info_from_type(mbedtls_pk_type_t pk_type);

/**
 * \brief           Initialize a #mbedtls_pk_context (as NONE).
 *
 * \param ctx       The context to initialize.
 *                  This must not be \c NULL.
 */
void mbedtls_pk_init(mbedtls_pk_context *ctx);

/**
 * \brief           Free the components of a #mbedtls_pk_context.
 *
 * \param ctx       The context to clear. It must have been initialized.
 *                  If this is \c NULL, this function does nothing.
 *
 * \note            For contexts that have been set up with
 *                  mbedtls_pk_setup_opaque(), this does not free the underlying
 *                  PSA key and you still need to call psa_destroy_key()
 *                  independently if you want to destroy that key.
 */
void mbedtls_pk_free(mbedtls_pk_context *ctx);

/**
 * \brief           Initialize a PK context with the information given
 *                  and allocates the type-specific PK subcontext.
 *
 * \param ctx       Context to initialize. It must not have been set
 *                  up yet (type #MBEDTLS_PK_NONE).
 * \param info      Information to use
 *
 * \return          0 on success,
 *                  MBEDTLS_ERR_PK_BAD_INPUT_DATA on invalid input,
 *                  MBEDTLS_ERR_PK_ALLOC_FAILED on allocation failure.
 *
 * \note            For contexts holding an RSA-alt key, use
 *                  \c mbedtls_pk_setup_rsa_alt() instead.
 */
int mbedtls_pk_setup(mbedtls_pk_context *ctx, const mbedtls_pk_info_t *info, MinTlsDiagnostics* diagnostics);

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/**
 * \brief           Initialize an RSA-alt context
 *
 * \param ctx       Context to initialize. It must not have been set
 *                  up yet (type #MBEDTLS_PK_NONE).
 * \param key       RSA key pointer
 * \param decrypt_func  Decryption function
 * \param sign_func     Signing function
 * \param key_len_func  Function returning key length in bytes
 *
 * \return          0 on success, or MBEDTLS_ERR_PK_BAD_INPUT_DATA if the
 *                  context wasn't already initialized as RSA_ALT.
 *
 * \note            This function replaces \c mbedtls_pk_setup() for RSA-alt.
 */
int mbedtls_pk_setup_rsa_alt(mbedtls_pk_context *ctx, void *key,
                             mbedtls_pk_rsa_alt_decrypt_func decrypt_func,
                             mbedtls_pk_rsa_alt_sign_func sign_func,
                             mbedtls_pk_rsa_alt_key_len_func key_len_func, MinTlsDiagnostics* diagnostics);
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/**
 * \brief           Get the size in bits of the underlying key
 *
 * \param ctx       The context to query. It must have been initialized.
 *
 * \return          Key size in bits, or 0 on error
 */
size_t mbedtls_pk_get_bitlen(const mbedtls_pk_context *ctx);

/**
 * \brief           Get the length in bytes of the underlying key
 *
 * \param ctx       The context to query. It must have been initialized.
 *
 * \return          Key length in bytes, or 0 on error
 */
static inline size_t mbedtls_pk_get_len(const mbedtls_pk_context *ctx)
{
    return (mbedtls_pk_get_bitlen(ctx) + 7) / 8;
}

/**
 * \brief           Tell if a context can do the operation given by type
 *
 * \param ctx       The context to query. It must have been initialized.
 * \param type      The desired type.
 *
 * \return          1 if the context can do operations on the given type.
 * \return          0 if the context cannot do the operations on the given
 *                  type. This is always the case for a context that has
 *                  been initialized but not set up, or that has been
 *                  cleared with mbedtls_pk_free().
 */
int mbedtls_pk_can_do(const mbedtls_pk_context *ctx, mbedtls_pk_type_t type);

/**
 * \brief           Verify signature (including padding if relevant).
 *
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used.
 *                  This can be #MBEDTLS_MD_NONE if the signature algorithm
 *                  does not rely on a hash algorithm (non-deterministic
 *                  ECDSA, RSA PKCS#1 v1.5).
 *                  For PKCS#1 v1.5, if \p md_alg is #MBEDTLS_MD_NONE, then
 *                  \p hash is the DigestInfo structure used by RFC 8017
 *                  &sect;9.2 steps 3&ndash;6. If \p md_alg is a valid hash
 *                  algorithm then \p hash is the digest itself, and this
 *                  function calculates the DigestInfo encoding internally.
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 *
 * \note            For keys of type #MBEDTLS_PK_RSA, the signature algorithm is
 *                  either PKCS#1 v1.5 or PSS (accepting any salt length),
 *                  depending on the padding mode in the underlying RSA context.
 *                  For a pk object constructed by parsing, this is PKCS#1 v1.5
 *                  by default. Use mbedtls_pk_verify_ext() to explicitly select
 *                  a different algorithm.
 *
 * \return          0 on success (signature is valid),
 *                  #MBEDTLS_ERR_PK_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig but its length is less than \p sig_len,
 *                  or a specific error code.
 */
int mbedtls_pk_verify(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                      const unsigned char *hash, size_t hash_len,
                      const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Restartable version of \c mbedtls_pk_verify()
 *
 * \note            Performs the same job as \c mbedtls_pk_verify(), but can
 *                  return early and restart according to the limit set with
 *                  \c mbedtls_ecp_set_max_ops() to reduce blocking for ECC
 *                  operations. For RSA, same as \c mbedtls_pk_verify().
 *
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length or 0 (see notes)
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 * \param rs_ctx    Restart context (NULL to disable restart)
 *
 * \return          See \c mbedtls_pk_verify(), or
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 */
int mbedtls_pk_verify_restartable(mbedtls_pk_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hash_len,
                                  const unsigned char *sig, size_t sig_len,
                                  mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Verify signature, with options.
 *                  (Includes verification of the padding depending on type.)
 *
 * \param type      Signature type (inc. possible padding type) to verify
 * \param options   Pointer to type-specific options, or NULL
 * \param ctx       The PK context to use. It must have been set up.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length or 0 (see notes)
 * \param sig       Signature to verify
 * \param sig_len   Signature length
 *
 * \return          0 on success (signature is valid),
 *                  #MBEDTLS_ERR_PK_TYPE_MISMATCH if the PK context can't be
 *                  used for this type of signatures,
 *                  #MBEDTLS_ERR_PK_SIG_LEN_MISMATCH if there is a valid
 *                  signature in \p sig but its length is less than \p sig_len,
 *                  or a specific error code.
 *
 * \note            If hash_len is 0, then the length associated with md_alg
 *                  is used instead, or an error returned if it is invalid.
 *
 * \note            md_alg may be MBEDTLS_MD_NONE, only if hash_len != 0
 *
 * \note            If type is MBEDTLS_PK_RSASSA_PSS, then options must point
 *                  to a mbedtls_pk_rsassa_pss_options structure,
 *                  otherwise it must be NULL. Note that if
 *                  #MBEDTLS_USE_PSA_CRYPTO is defined, the salt length is not
 *                  verified as PSA_ALG_RSA_PSS_ANY_SALT is used.
 */
int mbedtls_pk_verify_ext(mbedtls_pk_type_t type, const void *options,
                          mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                          const unsigned char *hash, size_t hash_len,
                          const unsigned char *sig, size_t sig_len, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Make signature, including padding if relevant.
 *
 * \param ctx       The PK context to use. It must have been set up
 *                  with a private key.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Place to write the signature.
 *                  It must have enough room for the signature.
 *                  #MBEDTLS_PK_SIGNATURE_MAX_SIZE is always enough.
 *                  You may use a smaller buffer if it is large enough
 *                  given the key type.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param sig_len   On successful return,
 *                  the number of bytes written to \p sig.
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 *
 * \note            For keys of type #MBEDTLS_PK_RSA, the signature algorithm is
 *                  either PKCS#1 v1.5 or PSS (using the largest possible salt
 *                  length up to the hash length), depending on the padding mode
 *                  in the underlying RSA context. For a pk object constructed
 *                  by parsing, this is PKCS#1 v1.5 by default. Use
 *                  mbedtls_pk_sign_ext() to explicitly select a different
 *                  algorithm.
 *
 * \return          0 on success, or a specific error code.
 *
 * \note            For RSA, md_alg may be MBEDTLS_MD_NONE if hash_len != 0.
 *                  For ECDSA, md_alg may never be MBEDTLS_MD_NONE.
 */
int mbedtls_pk_sign(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                    const unsigned char *hash, size_t hash_len,
                    unsigned char *sig, size_t sig_size, size_t *sig_len,
                    mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Make signature given a signature type.
 *
 * \param pk_type   Signature type.
 * \param ctx       The PK context to use. It must have been set up
 *                  with a private key.
 * \param md_alg    Hash algorithm used (see notes)
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Place to write the signature.
 *                  It must have enough room for the signature.
 *                  #MBEDTLS_PK_SIGNATURE_MAX_SIZE is always enough.
 *                  You may use a smaller buffer if it is large enough
 *                  given the key type.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param sig_len   On successful return,
 *                  the number of bytes written to \p sig.
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 *
 * \return          0 on success, or a specific error code.
 *
 * \note            When \p pk_type is #MBEDTLS_PK_RSASSA_PSS,
 *                  see #PSA_ALG_RSA_PSS for a description of PSS options used.
 *
 * \note            For RSA, md_alg may be MBEDTLS_MD_NONE if hash_len != 0.
 *                  For ECDSA, md_alg may never be MBEDTLS_MD_NONE.
 *
 */
int mbedtls_pk_sign_ext(mbedtls_pk_type_t pk_type,
                        mbedtls_pk_context *ctx,
                        mbedtls_md_type_t md_alg,
                        const unsigned char *hash, size_t hash_len,
                        unsigned char *sig, size_t sig_size, size_t *sig_len,
                        mbedtls_f_rng_t *f_rng,
                        void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Restartable version of \c mbedtls_pk_sign()
 *
 * \note            Performs the same job as \c mbedtls_pk_sign(), but can
 *                  return early and restart according to the limit set with
 *                  \c mbedtls_ecp_set_max_ops() to reduce blocking for ECC
 *                  operations. For RSA, same as \c mbedtls_pk_sign().
 *
 * \param ctx       The PK context to use. It must have been set up
 *                  with a private key.
 * \param md_alg    Hash algorithm used (see notes for mbedtls_pk_sign())
 * \param hash      Hash of the message to sign
 * \param hash_len  Hash length
 * \param sig       Place to write the signature.
 *                  It must have enough room for the signature.
 *                  #MBEDTLS_PK_SIGNATURE_MAX_SIZE is always enough.
 *                  You may use a smaller buffer if it is large enough
 *                  given the key type.
 * \param sig_size  The size of the \p sig buffer in bytes.
 * \param sig_len   On successful return,
 *                  the number of bytes written to \p sig.
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 * \param rs_ctx    Restart context (NULL to disable restart)
 *
 * \return          See \c mbedtls_pk_sign().
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 */
int mbedtls_pk_sign_restartable(mbedtls_pk_context *ctx,
                                mbedtls_md_type_t md_alg,
                                const unsigned char *hash, size_t hash_len,
                                unsigned char *sig, size_t sig_size, size_t *sig_len,
                                mbedtls_f_rng_t *f_rng, void *p_rng,
                                mbedtls_pk_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Decrypt message (including padding if relevant).
 *
 * \warning         When using PKCS#1 v1.5 (see note below), this is an
 *                  inherently dangerous function (CWE-242) and the return value
 *                  is sensitive, see mbedtls_rsa_rsaes_pkcs1_v15_decrypt().
 *
 * \note            For keys of type #MBEDTLS_PK_RSA, the encryption algorithm is
 *                  either PKCS#1 v1.5 or OAEP, depending on the padding mode in
 *                  the underlying RSA context. For a pk object constructed by
 *                  parsing, this is PKCS#1 v1.5 by default.
 *
 * \param ctx       The PK context to use. It must have been set up
 *                  with a private key.
 * \param input     Input to decrypt
 * \param ilen      Input size
 * \param output    Decrypted output
 * \param olen      Decrypted message length
 * \param osize     Size of the output buffer
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 *
 * \return          0 on success, or a specific error code.
 */
int mbedtls_pk_decrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Encrypt message (including padding if relevant).
 *
 * \param ctx       The PK context to use. It must have been set up.
 * \param input     Message to encrypt
 * \param ilen      Message size
 * \param output    Encrypted output
 * \param olen      Encrypted output length
 * \param osize     Size of the output buffer
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 *
 * \note            For keys of type #MBEDTLS_PK_RSA, the signature algorithm is
 *                  either PKCS#1 v1.5 or OAEP, depending on the padding mode in
 *                  the underlying RSA context. For a pk object constructed by
 *                  parsing, this is PKCS#1 v1.5 by default.
 *
 * \note            \p f_rng is used for padding generation.
 *
 * \return          0 on success, or a specific error code.
 */
int mbedtls_pk_encrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Check if a public-private pair of keys matches.
 *
 * \param pub       Context holding a public key.
 * \param prv       Context holding a private (and public) key.
 * \param f_rng     RNG function, must not be \c NULL.
 * \param p_rng     RNG parameter
 *
 * \return          \c 0 on success (keys were checked and match each other).
 * \return          #MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE if the keys could not
 *                  be checked - in that case they may or may not match.
 * \return          #MBEDTLS_ERR_PK_BAD_INPUT_DATA if a context is invalid.
 * \return          Another non-zero value if the keys do not match.
 */
int mbedtls_pk_check_pair(const mbedtls_pk_context *pub,
                          const mbedtls_pk_context *prv,
                          mbedtls_f_rng_t *f_rng,
                          void *p_rng, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Export debug information
 *
 * \param ctx       The PK context to use. It must have been initialized.
 * \param items     Place to write debug items
 *
 * \return          0 on success or MBEDTLS_ERR_PK_BAD_INPUT_DATA
 */
int mbedtls_pk_debug(const mbedtls_pk_context *ctx, mbedtls_pk_debug_item *items, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Access the type name
 *
 * \param ctx       The PK context to use. It must have been initialized.
 *
 * \return          Type name on success, or "invalid PK"
 */
const char *mbedtls_pk_get_name(const mbedtls_pk_context *ctx);

/**
 * \brief           Get the key type
 *
 * \param ctx       The PK context to use. It must have been initialized.
 *
 * \return          Type on success.
 * \return          #MBEDTLS_PK_NONE for a context that has not been set up.
 */
mbedtls_pk_type_t mbedtls_pk_get_type(const mbedtls_pk_context *ctx);

/**
 * Quick access to an RSA context inside a PK context.
 *
 * \warning This function can only be used when the type of the context, as
 * returned by mbedtls_pk_get_type(), is #MBEDTLS_PK_RSA.
 * Ensuring that is the caller's responsibility.
 * Alternatively, you can check whether this function returns NULL.
 *
 * \return The internal RSA context held by the PK context, or NULL.
 */
static inline mbedtls_rsa_context *mbedtls_pk_rsa(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_RSA:
            return (mbedtls_rsa_context *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

/**
 * Quick access to an EC context inside a PK context.
 *
 * \warning This function can only be used when the type of the context, as
 * returned by mbedtls_pk_get_type(), is #MBEDTLS_PK_ECKEY,
 * #MBEDTLS_PK_ECKEY_DH, or #MBEDTLS_PK_ECDSA.
 * Ensuring that is the caller's responsibility.
 * Alternatively, you can check whether this function returns NULL.
 *
 * \return The internal EC context held by the PK context, or NULL.
 */
static inline mbedtls_ecp_keypair *mbedtls_pk_ec(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
        case MBEDTLS_PK_ECDSA:
            return (mbedtls_ecp_keypair *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

/** \ingroup pk_module */
/**
 * \brief           Parse a private key in PEM or DER format
 *
 * \note            If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                  subsystem must have been initialized by calling
 *                  psa_crypto_init() before calling this function.
 *
 * \param ctx       The PK context to fill. It must have been initialized
 *                  but not set up.
 * \param key       Input buffer to parse.
 *                  The buffer must contain the input exactly, with no
 *                  extra trailing material. For PEM, the buffer must
 *                  contain a null-terminated string.
 * \param keylen    Size of \b key in bytes.
 *                  For PEM data, this includes the terminating null byte,
 *                  so \p keylen must be equal to `strlen(key) + 1`.
 * \param pwd       Optional password for decryption.
 *                  Pass \c NULL if expecting a non-encrypted key.
 *                  Pass a string of \p pwdlen bytes if expecting an encrypted
 *                  key; a non-encrypted key will also be accepted.
 *                  The empty password is not supported.
 * \param pwdlen    Size of the password in bytes.
 *                  Ignored if \p pwd is \c NULL.
 * \param f_rng     RNG function, must not be \c NULL. Used for blinding.
 * \param p_rng     RNG parameter
 *
 * \note            On entry, ctx must be empty, either freshly initialised
 *                  with mbedtls_pk_init() or reset with mbedtls_pk_free(). If you need a
 *                  specific key type, check the result with mbedtls_pk_can_do().
 *
 * \note            The key is also checked for correctness.
 *
 * \return          0 if successful, or a specific PK or PEM error code
 */
int mbedtls_pk_parse_key(mbedtls_pk_context *ctx,
                         const unsigned char *key, size_t keylen,
                         const unsigned char *pwd, size_t pwdlen,
                         mbedtls_f_rng_t *f_rng, void *p_rng, MinTlsDiagnostics* diagnostics);

/** \ingroup pk_module */
/**
 * \brief           Parse a public key in PEM or DER format
 *
 * \note            If #MBEDTLS_USE_PSA_CRYPTO is enabled, the PSA crypto
 *                  subsystem must have been initialized by calling
 *                  psa_crypto_init() before calling this function.
 *
 * \param ctx       The PK context to fill. It must have been initialized
 *                  but not set up.
 * \param key       Input buffer to parse.
 *                  The buffer must contain the input exactly, with no
 *                  extra trailing material. For PEM, the buffer must
 *                  contain a null-terminated string.
 * \param keylen    Size of \b key in bytes.
 *                  For PEM data, this includes the terminating null byte,
 *                  so \p keylen must be equal to `strlen(key) + 1`.
 *
 * \note            On entry, ctx must be empty, either freshly initialised
 *                  with mbedtls_pk_init() or reset with mbedtls_pk_free(). If you need a
 *                  specific key type, check the result with mbedtls_pk_can_do().
 *
 * \note            For compressed points, see #MBEDTLS_ECP_PF_COMPRESSED for
 *                  limitations.
 *
 * \note            The key is also checked for correctness.
 *
 * \return          0 if successful, or a specific PK or PEM error code
 */
int mbedtls_pk_parse_public_key(mbedtls_pk_context *ctx,
                                const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics);

/*
 * WARNING: Low-level functions. You probably do not want to use these unless
 *          you are certain you do ;)
 */

/**
 * \brief           Parse a SubjectPublicKeyInfo DER structure
 *
 * \param p         the position in the ASN.1 data
 * \param end       end of the buffer
 * \param pk        The PK context to fill. It must have been initialized
 *                  but not set up.
 *
 * \return          0 if successful, or a specific PK error code
 */
int mbedtls_pk_parse_subpubkey(unsigned char **p, const unsigned char *end,
                               mbedtls_pk_context *pk, MinTlsDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_PK_H */
