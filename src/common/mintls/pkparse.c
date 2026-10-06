// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Public Key layer for parsing key files and structures
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/pkparse.c.
 */

#include "common.h"

#include "pk.h"
#include "asn1.h"
#include "oid.h"
#include "platform_util.h"
#include "platform.h"
#include "error.h"
#include "ecp.h"
#include "pk_internal.h"

#include <string.h>

/* Key types */
#include "rsa.h"
#include "rsa_internal.h"

/* Extended formats */
#include "pem.h"

/***********************************************************************
 *
 *      Low-level ECC parsing: optional support for SpecifiedECDomain
 *
 * There are two functions here that are used by the rest of the code:
 * - pk_ecc_tag_is_speficied_ec_domain()
 * - pk_ecc_group_id_from_specified()
 *
 * All the other functions are internal to this section.
 *
 * The two "public" functions have a dummy variant provided
 * in configs without MBEDTLS_PK_PARSE_EC_EXTENDED. This acts as an
 * abstraction layer for this macro, which should not appear outside
 * this section.
 *
 **********************************************************************/

#if !defined(MBEDTLS_PK_PARSE_EC_EXTENDED)
/* See the "real" version for documentation */
int pk_ecc_tag_is_specified_ec_domain(int tag)
{
    (void) tag;
    return 0;
}

/* See the "real" version for documentation */
int pk_ecc_group_id_from_specified(const mbedtls_asn1_buf *params,
                                          mbedtls_ecp_group_id *grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    (void) params;
    (void) grp_id;
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
}
#else
/*
 * Tell if the passed tag might be the start of SpecifiedECDomain
 * (that is, a sequence).
 */
int pk_ecc_tag_is_specified_ec_domain(int tag)
{
    return tag == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
}

/*
 * Parse a SpecifiedECDomain (SEC 1 C.2) and (mostly) fill the group with it.
 * WARNING: the resulting group should only be used with
 * pk_ecc_group_id_from_specified(), since its base point may not be set correctly
 * if it was encoded compressed.
 *
 *  SpecifiedECDomain ::= SEQUENCE {
 *      version SpecifiedECDomainVersion(ecdpVer1 | ecdpVer2 | ecdpVer3, ...),
 *      fieldID FieldID {{FieldTypes}},
 *      curve Curve,
 *      base ECPoint,
 *      order INTEGER,
 *      cofactor INTEGER OPTIONAL,
 *      hash HashAlgorithm OPTIONAL,
 *      ...
 *  }
 *
 * We only support prime-field as field type, and ignore hash and cofactor.
 */
