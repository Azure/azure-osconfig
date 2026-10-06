/*
 *  ECC setters for PK.
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: library/pk_ecc.c.
 */

#include "common.h"

#include "pk.h"
#include "error.h"
#include "ecp.h"
#include "pk_internal.h"

int mbedtls_pk_ecc_set_group(mbedtls_pk_context *pk, mbedtls_ecp_group_id grp_id)
{
    mbedtls_ecp_keypair *ecp = mbedtls_pk_ec_rw(*pk);

    /* grp may already be initialized; if so, make sure IDs match */
    if (mbedtls_pk_ec_ro(*pk)->grp.id != MBEDTLS_ECP_DP_NONE &&
        mbedtls_pk_ec_ro(*pk)->grp.id != grp_id) {
        return MBEDTLS_ERR_PK_KEY_INVALID_FORMAT;
    }

    /* set group */
    return mbedtls_ecp_group_load(&(ecp->grp), grp_id);
}

int mbedtls_pk_ecc_set_key(mbedtls_pk_context *pk, unsigned char *key, size_t key_len)
{

    mbedtls_ecp_keypair *eck = mbedtls_pk_ec_rw(*pk);
    int ret = mbedtls_ecp_read_key(eck->grp.id, eck, key, key_len);
    if (ret != 0) {
        return MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret);
    }
    return 0;
}

int mbedtls_pk_ecc_set_pubkey_from_prv(mbedtls_pk_context *pk,
                                       const unsigned char *prv, size_t prv_len,
                                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{

    (void) prv;
    (void) prv_len;

    mbedtls_ecp_keypair *eck = (mbedtls_ecp_keypair *) pk->pk_ctx;
    return mbedtls_ecp_mul(&eck->grp, &eck->Q, &eck->d, &eck->grp.G, f_rng, p_rng);

}

int mbedtls_pk_ecc_set_pubkey(mbedtls_pk_context *pk, const unsigned char *pub, size_t pub_len)
{

    int ret;
    mbedtls_ecp_keypair *ec_key = (mbedtls_ecp_keypair *) pk->pk_ctx;
    ret = mbedtls_ecp_point_read_binary(&ec_key->grp, &ec_key->Q, pub, pub_len);
    if (ret != 0) {
        return ret;
    }
    return mbedtls_ecp_check_pubkey(&ec_key->grp, &ec_key->Q);

}

