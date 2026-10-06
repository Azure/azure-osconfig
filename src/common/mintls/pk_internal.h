/**
 * \file pk_internal.h
 *
 * \brief Public Key abstraction layer: internal (i.e. library only) functions
 *        and definitions.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: library/pk_internal.h.
 */
#ifndef MBEDTLS_PK_INTERNAL_H
#define MBEDTLS_PK_INTERNAL_H

#include "pk.h"

#include "ecp.h"

/* Headers/footers for PEM files */
#define PEM_BEGIN_PUBLIC_KEY    "-----BEGIN PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY      "-----END PUBLIC KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_RSA   "-----BEGIN RSA PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_RSA     "-----END RSA PRIVATE KEY-----"
#define PEM_BEGIN_PUBLIC_KEY_RSA     "-----BEGIN RSA PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY_RSA     "-----END RSA PUBLIC KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_EC    "-----BEGIN EC PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_EC      "-----END EC PRIVATE KEY-----"
#define PEM_BEGIN_PRIVATE_KEY_PKCS8 "-----BEGIN PRIVATE KEY-----"
#define PEM_END_PRIVATE_KEY_PKCS8   "-----END PRIVATE KEY-----"
#define PEM_BEGIN_ENCRYPTED_PRIVATE_KEY_PKCS8 "-----BEGIN ENCRYPTED PRIVATE KEY-----"
#define PEM_END_ENCRYPTED_PRIVATE_KEY_PKCS8   "-----END ENCRYPTED PRIVATE KEY-----"

/*
 * We're trying to statisfy two kinds of users:
 * - those who don't want to use the heap;
 * - those who can't afford large stack buffers.
 *
 * The current compromise is that if ECC is the only key type supported in PK,
 * then we export keys on the stack, and otherwise we use the heap.
 *
 * RSA can either be used directly or indirectly via opaque keys if enabled.
 * (RSA_ALT is not relevant here as we can't export from such contexts.)
 */

#if defined(PK_EXPORT_KEYS_ON_THE_STACK)
/* We know for ECC, pubkey are longer than privkeys, but double check.
 * Also, take the maximum size of legacy and PSA, as PSA might be disabled. */
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_PSA_MAX_EC_PUBKEY_LENGTH
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH
#endif
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_ECP_MAX_BYTES
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_ECP_MAX_BYTES
#endif
#if PK_EXPORT_KEY_STACK_BUFFER_SIZE < MBEDTLS_ECP_MAX_PT_LEN
#undef PK_EXPORT_KEY_STACK_BUFFER_SIZE
#define PK_EXPORT_KEY_STACK_BUFFER_SIZE  MBEDTLS_ECP_MAX_PT_LEN
#endif
#endif

/**
 * Public function mbedtls_pk_ec() can be used to get direct access to the
 * wrapped ecp_keypair structure pointed to the pk_ctx. However this is not
 * ideal because it bypasses the PK module on the control of its internal
 * structure (pk_context) fields.
 * For backward compatibility we keep mbedtls_pk_ec() when ECP_C is defined, but
 * we provide 2 very similar functions when only ECP_LIGHT is enabled and not
 * ECP_C.
 * These variants embed the "ro" or "rw" keywords in their name to make the
 * usage of the returned pointer explicit. Of course the returned value is
 * const or non-const accordingly.
 */
static inline const mbedtls_ecp_keypair *mbedtls_pk_ec_ro(const mbedtls_pk_context pk)
{
    switch (mbedtls_pk_get_type(&pk)) {
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
        case MBEDTLS_PK_ECDSA:
            return (const mbedtls_ecp_keypair *) (pk).MBEDTLS_PRIVATE(pk_ctx);
        default:
            return NULL;
    }
}

static inline mbedtls_ecp_keypair *mbedtls_pk_ec_rw(const mbedtls_pk_context pk)
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

static inline mbedtls_ecp_group_id mbedtls_pk_get_ec_group_id(const mbedtls_pk_context *pk)
{
    mbedtls_ecp_group_id id;

    {
        id = mbedtls_pk_ec_ro(*pk)->grp.id;
    }

    return id;
}

/* Helper for Montgomery curves */

#define MBEDTLS_PK_IS_RFC8410_GROUP_ID(id)  \
    ((id == MBEDTLS_ECP_DP_CURVE25519) || (id == MBEDTLS_ECP_DP_CURVE448))

static inline int mbedtls_pk_is_rfc8410(const mbedtls_pk_context *pk)
{
    mbedtls_ecp_group_id id = mbedtls_pk_get_ec_group_id(pk);

    return MBEDTLS_PK_IS_RFC8410_GROUP_ID(id);
}

/*
 * Set the group used by this key.
 *
 * [in/out] pk: in: must have been pk_setup() to an ECC type
 *              out: will have group (curve) information set
 * [in] grp_in: a supported group ID (not NONE)
 */
int mbedtls_pk_ecc_set_group(mbedtls_pk_context *pk, mbedtls_ecp_group_id grp_id);

/*
 * Set the private key material
 *
 * [in/out] pk: in: must have the group set already, see mbedtls_pk_ecc_set_group().
 *              out: will have the private key set.
 * [in] key, key_len: the raw private key (no ASN.1 wrapping).
 */
int mbedtls_pk_ecc_set_key(mbedtls_pk_context *pk, unsigned char *key, size_t key_len);

/*
 * Set the public key.
 *
 * [in/out] pk: in: must have its group set, see mbedtls_pk_ecc_set_group().
 *              out: will have the public key set.
 * [in] pub, pub_len: the raw public key (an ECPoint).
 *
 * Return:
 * - 0 on success;
 * - MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE if the format is potentially valid
 *   but not supported;
 * - another error code otherwise.
 */
int mbedtls_pk_ecc_set_pubkey(mbedtls_pk_context *pk, const unsigned char *pub, size_t pub_len);

/*
 * Derive a public key from its private counterpart.
 * Computationally intensive, only use when public key is not available.
 *
 * [in/out] pk: in: must have the private key set, see mbedtls_pk_ecc_set_key().
 *              out: will have the public key set.
 * [in] prv, prv_len: the raw private key (see note below).
 * [in] f_rng, p_rng: RNG function and context.
 *
 * Note: the private key information is always available from pk,
 * however for convenience the serialized version is also passed,
 * as it's available at each calling site, and useful in some configs
 * (as otherwise we would have to re-serialize it from the pk context).
 *
 * There are three implementations of this function:
 * 1. MBEDTLS_PK_USE_PSA_EC_DATA,
 * 2. MBEDTLS_USE_PSA_CRYPTO but not MBEDTLS_PK_USE_PSA_EC_DATA,
 * 3. not MBEDTLS_USE_PSA_CRYPTO.
 */
int mbedtls_pk_ecc_set_pubkey_from_prv(mbedtls_pk_context *pk,
                                       const unsigned char *prv, size_t prv_len,
                                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

/* Helper for (deterministic) ECDSA */
#define MBEDTLS_PK_PSA_ALG_ECDSA_MAYBE_DET  PSA_ALG_ECDSA

#endif /* MBEDTLS_PK_INTERNAL_H */