int pk_group_from_specified(const mbedtls_asn1_buf *params, mbedtls_ecp_group *grp, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = params->p;
    const unsigned char *const end = params->p + params->len;
    const unsigned char *end_field, *end_curve;
    size_t len;
    int ver;

    /* SpecifiedECDomainVersion ::= INTEGER { 1, 2, 3 } */
    if ((ret = mbedtls_asn1_get_int(&p, end, &ver, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if (ver < 1 || ver > 3) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
    }

    /*
     * FieldID { FIELD-ID:IOSet } ::= SEQUENCE { -- Finite field
     *       fieldType FIELD-ID.&id({IOSet}),
     *       parameters FIELD-ID.&Type({IOSet}{@fieldType})
     * }
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    end_field = p + len;

    /*
     * FIELD-ID ::= TYPE-IDENTIFIER
     * FieldTypes FIELD-ID ::= {
     *       { Prime-p IDENTIFIED BY prime-field } |
     *       { Characteristic-two IDENTIFIED BY characteristic-two-field }
     * }
     * prime-field OBJECT IDENTIFIER ::= { id-fieldType 1 }
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end_field, &len, MBEDTLS_ASN1_OID, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (len != MBEDTLS_OID_SIZE(MBEDTLS_OID_ANSI_X9_62_PRIME_FIELD) ||
        memcmp(p, MBEDTLS_OID_ANSI_X9_62_PRIME_FIELD, len) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE);
    }

    p += len;

    /* Prime-p ::= INTEGER -- Field of size p. */
    if ((ret = mbedtls_asn1_get_mpi(&p, end_field, &grp->P, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    grp->pbits = mbedtls_mpi_bitlen(&grp->P);

    if (p != end_field) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    /*
     * Curve ::= SEQUENCE {
     *       a FieldElement,
     *       b FieldElement,
     *       seed BIT STRING OPTIONAL
     *       -- Shall be present if used in SpecifiedECDomain
     *       -- with version equal to ecdpVer2 or ecdpVer3
     * }
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    end_curve = p + len;

    /*
     * FieldElement ::= OCTET STRING
     * containing an integer in the case of a prime field
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end_curve, &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_read_binary(&grp->A, p, len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    p += len;

    if ((ret = mbedtls_asn1_get_tag(&p, end_curve, &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_read_binary(&grp->B, p, len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    p += len;

    /* Ignore seed BIT STRING OPTIONAL */
    if ((ret = mbedtls_asn1_get_tag(&p, end_curve, &len, MBEDTLS_ASN1_BIT_STRING, diagnostics)) == 0) {
        p += len;
    }

    if (p != end_curve) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    /*
     * ECPoint ::= OCTET STRING
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if ((ret = mbedtls_ecp_point_read_binary(grp, &grp->G,
                                             (const unsigned char *) p, len, diagnostics)) != 0) {
        /*
         * If we can't read the point because it's compressed, cheat by
         * reading only the X coordinate and the parity bit of Y.
         */
        if (ret != MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE ||
            (p[0] != 0x02 && p[0] != 0x03) ||
            len != mbedtls_mpi_size(&grp->P) + 1 ||
            mbedtls_mpi_read_binary(&grp->G.X, p + 1, len - 1, diagnostics) != 0 ||
            mbedtls_mpi_lset(&grp->G.Y, p[0] - 2, diagnostics) != 0 ||
            mbedtls_mpi_lset(&grp->G.Z, 1, diagnostics) != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
        }
    }

    p += len;

    /*
     * order INTEGER
     */
    if ((ret = mbedtls_asn1_get_mpi(&p, end, &grp->N, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    grp->nbits = mbedtls_mpi_bitlen(&grp->N);

    /*
     * Allow optional elements by purposefully not enforcing p == end here.
     */

    MINTLS_RETURN(0);
}

/*
 * Find the group id associated with an (almost filled) group as generated by
 * pk_group_from_specified(), or return an error if unknown.
 */
int pk_group_id_from_group(const mbedtls_ecp_group *grp, mbedtls_ecp_group_id *grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    mbedtls_ecp_group ref;
    const mbedtls_ecp_group_id *id;

    mbedtls_ecp_group_init(&ref);

    for (id = mbedtls_ecp_grp_id_list(); *id != MBEDTLS_ECP_DP_NONE; id++) {
        /* Load the group associated to that id */
        mbedtls_ecp_group_free(&ref);
        MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&ref, *id, diagnostics));

        /* Compare to the group we were given, starting with easy tests */
        if (grp->pbits == ref.pbits && grp->nbits == ref.nbits &&
            mbedtls_mpi_cmp_mpi(&grp->P, &ref.P) == 0 &&
            mbedtls_mpi_cmp_mpi(&grp->A, &ref.A) == 0 &&
            mbedtls_mpi_cmp_mpi(&grp->B, &ref.B) == 0 &&
            mbedtls_mpi_cmp_mpi(&grp->N, &ref.N) == 0 &&
            mbedtls_mpi_cmp_mpi(&grp->G.X, &ref.G.X) == 0 &&
            mbedtls_mpi_cmp_mpi(&grp->G.Z, &ref.G.Z) == 0 &&
            /* For Y we may only know the parity bit, so compare only that */
            mbedtls_mpi_get_bit(&grp->G.Y, 0) == mbedtls_mpi_get_bit(&ref.G.Y, 0)) {
            break;
        }
    }

cleanup:
    mbedtls_ecp_group_free(&ref);

    *grp_id = *id;

    if (ret == 0 && *id == MBEDTLS_ECP_DP_NONE) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
    }

    MINTLS_RETURN(ret);
}

/*
 * Parse a SpecifiedECDomain (SEC 1 C.2) and find the associated group ID
 */
int pk_ecc_group_id_from_specified(const mbedtls_asn1_buf *params,
                                          mbedtls_ecp_group_id *grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_group grp;

    mbedtls_ecp_group_init(&grp);

    if ((ret = pk_group_from_specified(params, &grp, diagnostics)) != 0) {
        goto cleanup;
    }

    ret = pk_group_id_from_group(&grp, grp_id, diagnostics);

cleanup:
    /* The API respecting lifecycle for mbedtls_ecp_group struct is
     * _init(), _load() and _free(). In pk_ecc_group_id_from_specified() the
     * temporary grp breaks that flow and it's members are populated
     * by pk_group_id_from_group(). As such mbedtls_ecp_group_free()
     * which is assuming a group populated by _setup() may not clean-up
     * properly -> Manually free it's members.
     */
    mbedtls_mpi_free(&grp.N);
    mbedtls_mpi_free(&grp.P);
    mbedtls_mpi_free(&grp.A);
    mbedtls_mpi_free(&grp.B);
    mbedtls_ecp_point_free(&grp.G);

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_PK_PARSE_EC_EXTENDED */

/***********************************************************************
 *
 * Unsorted (yet!) from this point on until the next section header
 *
 **********************************************************************/

/* Minimally parse an ECParameters buffer to and mbedtls_asn1_buf
 *
 * ECParameters ::= CHOICE {
 *   namedCurve         OBJECT IDENTIFIER
 *   specifiedCurve     SpecifiedECDomain -- = SEQUENCE { ... }
 *   -- implicitCurve   NULL
 * }
 */
int pk_get_ecparams(unsigned char **p, const unsigned char *end,
                           mbedtls_asn1_buf *params, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (end - *p < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    /* Acceptable tags: OID for namedCurve, or specifiedECDomain */
    params->tag = **p;
    if (params->tag != MBEDTLS_ASN1_OID &&
        !pk_ecc_tag_is_specified_ec_domain(params->tag)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_UNEXPECTED_TAG));
    }

    if ((ret = mbedtls_asn1_get_tag(p, end, &params->len, params->tag, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    params->p = *p;
    *p += params->len;

    if (*p != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

/*
 * Use EC parameters to initialise an EC group
 *
 * ECParameters ::= CHOICE {
 *   namedCurve         OBJECT IDENTIFIER
 *   specifiedCurve     SpecifiedECDomain -- = SEQUENCE { ... }
 *   -- implicitCurve   NULL
 */
int pk_use_ecparams(const mbedtls_asn1_buf *params, mbedtls_pk_context *pk, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_group_id grp_id;

    if (params->tag == MBEDTLS_ASN1_OID) {
        if (mbedtls_oid_get_ec_grp(params, &grp_id, diagnostics) != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE);
        }
    } else {
        ret = pk_ecc_group_id_from_specified(params, &grp_id, diagnostics);
        if (ret != 0) {
            MINTLS_RETURN(ret);
        }
    }

    MINTLS_RETURN(mbedtls_pk_ecc_set_group(pk, grp_id, diagnostics));
}

#if defined(MBEDTLS_PK_HAVE_RFC8410_CURVES)

/*
 * Load an RFC8410 EC key, which doesn't have any parameters
 */
int pk_use_ecparams_rfc8410(const mbedtls_asn1_buf *params,
                                   mbedtls_ecp_group_id grp_id,
                                   mbedtls_pk_context *pk, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (params->tag != 0 || params->len != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
    }

    MINTLS_RETURN(mbedtls_pk_ecc_set_group(pk, grp_id, diagnostics));
}

/*
 * Parse an RFC 8410 encoded private EC key
 *
 * CurvePrivateKey ::= OCTET STRING
 */
int pk_parse_key_rfc8410_der(mbedtls_pk_context *pk,
                                    unsigned char *key, size_t keylen, const unsigned char *end,
                                    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    if ((ret = mbedtls_asn1_get_tag(&key, (key + keylen), &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if (key + len != end) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
    }

    /*
     * Load the private key
     */
    ret = mbedtls_pk_ecc_set_key(pk, key, len, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    /* pk_parse_key_pkcs8_unencrypted_der() only supports version 1 PKCS8 keys,
     * which never contain a public key. As such, derive the public key
     * unconditionally. */
    if ((ret = mbedtls_pk_ecc_set_pubkey_from_prv(pk, key, len, f_rng, p_rng, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}
#endif /* MBEDTLS_PK_HAVE_RFC8410_CURVES */

/* Get a PK algorithm identifier
 *
 *  AlgorithmIdentifier  ::=  SEQUENCE  {
 *       algorithm               OBJECT IDENTIFIER,
 *       parameters              ANY DEFINED BY algorithm OPTIONAL  }
 */
int pk_get_pk_alg(unsigned char **p,
                         const unsigned char *end,
                         mbedtls_pk_type_t *pk_alg, mbedtls_asn1_buf *params,
                         mbedtls_ecp_group_id *ec_grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_asn1_buf alg_oid;

    memset(params, 0, sizeof(mbedtls_asn1_buf));

    if ((ret = mbedtls_asn1_get_alg(p, end, &alg_oid, params, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_INVALID_ALG, ret));
    }

    ret = mbedtls_oid_get_pk_alg(&alg_oid, pk_alg, diagnostics);
    if (ret == MBEDTLS_ERR_OID_NOT_FOUND) {
        ret = mbedtls_oid_get_ec_grp_algid(&alg_oid, ec_grp_id, diagnostics);
        if (ret == 0) {
            *pk_alg = MBEDTLS_PK_ECKEY;
        }
    }
    if (ret != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);
    }

    /*
     * No parameters with RSA (only for EC)
     */
    if (*pk_alg == MBEDTLS_PK_RSA &&
        ((params->tag != MBEDTLS_ASN1_NULL && params->tag != 0) ||
         params->len != 0)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_INVALID_ALG);
    }

    MINTLS_RETURN(0);
}

/*
 *  SubjectPublicKeyInfo  ::=  SEQUENCE  {
 *       algorithm            AlgorithmIdentifier,
 *       subjectPublicKey     BIT STRING }
 */
int mbedtls_pk_parse_subpubkey(unsigned char **p, const unsigned char *end,
                               mbedtls_pk_context *pk, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;
    mbedtls_asn1_buf alg_params;
    mbedtls_pk_type_t pk_alg = MBEDTLS_PK_NONE;
    mbedtls_ecp_group_id ec_grp_id = MBEDTLS_ECP_DP_NONE;
    const mbedtls_pk_info_t *pk_info;

    if ((ret = mbedtls_asn1_get_tag(p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    end = *p + len;

    if ((ret = pk_get_pk_alg(p, end, &pk_alg, &alg_params, &ec_grp_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_asn1_get_bitstring_null(p, end, &len, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_INVALID_PUBKEY, ret));
    }

    if (*p + len != end) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_INVALID_PUBKEY,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    if ((pk_info = mbedtls_pk_info_from_type(pk_alg)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);
    }

    if ((ret = mbedtls_pk_setup(pk, pk_info, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (pk_alg == MBEDTLS_PK_RSA) {
        ret = mbedtls_rsa_parse_pubkey(mbedtls_pk_rsa(*pk), *p, (size_t) (end - *p), diagnostics);
        if (ret == 0) {
            /* On success all the input has been consumed by the parsing function. */
            *p += end - *p;
        } else if ((ret <= MBEDTLS_ERR_ASN1_OUT_OF_DATA) &&
                   (ret >= MBEDTLS_ERR_ASN1_BUF_TOO_SMALL)) {
            /* In case of ASN1 error codes add MBEDTLS_ERR_PK_INVALID_PUBKEY. */
            ret = MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_INVALID_PUBKEY, ret);
        } else {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PK_INVALID_PUBKEY);
        }
    } else
    if (pk_alg == MBEDTLS_PK_ECKEY_DH || pk_alg == MBEDTLS_PK_ECKEY) {
#if defined(MBEDTLS_PK_HAVE_RFC8410_CURVES)
        if (MBEDTLS_PK_IS_RFC8410_GROUP_ID(ec_grp_id)) {
            ret = pk_use_ecparams_rfc8410(&alg_params, ec_grp_id, pk, diagnostics);
        } else
#endif
        {
            ret = pk_use_ecparams(&alg_params, pk, diagnostics);
        }
        if (ret == 0) {
            ret = mbedtls_pk_ecc_set_pubkey(pk, *p, (size_t) (end - *p), diagnostics);
            *p += end - *p;
        }
    } else
    ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);

    if (ret == 0 && *p != end) {
        ret = MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_INVALID_PUBKEY,
                                MBEDTLS_ERR_ASN1_LENGTH_MISMATCH);
    }

    if (ret != 0) {
        mbedtls_pk_free(pk);
    }

    MINTLS_RETURN(ret);
}

/*
 * Parse a SEC1 encoded private EC key
 */
int pk_parse_key_sec1_der(mbedtls_pk_context *pk,
                                 const unsigned char *key, size_t keylen,
                                 int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int version, pubkey_done;
    size_t len, d_len;
    mbedtls_asn1_buf params = { 0, 0, NULL };
    unsigned char *p = (unsigned char *) key;
    unsigned char *d;
    unsigned char *end = p + keylen;
    unsigned char *end2;

    /*
     * RFC 5915, or SEC1 Appendix C.4
     *
     * ECPrivateKey ::= SEQUENCE {
     *      version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
     *      privateKey     OCTET STRING,
     *      parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
     *      publicKey  [1] BIT STRING OPTIONAL
     *    }
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    end = p + len;

    if ((ret = mbedtls_asn1_get_int(&p, end, &version, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if (version != 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_VERSION);
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    /* Keep a reference to the position fo the private key. It will be used
     * later in this function. */
    d = p;
    d_len = len;

    p += len;

    pubkey_done = 0;
    if (p != end) {
        /*
         * Is 'parameters' present?
         */
        if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                        MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                        0, diagnostics)) == 0) {
            if ((ret = pk_get_ecparams(&p, p + len, &params, diagnostics)) != 0 ||
                (ret = pk_use_ecparams(&params, pk, diagnostics)) != 0) {
                MINTLS_RETURN(ret);
            }
        } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
        }
    }

    /*
     * Load the private key
     */
    ret = mbedtls_pk_ecc_set_key(pk, d, d_len, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    if (p != end) {
        /*
         * Is 'publickey' present? If not, or if we can't read it (eg because it
         * is compressed), create it from the private key.
         */
        if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                        MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED |
                                        1, diagnostics)) == 0) {
            end2 = p + len;

            if ((ret = mbedtls_asn1_get_bitstring_null(&p, end2, &len, diagnostics)) != 0) {
                MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
            }

            if (p + len != end2) {
                MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                         MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
            }

            if ((ret = mbedtls_pk_ecc_set_pubkey(pk, p, (size_t) (end2 - p), diagnostics)) == 0) {
                pubkey_done = 1;
            } else {
                /*
                 * The only acceptable failure mode of mbedtls_pk_ecc_set_pubkey() above
                 * is if the point format is not recognized.
                 */
                if (ret != MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE) {
                    MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
                }
            }
        } else if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
        }
    }

    if (!pubkey_done) {
        if ((ret = mbedtls_pk_ecc_set_pubkey_from_prv(pk, d, d_len, f_rng, p_rng, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }
    }

    MINTLS_RETURN(0);
}

/***********************************************************************
 *
 *      PKCS#8 parsing functions
 *
 **********************************************************************/

/*
 * Parse an unencrypted PKCS#8 encoded private key
 *
 * Notes:
 *
 * - This function does not own the key buffer. It is the
 *   responsibility of the caller to take care of zeroizing
 *   and freeing it after use.
 *
 * - The function is responsible for freeing the provided
 *   PK context on failure.
 *
 */
int pk_parse_key_pkcs8_unencrypted_der(
    mbedtls_pk_context *pk,
    const unsigned char *key, size_t keylen,
    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, version;
    size_t len;
    mbedtls_asn1_buf params;
    unsigned char *p = (unsigned char *) key;
    unsigned char *end = p + keylen;
    mbedtls_pk_type_t pk_alg = MBEDTLS_PK_NONE;
    mbedtls_ecp_group_id ec_grp_id = MBEDTLS_ECP_DP_NONE;
    const mbedtls_pk_info_t *pk_info;

    /*
     * This function parses the PrivateKeyInfo object (PKCS#8 v1.2 = RFC 5208)
     *
     *    PrivateKeyInfo ::= SEQUENCE {
     *      version                   Version,
     *      privateKeyAlgorithm       PrivateKeyAlgorithmIdentifier,
     *      privateKey                PrivateKey,
     *      attributes           [0]  IMPLICIT Attributes OPTIONAL }
     *
     *    Version ::= INTEGER
     *    PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
     *    PrivateKey ::= OCTET STRING
     *
     *  The PrivateKey OCTET STRING is a SEC1 ECPrivateKey
     */

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    end = p + len;

    if ((ret = mbedtls_asn1_get_int(&p, end, &version, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if (version != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_VERSION, ret));
    }

    if ((ret = pk_get_pk_alg(&p, end, &pk_alg, &params, &ec_grp_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT, ret));
    }

    if (len < 1) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_OUT_OF_DATA));
    }

    if ((pk_info = mbedtls_pk_info_from_type(pk_alg)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);
    }

    if ((ret = mbedtls_pk_setup(pk, pk_info, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (pk_alg == MBEDTLS_PK_RSA) {
        if ((ret = mbedtls_rsa_parse_key(mbedtls_pk_rsa(*pk), p, len, diagnostics)) != 0) {
            mbedtls_pk_free(pk);
            MINTLS_RETURN(ret);
        }
    } else
    if (pk_alg == MBEDTLS_PK_ECKEY || pk_alg == MBEDTLS_PK_ECKEY_DH) {
#if defined(MBEDTLS_PK_HAVE_RFC8410_CURVES)
        if (MBEDTLS_PK_IS_RFC8410_GROUP_ID(ec_grp_id)) {
            if ((ret =
                     pk_use_ecparams_rfc8410(&params, ec_grp_id, pk, diagnostics)) != 0 ||
                (ret =
                     pk_parse_key_rfc8410_der(pk, p, len, end, f_rng,
                                              p_rng, diagnostics)) != 0) {
                mbedtls_pk_free(pk);
                MINTLS_RETURN(ret);
            }
        } else
#endif
        {
            if ((ret = pk_use_ecparams(&params, pk, diagnostics)) != 0 ||
                (ret = pk_parse_key_sec1_der(pk, p, len, f_rng, p_rng, diagnostics)) != 0) {
                mbedtls_pk_free(pk);
                MINTLS_RETURN(ret);
            }
        }
    } else
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);

    end = p + len;
    if (end != (key + keylen)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT,
                                 MBEDTLS_ERR_ASN1_LENGTH_MISMATCH));
    }

    MINTLS_RETURN(0);
}

/*
 * Parse an encrypted PKCS#8 encoded private key
 *
 * To save space, the decryption happens in-place on the given key buffer.
 * Also, while this function may modify the keybuffer, it doesn't own it,
 * and instead it is the responsibility of the caller to zeroize and properly
 * free it after use.
 *
 */

/***********************************************************************
 *
 *      Top-level functions, with format auto-discovery
 *
 **********************************************************************/

/*
 * Parse a private key
 */
int mbedtls_pk_parse_key(mbedtls_pk_context *pk,
                         const unsigned char *key, size_t keylen,
                         const unsigned char *pwd, size_t pwdlen,
                         int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_pk_info_t *pk_info;
    size_t len;
    mbedtls_pem_context pem;

    if (keylen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
    }

    mbedtls_pem_init(&pem);

    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if (key[keylen - 1] != '\0') {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    } else {
        ret = mbedtls_pem_read_buffer(&pem,
                                      PEM_BEGIN_PRIVATE_KEY_RSA, PEM_END_PRIVATE_KEY_RSA,
                                      key, pwd, pwdlen, &len, diagnostics);
    }

    if (ret == 0) {
        pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
        if ((ret = mbedtls_pk_setup(pk, pk_info, diagnostics)) != 0 ||
            (ret = mbedtls_rsa_parse_key(mbedtls_pk_rsa(*pk),
                                         pem.buf, pem.buflen, diagnostics)) != 0) {
            mbedtls_pk_free(pk);
        }

        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    } else if (ret == MBEDTLS_ERR_PEM_PASSWORD_MISMATCH) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_PASSWORD_MISMATCH);
    } else if (ret == MBEDTLS_ERR_PEM_PASSWORD_REQUIRED) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_PASSWORD_REQUIRED);
    } else if (ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT) {
        MINTLS_RETURN(ret);
    }

    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if (key[keylen - 1] != '\0') {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    } else {
        ret = mbedtls_pem_read_buffer(&pem,
                                      PEM_BEGIN_PRIVATE_KEY_EC,
                                      PEM_END_PRIVATE_KEY_EC,
                                      key, pwd, pwdlen, &len, diagnostics);
    }
    if (ret == 0) {
        pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);

        if ((ret = mbedtls_pk_setup(pk, pk_info, diagnostics)) != 0 ||
            (ret = pk_parse_key_sec1_der(pk,
                                         pem.buf, pem.buflen,
                                         f_rng, p_rng, diagnostics)) != 0) {
            mbedtls_pk_free(pk);
        }

        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    } else if (ret == MBEDTLS_ERR_PEM_PASSWORD_MISMATCH) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_PASSWORD_MISMATCH);
    } else if (ret == MBEDTLS_ERR_PEM_PASSWORD_REQUIRED) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_PASSWORD_REQUIRED);
    } else if (ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT) {
        MINTLS_RETURN(ret);
    }

    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if (key[keylen - 1] != '\0') {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    } else {
        ret = mbedtls_pem_read_buffer(&pem,
                                      PEM_BEGIN_PRIVATE_KEY_PKCS8, PEM_END_PRIVATE_KEY_PKCS8,
                                      key, NULL, 0, &len, diagnostics);
    }
    if (ret == 0) {
        if ((ret = pk_parse_key_pkcs8_unencrypted_der(pk,
                                                      pem.buf, pem.buflen, f_rng, p_rng, diagnostics)) != 0) {
            mbedtls_pk_free(pk);
        }

        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    } else if (ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT) {
        MINTLS_RETURN(ret);
    }

    /*
     * At this point we only know it's not a PEM formatted key. Could be any
     * of the known DER encoded private key formats
     *
     * We try the different DER format parsers to see if one passes without
     * error
     */

    ret = pk_parse_key_pkcs8_unencrypted_der(pk, key, keylen, f_rng, p_rng, diagnostics);
    if (ret == 0) {
        MINTLS_RETURN(0);
    }

    mbedtls_pk_free(pk);
    mbedtls_pk_init(pk);

    pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    if (mbedtls_pk_setup(pk, pk_info, diagnostics) == 0 &&
        mbedtls_rsa_parse_key(mbedtls_pk_rsa(*pk), key, keylen, diagnostics) == 0) {
        MINTLS_RETURN(0);
    }

    mbedtls_pk_free(pk);
    mbedtls_pk_init(pk);

    pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);
    if (mbedtls_pk_setup(pk, pk_info, diagnostics) == 0 &&
        pk_parse_key_sec1_der(pk,
                              key, keylen, f_rng, p_rng, diagnostics) == 0) {
        MINTLS_RETURN(0);
    }
    mbedtls_pk_free(pk);

    /* If MBEDTLS_RSA_C is defined but MBEDTLS_PK_HAVE_ECC_KEYS isn't,
     * it is ok to leave the PK context initialized but not
     * freed: It is the caller's responsibility to call pk_init()
     * before calling this function, and to call pk_free()
     * when it fails. If MBEDTLS_PK_HAVE_ECC_KEYS is defined but MBEDTLS_RSA_C
     * isn't, this leads to mbedtls_pk_free() being called
     * twice, once here and once by the caller, but this is
     * also ok and in line with the mbedtls_pk_free() calls
     * on failed PEM parsing attempts. */

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
}

/*
 * Parse a public key
 */
int mbedtls_pk_parse_public_key(mbedtls_pk_context *ctx,
                                const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p;
    const mbedtls_pk_info_t *pk_info;
    size_t len;
    mbedtls_pem_context pem;

    if (keylen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT);
    }

    mbedtls_pem_init(&pem);
    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if (key[keylen - 1] != '\0') {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    } else {
        ret = mbedtls_pem_read_buffer(&pem,
                                      PEM_BEGIN_PUBLIC_KEY_RSA, PEM_END_PUBLIC_KEY_RSA,
                                      key, NULL, 0, &len, diagnostics);
    }

    if (ret == 0) {
        p = pem.buf;
        if ((pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) == NULL) {
            mbedtls_pem_free(&pem);
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);
        }

        if ((ret = mbedtls_pk_setup(ctx, pk_info, diagnostics)) != 0) {
            mbedtls_pem_free(&pem);
            MINTLS_RETURN(ret);
        }

        if ((ret = mbedtls_rsa_parse_pubkey(mbedtls_pk_rsa(*ctx), p, pem.buflen, diagnostics)) != 0) {
            mbedtls_pk_free(ctx);
        }

        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    } else if (ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT) {
        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    }

    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if (key[keylen - 1] != '\0') {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT);
    } else {
        ret = mbedtls_pem_read_buffer(&pem,
                                      PEM_BEGIN_PUBLIC_KEY, PEM_END_PUBLIC_KEY,
                                      key, NULL, 0, &len, diagnostics);
    }

    if (ret == 0) {
        /*
         * Was PEM encoded
         */
        p = pem.buf;

        ret = mbedtls_pk_parse_subpubkey(&p, p + pem.buflen, ctx, diagnostics);
        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    } else if (ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT) {
        mbedtls_pem_free(&pem);
        MINTLS_RETURN(ret);
    }
    mbedtls_pem_free(&pem);

    if ((pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG);
    }

    if ((ret = mbedtls_pk_setup(ctx, pk_info, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    p = (unsigned char *) key;
    ret = mbedtls_rsa_parse_pubkey(mbedtls_pk_rsa(*ctx), p, keylen, diagnostics);
    if (ret == 0) {
        MINTLS_RETURN(ret);
    }
    mbedtls_pk_free(ctx);
    if (ret != MBEDTLS_ERR_ASN1_UNEXPECTED_TAG) {
        MINTLS_RETURN(ret);
    }
    p = (unsigned char *) key;

    ret = mbedtls_pk_parse_subpubkey(&p, p + keylen, ctx, diagnostics);

    MINTLS_RETURN(ret);
}

/***********************************************************************
 *
 *      Top-level functions, with filesystem support
 *
 **********************************************************************/

