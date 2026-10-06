// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Elliptic curves over GF(p): generic functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ecp.c.
 */

/*
 * References:
 *
 * SEC1 https://www.secg.org/sec1-v2.pdf
 * GECC = Guide to Elliptic Curve Cryptography - Hankerson, Menezes, Vanstone
 * FIPS 186-3 http://csrc.nist.gov/publications/fips/fips186-3/fips_186-3.pdf
 * RFC 4492 for the related TLS structures and constants
 * - https://www.rfc-editor.org/rfc/rfc4492
 * RFC 7748 for the Curve448 and Curve25519 curve definitions
 * - https://www.rfc-editor.org/rfc/rfc7748
 *
 * [Curve25519] https://cr.yp.to/ecdh/curve25519-20060209.pdf
 *
 * [2] CORON, Jean-S'ebastien. Resistance against differential power analysis
 *     for elliptic curve cryptosystems. In : Cryptographic Hardware and
 *     Embedded Systems. Springer Berlin Heidelberg, 1999. p. 292-302.
 *     <http://link.springer.com/chapter/10.1007/3-540-48059-5_25>
 *
 * [3] HEDABOU, Mustapha, PINEL, Pierre, et B'EN'ETEAU, Lucien. A comb method to
 *     render ECC resistant against Side Channel Attacks. IACR Cryptology
 *     ePrint Archive, 2004, vol. 2004, p. 342.
 *     <http://eprint.iacr.org/2004/342.pdf>
 */

#include "common.h"

/**
 * \brief Function level alternative implementation.
 *
 * The MBEDTLS_ECP_INTERNAL_ALT macro enables alternative implementations to
 * replace certain functions in this module. The alternative implementations are
 * typically hardware accelerators and need to activate the hardware before the
 * computation starts and deactivate it after it finishes. The
 * mbedtls_internal_ecp_init() and mbedtls_internal_ecp_free() functions serve
 * this purpose.
 *
 * To preserve the correct functionality the following conditions must hold:
 *
 * - The alternative implementation must be activated by
 *   mbedtls_internal_ecp_init() before any of the replaceable functions is
 *   called.
 * - mbedtls_internal_ecp_free() must \b only be called when the alternative
 *   implementation is activated.
 * - mbedtls_internal_ecp_init() must \b not be called when the alternative
 *   implementation is activated.
 * - Public functions must not return while the alternative implementation is
 *   activated.
 * - Replaceable functions are guarded by \c MBEDTLS_ECP_XXX_ALT macros and
 *   before calling them an \code if( mbedtls_internal_ecp_grp_capable( grp ) )
 *   \endcode ensures that the alternative implementation supports the current
 *   group.
 */

#include "ecp.h"
#include "platform_util.h"
#include "error.h"

#include "bn_mul.h"
#include "bignum_internal.h"
#include "bignum_core.h"

#include <string.h>

#include "platform.h"

#define ECP_RS_ENTER(sub)     (void) rs_ctx;
#define ECP_RS_LEAVE(sub)     (void) rs_ctx;

void mpi_init_many(mbedtls_mpi *arr, size_t size)
{
    while (size--) {
        mbedtls_mpi_init(arr++);
    }
}

void mpi_free_many(mbedtls_mpi *arr, size_t size)
{
    while (size--) {
        mbedtls_mpi_free(arr++);
    }
}

/*
 * List of supported curves:
 *  - internal ID
 *  - TLS NamedCurve ID (RFC 4492 sec. 5.1.1, RFC 7071 sec. 2, RFC 8446 sec. 4.2.7)
 *  - size in bits
 *  - readable name
 *
 * Curves are listed in order: largest curves first, and for a given size,
 * fastest curves first.
 *
 * Reminder: update profiles in x509_crt.c and ssl_tls.c when adding a new curve!
 */
static const mbedtls_ecp_curve_info ecp_supported_curves[] =
{
    { MBEDTLS_ECP_DP_SECP521R1,    25,     521,    "secp521r1"         },
    { MBEDTLS_ECP_DP_SECP384R1,    24,     384,    "secp384r1"         },
    { MBEDTLS_ECP_DP_SECP256R1,    23,     256,    "secp256r1"         },
    { MBEDTLS_ECP_DP_NONE,          0,     0,      NULL                },
};

#define ECP_NB_CURVES   sizeof(ecp_supported_curves) /    \
    sizeof(ecp_supported_curves[0])

static mbedtls_ecp_group_id ecp_supported_grp_id[ECP_NB_CURVES];

/*
 * List of supported curves and associated info
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_list(void)
{
    return ecp_supported_curves;
}

/*
 * List of supported curves, group ID only
 */
const mbedtls_ecp_group_id *mbedtls_ecp_grp_id_list(void)
{
    static int init_done = 0;

    if (!init_done) {
        size_t i = 0;
        const mbedtls_ecp_curve_info *curve_info;

        for (curve_info = mbedtls_ecp_curve_list();
             curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
             curve_info++) {
            ecp_supported_grp_id[i++] = curve_info->grp_id;
        }
        ecp_supported_grp_id[i] = MBEDTLS_ECP_DP_NONE;

        init_done = 1;
    }

    return ecp_supported_grp_id;
}

/*
 * Get the curve info for the internal identifier
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_info_from_grp_id(mbedtls_ecp_group_id grp_id)
{
    const mbedtls_ecp_curve_info *curve_info;

    for (curve_info = mbedtls_ecp_curve_list();
         curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
         curve_info++) {
        if (curve_info->grp_id == grp_id) {
            return curve_info;
        }
    }

    return NULL;
}

/*
 * Get the curve info from the TLS identifier
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_info_from_tls_id(uint16_t tls_id)
{
    const mbedtls_ecp_curve_info *curve_info;

    for (curve_info = mbedtls_ecp_curve_list();
         curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
         curve_info++) {
        if (curve_info->tls_id == tls_id) {
            return curve_info;
        }
    }

    return NULL;
}

/*
 * Get the curve info from the name
 */
const mbedtls_ecp_curve_info *mbedtls_ecp_curve_info_from_name(const char *name)
{
    const mbedtls_ecp_curve_info *curve_info;

    if (name == NULL) {
        return NULL;
    }

    for (curve_info = mbedtls_ecp_curve_list();
         curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
         curve_info++) {
        if (strcmp(curve_info->name, name) == 0) {
            return curve_info;
        }
    }

    return NULL;
}

/*
 * Get the type of a curve
 */
mbedtls_ecp_curve_type mbedtls_ecp_get_type(const mbedtls_ecp_group *grp)
{
    if (grp->G.X.p == NULL) {
        return MBEDTLS_ECP_TYPE_NONE;
    }

    if (grp->G.Y.p == NULL) {
        return MBEDTLS_ECP_TYPE_MONTGOMERY;
    } else {
        return MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS;
    }
}

/*
 * Initialize (the components of) a point
 */
void mbedtls_ecp_point_init(mbedtls_ecp_point *pt)
{
    mbedtls_mpi_init(&pt->X);
    mbedtls_mpi_init(&pt->Y);
    mbedtls_mpi_init(&pt->Z);
}

/*
 * Initialize (the components of) a group
 */
void mbedtls_ecp_group_init(mbedtls_ecp_group *grp)
{
    grp->id = MBEDTLS_ECP_DP_NONE;
    mbedtls_mpi_init(&grp->P);
    mbedtls_mpi_init(&grp->A);
    mbedtls_mpi_init(&grp->B);
    mbedtls_ecp_point_init(&grp->G);
    mbedtls_mpi_init(&grp->N);
    grp->pbits = 0;
    grp->nbits = 0;
    grp->h = 0;
    grp->modp = NULL;
    grp->t_pre = NULL;
    grp->t_post = NULL;
    grp->t_data = NULL;
    grp->T = NULL;
    grp->T_size = 0;
}

/*
 * Initialize (the components of) a key pair
 */
void mbedtls_ecp_keypair_init(mbedtls_ecp_keypair *key)
{
    mbedtls_ecp_group_init(&key->grp);
    mbedtls_mpi_init(&key->d);
    mbedtls_ecp_point_init(&key->Q);
}

/*
 * Unallocate (the components of) a point
 */
void mbedtls_ecp_point_free(mbedtls_ecp_point *pt)
{
    if (pt == NULL) {
        return;
    }

    mbedtls_mpi_free(&(pt->X));
    mbedtls_mpi_free(&(pt->Y));
    mbedtls_mpi_free(&(pt->Z));
}

/*
 * Check that the comb table (grp->T) is static initialized.
 */
int ecp_group_is_static_comb_table(const mbedtls_ecp_group *grp)
{
#if MBEDTLS_ECP_FIXED_POINT_OPTIM == 1
    return grp->T != NULL && grp->T_size == 0;
#else
    (void) grp;
    return 0;
#endif
}

/*
 * Unallocate (the components of) a group
 */
void mbedtls_ecp_group_free(mbedtls_ecp_group *grp)
{
    size_t i;

    if (grp == NULL) {
        return;
    }

    if (grp->h != 1) {
        mbedtls_mpi_free(&grp->A);
        mbedtls_mpi_free(&grp->B);
        mbedtls_ecp_point_free(&grp->G);

        if (grp->h != 2) {
            mbedtls_mpi_free(&grp->N);
            mbedtls_mpi_free(&grp->P);
        }
    }

    if (!ecp_group_is_static_comb_table(grp) && grp->T != NULL) {
        for (i = 0; i < grp->T_size; i++) {
            mbedtls_ecp_point_free(&grp->T[i]);
        }
        mbedtls_free(grp->T);
    }

    mbedtls_platform_zeroize(grp, sizeof(mbedtls_ecp_group));
}

/*
 * Unallocate (the components of) a key pair
 */
void mbedtls_ecp_keypair_free(mbedtls_ecp_keypair *key)
{
    if (key == NULL) {
        return;
    }

    mbedtls_ecp_group_free(&key->grp);
    mbedtls_mpi_free(&key->d);
    mbedtls_ecp_point_free(&key->Q);
}

/*
 * Copy the contents of a point
 */
int mbedtls_ecp_copy(mbedtls_ecp_point *P, const mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&P->X, &Q->X, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&P->Y, &Q->Y, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&P->Z, &Q->Z, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Copy the contents of a group object
 */
int mbedtls_ecp_group_copy(mbedtls_ecp_group *dst, const mbedtls_ecp_group *src, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_group_load(dst, src->id, diagnostics));
}

/*
 * Set point to zero
 */
int mbedtls_ecp_set_zero(mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&pt->X, 1, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&pt->Y, 1, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&pt->Z, 0, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Tell if a point is zero
 */
int mbedtls_ecp_is_zero(mbedtls_ecp_point *pt)
{
    return mbedtls_mpi_cmp_int(&pt->Z, 0) == 0;
}

/*
 * Compare two points lazily
 */
int mbedtls_ecp_point_cmp(const mbedtls_ecp_point *P,
                          const mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (mbedtls_mpi_cmp_mpi(&P->X, &Q->X) == 0 &&
        mbedtls_mpi_cmp_mpi(&P->Y, &Q->Y) == 0 &&
        mbedtls_mpi_cmp_mpi(&P->Z, &Q->Z) == 0) {
        MINTLS_RETURN(0);
    }

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
}

/*
 * Import a non-zero point from ASCII strings
 */
int mbedtls_ecp_point_read_string(mbedtls_ecp_point *P, int radix,
                                  const char *x, const char *y, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&P->X, radix, x, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&P->Y, radix, y, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&P->Z, 1, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Export a point into unsigned binary data (SEC1 2.3.3 and RFC7748)
 */
int mbedtls_ecp_point_write_binary(const mbedtls_ecp_group *grp,
                                   const mbedtls_ecp_point *P,
                                   int format, size_t *olen,
                                   unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
    size_t plen;
    if (format != MBEDTLS_ECP_PF_UNCOMPRESSED &&
        format != MBEDTLS_ECP_PF_COMPRESSED) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    plen = mbedtls_mpi_size(&grp->P);

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        /*
         * Common case: P == 0
         */
        if (mbedtls_mpi_cmp_int(&P->Z, 0) == 0) {
            if (buflen < 1) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
            }

            buf[0] = 0x00;
            *olen = 1;

            MINTLS_RETURN(0);
        }

        if (format == MBEDTLS_ECP_PF_UNCOMPRESSED) {
            *olen = 2 * plen + 1;

            if (buflen < *olen) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
            }

            buf[0] = 0x04;
            MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&P->X, buf + 1, plen, diagnostics));
            MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&P->Y, buf + 1 + plen, plen, diagnostics));
        } else if (format == MBEDTLS_ECP_PF_COMPRESSED) {
            *olen = plen + 1;

            if (buflen < *olen) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
            }

            buf[0] = 0x02 + mbedtls_mpi_get_bit(&P->Y, 0);
            MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&P->X, buf + 1, plen, diagnostics));
        }
    }
#endif

cleanup:
    MINTLS_RETURN(ret);
}

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
int mbedtls_ecp_sw_derive_y(const mbedtls_ecp_group *grp,
                                   const mbedtls_mpi *X,
                                   mbedtls_mpi *Y,
                                   int parity_bit, MinTlsDiagnostics* diagnostics);
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

/*
 * Import a point from unsigned binary data (SEC1 2.3.4 and RFC7748)
 */
int mbedtls_ecp_point_read_binary(const mbedtls_ecp_group *grp,
                                  mbedtls_ecp_point *pt,
                                  const unsigned char *buf, size_t ilen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
    size_t plen;
    if (ilen < 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    plen = mbedtls_mpi_size(&grp->P);

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        if (buf[0] == 0x00) {
            if (ilen == 1) {
                MINTLS_RETURN(mbedtls_ecp_set_zero(pt, diagnostics));
            } else {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
            }
        }

        if (ilen < 1 + plen) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
        }

        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&pt->X, buf + 1, plen, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&pt->Z, 1, diagnostics));

        if (buf[0] == 0x04) {
            /* format == MBEDTLS_ECP_PF_UNCOMPRESSED */
            if (ilen != 1 + plen * 2) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
            }
            MINTLS_RETURN(mbedtls_mpi_read_binary(&pt->Y, buf + 1 + plen, plen, diagnostics));
        } else if (buf[0] == 0x02 || buf[0] == 0x03) {
            /* format == MBEDTLS_ECP_PF_COMPRESSED */
            if (ilen != 1 + plen) {
                MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
            }
            MINTLS_RETURN(mbedtls_ecp_sw_derive_y(grp, &pt->X, &pt->Y,
                                           (buf[0] & 1), diagnostics));
        } else {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
        }
    }
#endif

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Import a point from a TLS ECPoint record (RFC 4492)
 *      struct {
 *          opaque point <1..2^8-1>;
 *      } ECPoint;
 */
int mbedtls_ecp_tls_read_point(const mbedtls_ecp_group *grp,
                               mbedtls_ecp_point *pt,
                               const unsigned char **buf, size_t buf_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char data_len;
    const unsigned char *buf_start;
    /*
     * We must have at least two bytes (1 for length, at least one for data)
     */
    if (buf_len < 2) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    data_len = *(*buf)++;
    if (data_len < 1 || data_len > buf_len - 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * Save buffer start for read_binary and update buf
     */
    buf_start = *buf;
    *buf += data_len;

    MINTLS_RETURN(mbedtls_ecp_point_read_binary(grp, pt, buf_start, data_len, diagnostics));
}

/*
 * Export a point as a TLS ECPoint record (RFC 4492)
 *      struct {
 *          opaque point <1..2^8-1>;
 *      } ECPoint;
 */
int mbedtls_ecp_tls_write_point(const mbedtls_ecp_group *grp, const mbedtls_ecp_point *pt,
                                int format, size_t *olen,
                                unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    if (format != MBEDTLS_ECP_PF_UNCOMPRESSED &&
        format != MBEDTLS_ECP_PF_COMPRESSED) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * buffer length must be at least one, for our length byte
     */
    if (blen < 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    if ((ret = mbedtls_ecp_point_write_binary(grp, pt, format,
                                              olen, buf + 1, blen - 1, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    /*
     * write length to the first byte and update total length
     */
    buf[0] = (unsigned char) *olen;
    ++*olen;

    MINTLS_RETURN(0);
}

/*
 * Set a group from an ECParameters record (RFC 4492)
 */
int mbedtls_ecp_tls_read_group(mbedtls_ecp_group *grp,
                               const unsigned char **buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_group_id grp_id;
    if ((ret = mbedtls_ecp_tls_read_group_id(&grp_id, buf, len, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(mbedtls_ecp_group_load(grp, grp_id, diagnostics));
}

/*
 * Read a group id from an ECParameters record (RFC 4492) and convert it to
 * mbedtls_ecp_group_id.
 */
int mbedtls_ecp_tls_read_group_id(mbedtls_ecp_group_id *grp,
                                  const unsigned char **buf, size_t len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    uint16_t tls_id;
    const mbedtls_ecp_curve_info *curve_info;
    /*
     * We expect at least three bytes (see below)
     */
    if (len < 3) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * First byte is curve_type; only named_curve is handled
     */
    if (*(*buf)++ != MBEDTLS_ECP_TLS_NAMED_CURVE) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * Next two bytes are the namedcurve value
     */
    tls_id = MBEDTLS_GET_UINT16_BE(*buf, 0);
    *buf += 2;

    if ((curve_info = mbedtls_ecp_curve_info_from_tls_id(tls_id)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
    }

    *grp = curve_info->grp_id;

    MINTLS_RETURN(0);
}

/*
 * Write the ECParameters record corresponding to a group (RFC 4492)
 */
int mbedtls_ecp_tls_write_group(const mbedtls_ecp_group *grp, size_t *olen,
                                unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_ecp_curve_info *curve_info;
    if ((curve_info = mbedtls_ecp_curve_info_from_grp_id(grp->id)) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * We are going to write 3 bytes (see below)
     */
    *olen = 3;
    if (blen < *olen) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
    }

    /*
     * First byte is curve_type, always named_curve
     */
    *buf++ = MBEDTLS_ECP_TLS_NAMED_CURVE;

    /*
     * Next two bytes are the namedcurve value
     */
    MBEDTLS_PUT_UINT16_BE(curve_info->tls_id, buf, 0);

    MINTLS_RETURN(0);
}

/*
 * Wrapper around fast quasi-modp functions, with fall-back to mbedtls_mpi_mod_mpi.
 * See the documentation of struct mbedtls_ecp_group.
 *
 * This function is in the critial loop for mbedtls_ecp_mul, so pay attention to perf.
 */
int ecp_modp(mbedtls_mpi *N, const mbedtls_ecp_group *grp, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (grp->modp == NULL) {
        MINTLS_RETURN(mbedtls_mpi_mod_mpi(N, N, &grp->P, diagnostics));
    }

    /* N->s < 0 is a much faster test, which fails only if N is 0 */
    if ((N->s < 0 && mbedtls_mpi_cmp_int(N, 0) != 0) ||
        mbedtls_mpi_bitlen(N) > 2 * grp->pbits) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    MBEDTLS_MPI_CHK(grp->modp(N, diagnostics));

    /* The previous call left N in the range [0, 2P) with no more limbs than P
     * (see documentation of mbedtls_ecp_mod_pXXX_raw() in ecp_invasive.h),
     * so we can bring it into the range [0, P) in constant time. */
    mbedtls_mpi_uint c = mbedtls_mpi_core_sub(N->p, N->p, grp->P.p, grp->P.n);
    (void) mbedtls_mpi_core_add_if(N->p, grp->P.p, grp->P.n, (unsigned) c);

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Fast mod-p functions expect their argument to be in the 0..p^2 range.
 *
 * In order to guarantee that, we need to ensure that operands of
 * mbedtls_mpi_mul_mpi are in the 0..p range. So, after each operation we will
 * bring the result back to this range.
 *
 * The following macros are shortcuts for doing that.
 */

/*
 * Reduce a mbedtls_mpi mod p in-place, general case, to use after mbedtls_mpi_mul_mpi
 */
#define INC_MUL_COUNT

#define MOD_MUL(N)                                                    \
    do                                                                  \
    {                                                                   \
        MBEDTLS_MPI_CHK(ecp_modp(&(N), grp, diagnostics));                       \
        INC_MUL_COUNT                                                   \
    } while (0)

int mbedtls_mpi_mul_mod(const mbedtls_ecp_group *grp,
                                      mbedtls_mpi *X,
                                      const mbedtls_mpi *A,
                                      const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(X, A, B, diagnostics));
    MOD_MUL(*X);
cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Reduce a mbedtls_mpi mod p in-place, to use after mbedtls_mpi_sub_mpi
 * N->s < 0 is a very fast test, which fails only if N is 0
 */
#define MOD_SUB(N)                                                          \
    do {                                                                      \
        while ((N)->s < 0 && mbedtls_mpi_cmp_int((N), 0) != 0)             \
        MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi((N), (N), &grp->P, diagnostics));      \
    } while (0)

MBEDTLS_MAYBE_UNUSED
int mbedtls_mpi_sub_mod(const mbedtls_ecp_group *grp,
                                      mbedtls_mpi *X,
                                      const mbedtls_mpi *A,
                                      const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(X, A, B, diagnostics));
    MOD_SUB(X);
cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Reduce a mbedtls_mpi mod p in-place, to use after mbedtls_mpi_add_mpi and mbedtls_mpi_mul_int.
 * We known P, N and the result are positive, so sub_abs is correct, and
 * a bit faster.
 */
#define MOD_ADD(N)                                                   \
    while (mbedtls_mpi_cmp_mpi((N), &grp->P) >= 0)                  \
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_abs((N), (N), &grp->P, diagnostics))

int mbedtls_mpi_add_mod(const mbedtls_ecp_group *grp,
                                      mbedtls_mpi *X,
                                      const mbedtls_mpi *A,
                                      const mbedtls_mpi *B, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(X, A, B, diagnostics));
    MOD_ADD(X);
cleanup:
    MINTLS_RETURN(ret);
}

MBEDTLS_MAYBE_UNUSED
int mbedtls_mpi_mul_int_mod(const mbedtls_ecp_group *grp,
                                          mbedtls_mpi *X,
                                          const mbedtls_mpi *A,
                                          mbedtls_mpi_uint c, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_int(X, A, c, diagnostics));
    MOD_ADD(X);
cleanup:
    MINTLS_RETURN(ret);
}

MBEDTLS_MAYBE_UNUSED
int mbedtls_mpi_sub_int_mod(const mbedtls_ecp_group *grp,
                                          mbedtls_mpi *X,
                                          const mbedtls_mpi *A,
                                          mbedtls_mpi_uint c, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(X, A, c, diagnostics));
    MOD_SUB(X);
cleanup:
    MINTLS_RETURN(ret);
}

#define MPI_ECP_SUB_INT(X, A, c)             \
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int_mod(grp, X, A, c, diagnostics))

MBEDTLS_MAYBE_UNUSED
int mbedtls_mpi_shift_l_mod(const mbedtls_ecp_group *grp,
                                          mbedtls_mpi *X,
                                          size_t count, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l(X, count, diagnostics));
    MOD_ADD(X);
cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Macro wrappers around ECP modular arithmetic
 *
 * Currently, these wrappers are defined via the bignum module.
 */

#define MPI_ECP_ADD(X, A, B)                                                  \
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mod(grp, X, A, B, diagnostics))

#define MPI_ECP_SUB(X, A, B)                                                  \
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mod(grp, X, A, B, diagnostics))

#define MPI_ECP_MUL(X, A, B)                                                  \
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mod(grp, X, A, B, diagnostics))

#define MPI_ECP_SQR(X, A)                                                     \
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mod(grp, X, A, A, diagnostics))

#define MPI_ECP_MUL_INT(X, A, c)                                              \
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_int_mod(grp, X, A, c, diagnostics))

#define MPI_ECP_INV(dst, src)                                                 \
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(NULL, (dst), (src), &grp->P, diagnostics))

#define MPI_ECP_MOV(X, A)                                                     \
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(X, A, diagnostics))

#define MPI_ECP_SHIFT_L(X, count)                                             \
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l_mod(grp, X, count, diagnostics))

#define MPI_ECP_LSET(X, c)                                                    \
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(X, c, diagnostics))

#define MPI_ECP_CMP_INT(X, c)                                                 \
    mbedtls_mpi_cmp_int(X, c)

#define MPI_ECP_CMP(X, Y)                                                     \
    mbedtls_mpi_cmp_mpi(X, Y)

/* Needs f_rng, p_rng to be defined. */
#define MPI_ECP_RAND(X)                                                       \
    MBEDTLS_MPI_CHK(mbedtls_mpi_random((X), 2, &grp->P, f_rng, p_rng, diagnostics))

/* Conditional negation
 * Needs grp and a temporary MPI tmp to be defined. */
#define MPI_ECP_COND_NEG(X, cond)                                        \
    do                                                                     \
    {                                                                      \
        unsigned char nonzero = mbedtls_mpi_cmp_int((X), 0) != 0;        \
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&tmp, &grp->P, (X), diagnostics));      \
        MBEDTLS_MPI_CHK(mbedtls_mpi_safe_cond_assign((X), &tmp,          \
                                                     nonzero & cond, diagnostics)); \
    } while (0)

#define MPI_ECP_NEG(X) MPI_ECP_COND_NEG((X), 1)

#define MPI_ECP_VALID(X)                      \
    ((X)->p != NULL)

#define MPI_ECP_COND_ASSIGN(X, Y, cond)       \
    MBEDTLS_MPI_CHK(mbedtls_mpi_safe_cond_assign((X), (Y), (cond), diagnostics))

#define MPI_ECP_COND_SWAP(X, Y, cond)       \
    MBEDTLS_MPI_CHK(mbedtls_mpi_safe_cond_swap((X), (Y), (cond), diagnostics))

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)

/*
 * Computes the right-hand side of the Short Weierstrass equation
 * RHS = X^3 + A X + B
 */
int ecp_sw_rhs(const mbedtls_ecp_group *grp,
                      mbedtls_mpi *rhs,
                      const mbedtls_mpi *X, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    /* Compute X^3 + A X + B as X (X^2 + A) + B */
    MPI_ECP_SQR(rhs, X);

    /* Special case for A = -3 */
    if (mbedtls_ecp_group_a_is_minus_3(grp)) {
        MPI_ECP_SUB_INT(rhs, rhs, 3);
    } else {
        MPI_ECP_ADD(rhs, rhs, &grp->A);
    }

    MPI_ECP_MUL(rhs, rhs, X);
    MPI_ECP_ADD(rhs, rhs, &grp->B);

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Derive Y from X and a parity bit
 */
int mbedtls_ecp_sw_derive_y(const mbedtls_ecp_group *grp,
                                   const mbedtls_mpi *X,
                                   mbedtls_mpi *Y,
                                   int parity_bit, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* w = y^2 = x^3 + ax + b
     * y = sqrt(w) = w^((p+1)/4) mod p   (for prime p where p = 3 mod 4)
     *
     * Note: this method for extracting square root does not validate that w
     * was indeed a square so this function will return garbage in Y if X
     * does not correspond to a point on the curve.
     */

    /* Check prerequisite p = 3 mod 4 */
    if (mbedtls_mpi_get_bit(&grp->P, 0) != 1 ||
        mbedtls_mpi_get_bit(&grp->P, 1) != 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
    }

    int ret;
    mbedtls_mpi exp;
    mbedtls_mpi_init(&exp);

    /* use Y to store intermediate result, actually w above */
    MBEDTLS_MPI_CHK(ecp_sw_rhs(grp, Y, X, diagnostics));

    /* w = y^2 */ /* Y contains y^2 intermediate result */
    /* exp = ((p+1)/4) */
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_int(&exp, &grp->P, 1, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_r(&exp, 2));
    /* sqrt(w) = w^((p+1)/4) mod p   (for prime p where p = 3 mod 4) */
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(Y, Y /*y^2*/, &exp, &grp->P, NULL, diagnostics));

    /* check parity bit match or else invert Y */
    /* This quick inversion implementation is valid because Y != 0 for all
     * Short Weierstrass curves supported by mbedtls, as each supported curve
     * has an order that is a large prime, so each supported curve does not
     * have any point of order 2, and a point with Y == 0 would be of order 2 */
    if (mbedtls_mpi_get_bit(Y, 0) != parity_bit) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(Y, &grp->P, Y, diagnostics));
    }

cleanup:

    mbedtls_mpi_free(&exp);
    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
/*
 * For curves in short Weierstrass form, we do all the internal operations in
 * Jacobian coordinates.
 *
 * For multiplication, we'll use a comb method with countermeasures against
 * SPA, hence timing attacks.
 */

/*
 * Normalize jacobian coordinates so that Z == 0 || Z == 1  (GECC 3.2.1)
 * Cost: 1N := 1I + 3M + 1S
 */
int ecp_normalize_jac(const mbedtls_ecp_group *grp, mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (MPI_ECP_CMP_INT(&pt->Z, 0) == 0) {
        MINTLS_RETURN(0);
    }

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp)) {
        MINTLS_RETURN(mbedtls_internal_ecp_normalize_jac(grp, pt));
    }
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_ALT */

#if defined(MBEDTLS_ECP_NO_FALLBACK) && defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT)
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
#else
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi T;
    mbedtls_mpi_init(&T);

    MPI_ECP_INV(&T,       &pt->Z);            /* T   <-          1 / Z   */
    MPI_ECP_MUL(&pt->Y,   &pt->Y,     &T);    /* Y'  <- Y*T    = Y / Z   */
    MPI_ECP_SQR(&T,       &T);                /* T   <- T^2    = 1 / Z^2 */
    MPI_ECP_MUL(&pt->X,   &pt->X,     &T);    /* X   <- X  * T = X / Z^2 */
    MPI_ECP_MUL(&pt->Y,   &pt->Y,     &T);    /* Y'' <- Y' * T = Y / Z^3 */

    MPI_ECP_LSET(&pt->Z, 1);

cleanup:

    mbedtls_mpi_free(&T);

    MINTLS_RETURN(ret);
#endif /* !defined(MBEDTLS_ECP_NO_FALLBACK) || !defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT) */
}

/*
 * Normalize jacobian coordinates of an array of (pointers to) points,
 * using Montgomery's trick to perform only one inversion mod P.
 * (See for example Cohen's "A Course in Computational Algebraic Number
 * Theory", Algorithm 10.3.4.)
 *
 * Warning: fails (returning an error) if one of the points is zero!
 * This should never happen, see choice of w in ecp_mul_comb().
 *
 * Cost: 1N(t) := 1I + (6t - 3)M + 1S
 */
int ecp_normalize_jac_many(const mbedtls_ecp_group *grp,
                                  mbedtls_ecp_point *T[], size_t T_size, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (T_size < 2) {
        MINTLS_RETURN(ecp_normalize_jac(grp, *T, diagnostics));
    }

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp)) {
        MINTLS_RETURN(mbedtls_internal_ecp_normalize_jac_many(grp, T, T_size));
    }
#endif

#if defined(MBEDTLS_ECP_NO_FALLBACK) && defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT)
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
#else
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i;
    mbedtls_mpi *c, t;

    if ((c = mbedtls_calloc(T_size, sizeof(mbedtls_mpi))) == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_ALLOC_FAILED);
    }

    mbedtls_mpi_init(&t);

    mpi_init_many(c, T_size);
    /*
     * c[i] = Z_0 * ... * Z_i,   i = 0,..,n := T_size-1
     */
    MPI_ECP_MOV(&c[0], &T[0]->Z);
    for (i = 1; i < T_size; i++) {
        MPI_ECP_MUL(&c[i], &c[i-1], &T[i]->Z);
    }

    /*
     * c[n] = 1 / (Z_0 * ... * Z_n) mod P
     */
    MPI_ECP_INV(&c[T_size-1], &c[T_size-1]);

    for (i = T_size - 1;; i--) {
        /* At the start of iteration i (note that i decrements), we have
         * - c[j] = Z_0 * .... * Z_j        for j  < i,
         * - c[j] = 1 / (Z_0 * .... * Z_j)  for j == i,
         *
         * This is maintained via
         * - c[i-1] <- c[i] * Z_i
         *
         * We also derive 1/Z_i = c[i] * c[i-1] for i>0 and use that
         * to do the actual normalization. For i==0, we already have
         * c[0] = 1 / Z_0.
         */

        if (i > 0) {
            /* Compute 1/Z_i and establish invariant for the next iteration. */
            MPI_ECP_MUL(&t,      &c[i], &c[i-1]);
            MPI_ECP_MUL(&c[i-1], &c[i], &T[i]->Z);
        } else {
            MPI_ECP_MOV(&t, &c[0]);
        }

        /* Now t holds 1 / Z_i; normalize as in ecp_normalize_jac() */
        MPI_ECP_MUL(&T[i]->Y, &T[i]->Y, &t);
        MPI_ECP_SQR(&t,       &t);
        MPI_ECP_MUL(&T[i]->X, &T[i]->X, &t);
        MPI_ECP_MUL(&T[i]->Y, &T[i]->Y, &t);

        /*
         * Post-precessing: reclaim some memory by shrinking coordinates
         * - not storing Z (always 1)
         * - shrinking other coordinates, but still keeping the same number of
         *   limbs as P, as otherwise it will too likely be regrown too fast.
         */
        MBEDTLS_MPI_CHK(mbedtls_mpi_shrink(&T[i]->X, grp->P.n, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_shrink(&T[i]->Y, grp->P.n, diagnostics));

        MPI_ECP_LSET(&T[i]->Z, 1);

        if (i == 0) {
            break;
        }
    }

cleanup:

    mbedtls_mpi_free(&t);
    mpi_free_many(c, T_size);
    mbedtls_free(c);

    MINTLS_RETURN(ret);
#endif /* !defined(MBEDTLS_ECP_NO_FALLBACK) || !defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT) */
}

/*
 * Conditional point inversion: Q -> -Q = (Q.X, -Q.Y, Q.Z) without leak.
 * "inv" must be 0 (don't invert) or 1 (invert) or the result will be invalid
 */
int ecp_safe_invert_jac(const mbedtls_ecp_group *grp,
                               mbedtls_ecp_point *Q,
                               unsigned char inv, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi tmp;
    mbedtls_mpi_init(&tmp);

    MPI_ECP_COND_NEG(&Q->Y, inv);

cleanup:
    mbedtls_mpi_free(&tmp);
    MINTLS_RETURN(ret);
}

/*
 * Point doubling R = 2 P, Jacobian coordinates
 *
 * Based on http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html#doubling-dbl-1998-cmo-2 .
 *
 * We follow the variable naming fairly closely. The formula variations that trade a MUL for a SQR
 * (plus a few ADDs) aren't useful as our bignum implementation doesn't distinguish squaring.
 *
 * Standard optimizations are applied when curve parameter A is one of { 0, -3 }.
 *
 * Cost: 1D := 3M + 4S          (A ==  0)
 *             4M + 4S          (A == -3)
 *             3M + 6S + 1a     otherwise
 */
int ecp_double_jac(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                          const mbedtls_ecp_point *P,
                          mbedtls_mpi tmp[4], MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


#if defined(MBEDTLS_ECP_DOUBLE_JAC_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp)) {
        MINTLS_RETURN(mbedtls_internal_ecp_double_jac(grp, R, P));
    }
#endif /* MBEDTLS_ECP_DOUBLE_JAC_ALT */

#if defined(MBEDTLS_ECP_NO_FALLBACK) && defined(MBEDTLS_ECP_DOUBLE_JAC_ALT)
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
#else
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* Special case for A = -3 */
    if (mbedtls_ecp_group_a_is_minus_3(grp)) {
        /* tmp[0] <- M = 3(X + Z^2)(X - Z^2) */
        MPI_ECP_SQR(&tmp[1],  &P->Z);
        MPI_ECP_ADD(&tmp[2],  &P->X,  &tmp[1]);
        MPI_ECP_SUB(&tmp[3],  &P->X,  &tmp[1]);
        MPI_ECP_MUL(&tmp[1],  &tmp[2],     &tmp[3]);
        MPI_ECP_MUL_INT(&tmp[0],  &tmp[1],     3);
    } else {
        /* tmp[0] <- M = 3.X^2 + A.Z^4 */
        MPI_ECP_SQR(&tmp[1],  &P->X);
        MPI_ECP_MUL_INT(&tmp[0],  &tmp[1],  3);

        /* Optimize away for "koblitz" curves with A = 0 */
        if (MPI_ECP_CMP_INT(&grp->A, 0) != 0) {
            /* M += A.Z^4 */
            MPI_ECP_SQR(&tmp[1],  &P->Z);
            MPI_ECP_SQR(&tmp[2],  &tmp[1]);
            MPI_ECP_MUL(&tmp[1],  &tmp[2],     &grp->A);
            MPI_ECP_ADD(&tmp[0],  &tmp[0],     &tmp[1]);
        }
    }

    /* tmp[1] <- S = 4.X.Y^2 */
    MPI_ECP_SQR(&tmp[2],  &P->Y);
    MPI_ECP_SHIFT_L(&tmp[2],  1);
    MPI_ECP_MUL(&tmp[1],  &P->X, &tmp[2]);
    MPI_ECP_SHIFT_L(&tmp[1],  1);

    /* tmp[3] <- U = 8.Y^4 */
    MPI_ECP_SQR(&tmp[3],  &tmp[2]);
    MPI_ECP_SHIFT_L(&tmp[3],  1);

    /* tmp[2] <- T = M^2 - 2.S */
    MPI_ECP_SQR(&tmp[2],  &tmp[0]);
    MPI_ECP_SUB(&tmp[2],  &tmp[2], &tmp[1]);
    MPI_ECP_SUB(&tmp[2],  &tmp[2], &tmp[1]);

    /* tmp[1] <- S = M(S - T) - U */
    MPI_ECP_SUB(&tmp[1],  &tmp[1],     &tmp[2]);
    MPI_ECP_MUL(&tmp[1],  &tmp[1],     &tmp[0]);
    MPI_ECP_SUB(&tmp[1],  &tmp[1],     &tmp[3]);

    /* tmp[3] <- U = 2.Y.Z */
    MPI_ECP_MUL(&tmp[3],  &P->Y,  &P->Z);
    MPI_ECP_SHIFT_L(&tmp[3],  1);

    /* Store results */
    MPI_ECP_MOV(&R->X, &tmp[2]);
    MPI_ECP_MOV(&R->Y, &tmp[1]);
    MPI_ECP_MOV(&R->Z, &tmp[3]);

cleanup:

    MINTLS_RETURN(ret);
#endif /* !defined(MBEDTLS_ECP_NO_FALLBACK) || !defined(MBEDTLS_ECP_DOUBLE_JAC_ALT) */
}

/*
 * Addition: R = P + Q, mixed affine-Jacobian coordinates (GECC 3.22)
 *
 * The coordinates of Q must be normalized (= affine),
 * but those of P don't need to. R is not normalized.
 *
 * P,Q,R may alias, but only at the level of EC points: they must be either
 * equal as pointers, or disjoint (including the coordinate data buffers).
 * Fine-grained aliasing at the level of coordinates is not supported.
 *
 * Special cases: (1) P or Q is zero, (2) R is zero, (3) P == Q.
 * None of these cases can happen as intermediate step in ecp_mul_comb():
 * - at each step, P, Q and R are multiples of the base point, the factor
 *   being less than its order, so none of them is zero;
 * - Q is an odd multiple of the base point, P an even multiple,
 *   due to the choice of precomputed points in the modified comb method.
 * So branches for these cases do not leak secret information.
 *
 * Cost: 1A := 8M + 3S
 */
int ecp_add_mixed(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                         const mbedtls_ecp_point *P, const mbedtls_ecp_point *Q,
                         mbedtls_mpi tmp[4], MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


#if defined(MBEDTLS_ECP_ADD_MIXED_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp)) {
        MINTLS_RETURN(mbedtls_internal_ecp_add_mixed(grp, R, P, Q));
    }
#endif /* MBEDTLS_ECP_ADD_MIXED_ALT */

#if defined(MBEDTLS_ECP_NO_FALLBACK) && defined(MBEDTLS_ECP_ADD_MIXED_ALT)
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
#else
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* NOTE: Aliasing between input and output is allowed, so one has to make
     *       sure that at the point X,Y,Z are written, {P,Q}->{X,Y,Z} are no
     *       longer read from. */
    mbedtls_mpi * const X = &R->X;
    mbedtls_mpi * const Y = &R->Y;
    mbedtls_mpi * const Z = &R->Z;

    if (!MPI_ECP_VALID(&Q->Z)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /*
     * Trivial cases: P == 0 or Q == 0 (case 1)
     */
    if (MPI_ECP_CMP_INT(&P->Z, 0) == 0) {
        MINTLS_RETURN(mbedtls_ecp_copy(R, Q, diagnostics));
    }

    if (MPI_ECP_CMP_INT(&Q->Z, 0) == 0) {
        MINTLS_RETURN(mbedtls_ecp_copy(R, P, diagnostics));
    }

    /*
     * Make sure Q coordinates are normalized
     */
    if (MPI_ECP_CMP_INT(&Q->Z, 1) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    MPI_ECP_SQR(&tmp[0], &P->Z);
    MPI_ECP_MUL(&tmp[1], &tmp[0], &P->Z);
    MPI_ECP_MUL(&tmp[0], &tmp[0], &Q->X);
    MPI_ECP_MUL(&tmp[1], &tmp[1], &Q->Y);
    MPI_ECP_SUB(&tmp[0], &tmp[0], &P->X);
    MPI_ECP_SUB(&tmp[1], &tmp[1], &P->Y);

    /* Special cases (2) and (3) */
    if (MPI_ECP_CMP_INT(&tmp[0], 0) == 0) {
        if (MPI_ECP_CMP_INT(&tmp[1], 0) == 0) {
            ret = ecp_double_jac(grp, R, P, tmp, diagnostics);
            goto cleanup;
        } else {
            ret = mbedtls_ecp_set_zero(R, diagnostics);
            goto cleanup;
        }
    }

    /* {P,Q}->Z no longer used, so OK to write to Z even if there's aliasing. */
    MPI_ECP_MUL(Z,        &P->Z,    &tmp[0]);
    MPI_ECP_SQR(&tmp[2],  &tmp[0]);
    MPI_ECP_MUL(&tmp[3],  &tmp[2],  &tmp[0]);
    MPI_ECP_MUL(&tmp[2],  &tmp[2],  &P->X);

    MPI_ECP_MOV(&tmp[0], &tmp[2]);
    MPI_ECP_SHIFT_L(&tmp[0], 1);

    /* {P,Q}->X no longer used, so OK to write to X even if there's aliasing. */
    MPI_ECP_SQR(X,        &tmp[1]);
    MPI_ECP_SUB(X,        X,        &tmp[0]);
    MPI_ECP_SUB(X,        X,        &tmp[3]);
    MPI_ECP_SUB(&tmp[2],  &tmp[2],  X);
    MPI_ECP_MUL(&tmp[2],  &tmp[2],  &tmp[1]);
    MPI_ECP_MUL(&tmp[3],  &tmp[3],  &P->Y);
    /* {P,Q}->Y no longer used, so OK to write to Y even if there's aliasing. */
    MPI_ECP_SUB(Y,     &tmp[2],     &tmp[3]);

cleanup:

    MINTLS_RETURN(ret);
#endif /* !defined(MBEDTLS_ECP_NO_FALLBACK) || !defined(MBEDTLS_ECP_ADD_MIXED_ALT) */
}

/*
 * Randomize jacobian coordinates:
 * (X, Y, Z) -> (l^2 X, l^3 Y, l Z) for random l
 * This is sort of the reverse operation of ecp_normalize_jac().
 *
 * This countermeasure was first suggested in [2].
 */
int ecp_randomize_jac(const mbedtls_ecp_group *grp, mbedtls_ecp_point *pt,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp)) {
        MINTLS_RETURN(mbedtls_internal_ecp_randomize_jac(grp, pt, f_rng, p_rng));
    }
#endif /* MBEDTLS_ECP_RANDOMIZE_JAC_ALT */

#if defined(MBEDTLS_ECP_NO_FALLBACK) && defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT)
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
#else
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi l;

    mbedtls_mpi_init(&l);

    /* Generate l such that 1 < l < p */
    MPI_ECP_RAND(&l);

    /* Z' = l * Z */
    MPI_ECP_MUL(&pt->Z,   &pt->Z,     &l);

    /* Y' = l * Y */
    MPI_ECP_MUL(&pt->Y,   &pt->Y,     &l);

    /* X' = l^2 * X */
    MPI_ECP_SQR(&l,       &l);
    MPI_ECP_MUL(&pt->X,   &pt->X,     &l);

    /* Y'' = l^2 * Y' = l^3 * Y */
    MPI_ECP_MUL(&pt->Y,   &pt->Y,     &l);

cleanup:
    mbedtls_mpi_free(&l);

    if (ret == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_RANDOM_FAILED);
    }
    MINTLS_RETURN(ret);
#endif /* !defined(MBEDTLS_ECP_NO_FALLBACK) || !defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT) */
}

/*
 * Check and define parameters used by the comb method (see below for details)
 */
#if MBEDTLS_ECP_WINDOW_SIZE < 2 || MBEDTLS_ECP_WINDOW_SIZE > 7
#error "MBEDTLS_ECP_WINDOW_SIZE out of bounds"
#endif

/* d = ceil( n / w ) */
#define COMB_MAX_D      (MBEDTLS_ECP_MAX_BITS + 1) / 2

/* number of precomputed points */
#define COMB_MAX_PRE    (1 << (MBEDTLS_ECP_WINDOW_SIZE - 1))

/*
 * Compute the representation of m that will be used with our comb method.
 *
 * The basic comb method is described in GECC 3.44 for example. We use a
 * modified version that provides resistance to SPA by avoiding zero
 * digits in the representation as in [3]. We modify the method further by
 * requiring that all K_i be odd, which has the small cost that our
 * representation uses one more K_i, due to carries, but saves on the size of
 * the precomputed table.
 *
 * Summary of the comb method and its modifications:
 *
 * - The goal is to compute m*P for some w*d-bit integer m.
 *
 * - The basic comb method splits m into the w-bit integers
 *   x[0] .. x[d-1] where x[i] consists of the bits in m whose
 *   index has residue i modulo d, and computes m * P as
 *   S[x[0]] + 2 * S[x[1]] + .. + 2^(d-1) S[x[d-1]], where
 *   S[i_{w-1} .. i_0] := i_{w-1} 2^{(w-1)d} P + ... + i_1 2^d P + i_0 P.
 *
 * - If it happens that, say, x[i+1]=0 (=> S[x[i+1]]=0), one can replace the sum by
 *    .. + 2^{i-1} S[x[i-1]] - 2^i S[x[i]] + 2^{i+1} S[x[i]] + 2^{i+2} S[x[i+2]] ..,
 *   thereby successively converting it into a form where all summands
 *   are nonzero, at the cost of negative summands. This is the basic idea of [3].
 *
 * - More generally, even if x[i+1] != 0, we can first transform the sum as
 *   .. - 2^i S[x[i]] + 2^{i+1} ( S[x[i]] + S[x[i+1]] ) + 2^{i+2} S[x[i+2]] ..,
 *   and then replace S[x[i]] + S[x[i+1]] = S[x[i] ^ x[i+1]] + 2 S[x[i] & x[i+1]].
 *   Performing and iterating this procedure for those x[i] that are even
 *   (keeping track of carry), we can transform the original sum into one of the form
 *   S[x'[0]] +- 2 S[x'[1]] +- .. +- 2^{d-1} S[x'[d-1]] + 2^d S[x'[d]]
 *   with all x'[i] odd. It is therefore only necessary to know S at odd indices,
 *   which is why we are only computing half of it in the first place in
 *   ecp_precompute_comb and accessing it with index abs(i) / 2 in ecp_select_comb.
 *
 * - For the sake of compactness, only the seven low-order bits of x[i]
 *   are used to represent its absolute value (K_i in the paper), and the msb
 *   of x[i] encodes the sign (s_i in the paper): it is set if and only if
 *   if s_i == -1;
 *
 * Calling conventions:
 * - x is an array of size d + 1
 * - w is the size, ie number of teeth, of the comb, and must be between
 *   2 and 7 (in practice, between 2 and MBEDTLS_ECP_WINDOW_SIZE)
 * - m is the MPI, expected to be odd and such that bitlength(m) <= w * d
 *   (the result will be incorrect if these assumptions are not satisfied)
 */
void ecp_comb_recode_core(unsigned char x[], size_t d,
                                 unsigned char w, const mbedtls_mpi *m)
{
    size_t i, j;
    unsigned char c, cc, adjust;

    memset(x, 0, d+1);

    /* First get the classical comb values (except for x_d = 0) */
    for (i = 0; i < d; i++) {
        for (j = 0; j < w; j++) {
            x[i] |= mbedtls_mpi_get_bit(m, i + d * j) << j;
        }
    }

    /* Now make sure x_1 .. x_d are odd */
    c = 0;
    for (i = 1; i <= d; i++) {
        /* Add carry and update it */
        cc   = x[i] & c;
        x[i] = x[i] ^ c;
        c = cc;

        /* Adjust if needed, avoiding branches */
        adjust = 1 - (x[i] & 0x01);
        c   |= x[i] & (x[i-1] * adjust);
        x[i] = x[i] ^ (x[i-1] * adjust);
        x[i-1] |= adjust << 7;
    }
}

/*
 * Precompute points for the adapted comb method
 *
 * Assumption: T must be able to hold 2^{w - 1} elements.
 *
 * Operation: If i = i_{w-1} ... i_1 is the binary representation of i,
 *            sets T[i] = i_{w-1} 2^{(w-1)d} P + ... + i_1 2^d P + P.
 *
 * Cost: d(w-1) D + (2^{w-1} - 1) A + 1 N(w-1) + 1 N(2^{w-1} - 1)
 *
 * Note: Even comb values (those where P would be omitted from the
 *       sum defining T[i] above) are not needed in our adaption
 *       the comb method. See ecp_comb_recode_core().
 *
 * This function currently works in four steps:
 * (1) [dbl]      Computation of intermediate T[i] for 2-power values of i
 * (2) [norm_dbl] Normalization of coordinates of these T[i]
 * (3) [add]      Computation of all T[i]
 * (4) [norm_add] Normalization of all T[i]
 *
 * Step 1 can be interrupted but not the others; together with the final
 * coordinate normalization they are the largest steps done at once, depending
 * on the window size. Here are operation counts for P-256:
 *
 * step     (2)     (3)     (4)
 * w = 5    142     165     208
 * w = 4    136      77     160
 * w = 3    130      33     136
 * w = 2    124      11     124
 *
 * So if ECC operations are blocking for too long even with a low max_ops
 * value, it's useful to set MBEDTLS_ECP_WINDOW_SIZE to a lower value in order
 * to minimize maximum blocking time.
 */
int ecp_precompute_comb(const mbedtls_ecp_group *grp,
                               mbedtls_ecp_point T[], const mbedtls_ecp_point *P,
                               unsigned char w, size_t d,
                               mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char i;
    size_t j = 0;
    const unsigned char T_size = 1U << (w - 1);
    mbedtls_ecp_point *cur, *TT[COMB_MAX_PRE - 1] = { NULL };

    mbedtls_mpi tmp[4];

    mpi_init_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    (void) rs_ctx;

    /*
     * Set T[0] = P and
     * T[2^{l-1}] = 2^{dl} P for l = 1 .. w-1 (this is not the final value)
     */
    MBEDTLS_MPI_CHK(mbedtls_ecp_copy(&T[0], P, diagnostics));

    j = 0;

    for (; j < d * (w - 1); j++) {
        MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_DBL);

        i = 1U << (j / d);
        cur = T + i;

        if (j % d == 0) {
            MBEDTLS_MPI_CHK(mbedtls_ecp_copy(cur, T + (i >> 1), diagnostics));
        }

        MBEDTLS_MPI_CHK(ecp_double_jac(grp, cur, cur, tmp, diagnostics));
    }

    /*
     * Normalize current elements in T to allow them to be used in
     * ecp_add_mixed() below, which requires one normalized input.
     *
     * As T has holes, use an auxiliary array of pointers to elements in T.
     *
     */
    j = 0;
    for (i = 1; i < T_size; i <<= 1) {
        TT[j++] = T + i;
    }

    MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_INV + 6 * j - 2);

    MBEDTLS_MPI_CHK(ecp_normalize_jac_many(grp, TT, j, diagnostics));

    /*
     * Compute the remaining ones using the minimal number of additions
     * Be careful to update T[2^l] only after using it!
     */
    MBEDTLS_ECP_BUDGET((T_size - 1) * MBEDTLS_ECP_OPS_ADD);

    for (i = 1; i < T_size; i <<= 1) {
        j = i;
        while (j--) {
            MBEDTLS_MPI_CHK(ecp_add_mixed(grp, &T[i + j], &T[j], &T[i], tmp, diagnostics));
        }
    }

    /*
     * Normalize final elements in T. Even though there are no holes now, we
     * still need the auxiliary array for homogeneity with the previous
     * call. Also, skip T[0] which is already normalised, being a copy of P.
     */
    for (j = 0; j + 1 < T_size; j++) {
        TT[j] = T + j + 1;
    }

    MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_INV + 6 * j - 2);

    MBEDTLS_MPI_CHK(ecp_normalize_jac_many(grp, TT, j, diagnostics));

    /* Free Z coordinate (=1 after normalization) to save RAM.
     * This makes T[i] invalid as mbedtls_ecp_points, but this is OK
     * since from this point onwards, they are only accessed indirectly
     * via the getter function ecp_select_comb() which does set the
     * target's Z coordinate to 1. */
    for (i = 0; i < T_size; i++) {
        mbedtls_mpi_free(&T[i].Z);
    }

cleanup:

    mpi_free_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    MINTLS_RETURN(ret);
}

/*
 * Select precomputed point: R = sign(i) * T[ abs(i) / 2 ]
 *
 * See ecp_comb_recode_core() for background
 */
int ecp_select_comb(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                           const mbedtls_ecp_point T[], unsigned char T_size,
                           unsigned char i, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char ii, j;

    /* Ignore the "sign" bit and scale down */
    ii =  (i & 0x7Fu) >> 1;

    /* Read the whole table to thwart cache-based timing attacks */
    for (j = 0; j < T_size; j++) {
        MPI_ECP_COND_ASSIGN(&R->X, &T[j].X, j == ii);
        MPI_ECP_COND_ASSIGN(&R->Y, &T[j].Y, j == ii);
    }

    /* Safely invert result if i is "negative" */
    MBEDTLS_MPI_CHK(ecp_safe_invert_jac(grp, R, i >> 7, diagnostics));

    MPI_ECP_LSET(&R->Z, 1);

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Core multiplication algorithm for the (modified) comb method.
 * This part is actually common with the basic comb method (GECC 3.44)
 *
 * Cost: d A + d D + 1 R
 */
int ecp_mul_comb_core(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                             const mbedtls_ecp_point T[], unsigned char T_size,
                             const unsigned char x[], size_t d,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                             void *p_rng,
                             mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_point Txi;
    mbedtls_mpi tmp[4];
    size_t i;

    mbedtls_ecp_point_init(&Txi);
    mpi_init_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    (void) rs_ctx;

    {
        /* Start with a non-zero point and randomize its coordinates */
        i = d;
        MBEDTLS_MPI_CHK(ecp_select_comb(grp, R, T, T_size, x[i], diagnostics));
        if (f_rng != 0) {
            MBEDTLS_MPI_CHK(ecp_randomize_jac(grp, R, f_rng, p_rng, diagnostics));
        }
    }

    while (i != 0) {
        MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_DBL + MBEDTLS_ECP_OPS_ADD);
        --i;

        MBEDTLS_MPI_CHK(ecp_double_jac(grp, R, R, tmp, diagnostics));
        MBEDTLS_MPI_CHK(ecp_select_comb(grp, &Txi, T, T_size, x[i], diagnostics));
        MBEDTLS_MPI_CHK(ecp_add_mixed(grp, R, R, &Txi, tmp, diagnostics));
    }

cleanup:

    mbedtls_ecp_point_free(&Txi);
    mpi_free_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    MINTLS_RETURN(ret);
}

/*
 * Recode the scalar to get constant-time comb multiplication
 *
 * As the actual scalar recoding needs an odd scalar as a starting point,
 * this wrapper ensures that by replacing m by N - m if necessary, and
 * informs the caller that the result of multiplication will be negated.
 *
 * This works because we only support large prime order for Short Weierstrass
 * curves, so N is always odd hence either m or N - m is.
 *
 * See ecp_comb_recode_core() for background.
 */
int ecp_comb_recode_scalar(const mbedtls_ecp_group *grp,
                                  const mbedtls_mpi *m,
                                  unsigned char k[COMB_MAX_D + 1],
                                  size_t d,
                                  unsigned char w,
                                  unsigned char *parity_trick, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi M, mm;

    mbedtls_mpi_init(&M);
    mbedtls_mpi_init(&mm);

    /* N is always odd (see above), just make extra sure */
    if (mbedtls_mpi_get_bit(&grp->N, 0) != 1) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /* do we need the parity trick? */
    *parity_trick = (mbedtls_mpi_get_bit(m, 0) == 0);

    /* execute parity fix in constant time */
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&M, m, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&mm, &grp->N, m, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_safe_cond_assign(&M, &mm, *parity_trick, diagnostics));

    /* actual scalar recoding */
    ecp_comb_recode_core(k, d, w, &M);

cleanup:
    mbedtls_mpi_free(&mm);
    mbedtls_mpi_free(&M);

    MINTLS_RETURN(ret);
}

/*
 * Perform comb multiplication (for short Weierstrass curves)
 * once the auxiliary table has been pre-computed.
 *
 * Scalar recoding may use a parity trick that makes us compute -m * P,
 * if that is the case we'll need to recover m * P at the end.
 */
int ecp_mul_comb_after_precomp(const mbedtls_ecp_group *grp,
                                      mbedtls_ecp_point *R,
                                      const mbedtls_mpi *m,
                                      const mbedtls_ecp_point *T,
                                      unsigned char T_size,
                                      unsigned char w,
                                      size_t d,
                                      int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                      void *p_rng,
                                      mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char parity_trick;
    unsigned char k[COMB_MAX_D + 1];
    mbedtls_ecp_point *RR = R;

    MBEDTLS_MPI_CHK(ecp_comb_recode_scalar(grp, m, k, d, w,
                                           &parity_trick, diagnostics));
    MBEDTLS_MPI_CHK(ecp_mul_comb_core(grp, RR, T, T_size, k, d,
                                      f_rng, p_rng, rs_ctx, diagnostics));
    MBEDTLS_MPI_CHK(ecp_safe_invert_jac(grp, RR, parity_trick, diagnostics));

    MBEDTLS_MPI_CHK(ecp_normalize_jac(grp, RR, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Pick window size based on curve size and whether we optimize for base point
 */
unsigned char ecp_pick_window_size(const mbedtls_ecp_group *grp,
                                          unsigned char p_eq_g)
{
    unsigned char w;

    /*
     * Minimize the number of multiplications, that is minimize
     * 10 * d * w + 18 * 2^(w-1) + 11 * d + 7 * w, with d = ceil( nbits / w )
     * (see costs of the various parts, with 1S = 1M)
     */
    w = grp->nbits >= 384 ? 5 : 4;

    /*
     * If P == G, pre-compute a bit more, since this may be re-used later.
     * Just adding one avoids upping the cost of the first mul too much,
     * and the memory cost too.
     */
    if (p_eq_g) {
        w++;
    }

    /*
     * If static comb table may not be used (!p_eq_g) or static comb table does
     * not exists, make sure w is within bounds.
     * (The last test is useful only for very small curves in the test suite.)
     *
     * The user reduces MBEDTLS_ECP_WINDOW_SIZE does not changes the size of
     * static comb table, because the size of static comb table is fixed when
     * it is generated.
     */
#if (MBEDTLS_ECP_WINDOW_SIZE < 6)
    if ((!p_eq_g || !ecp_group_is_static_comb_table(grp)) && w > MBEDTLS_ECP_WINDOW_SIZE) {
        w = MBEDTLS_ECP_WINDOW_SIZE;
    }
#endif
    if (w >= grp->nbits) {
        w = 2;
    }

    return w;
}

/*
 * Multiplication using the comb method - for curves in short Weierstrass form
 *
 * This function is mainly responsible for administrative work:
 * - managing the restart context if enabled
 * - managing the table of precomputed points (passed between the below two
 *   functions): allocation, computation, ownership transfer, freeing.
 *
 * It delegates the actual arithmetic work to:
 *      ecp_precompute_comb() and ecp_mul_comb_with_precomp()
 *
 * See comments on ecp_comb_recode_core() regarding the computation strategy.
 */
int ecp_mul_comb(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                        const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng,
                        mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char w, p_eq_g, i;
    size_t d;
    unsigned char T_size = 0, T_ok = 0;
    mbedtls_ecp_point *T = NULL;

    ECP_RS_ENTER(rsm);

    /* Is P the base point ? */
#if MBEDTLS_ECP_FIXED_POINT_OPTIM == 1
    p_eq_g = (MPI_ECP_CMP(&P->Y, &grp->G.Y) == 0 &&
              MPI_ECP_CMP(&P->X, &grp->G.X) == 0);
#else
    p_eq_g = 0;
#endif

    /* Pick window size and deduce related sizes */
    w = ecp_pick_window_size(grp, p_eq_g);
    T_size = 1U << (w - 1);
    d = (grp->nbits + w - 1) / w;

    /* Pre-computed table: do we have it already for the base point? */
    if (p_eq_g && grp->T != NULL) {
        /* second pointer to the same table, will be deleted on exit */
        T = grp->T;
        T_ok = 1;
    } else
    /* Allocate table if we didn't have any */
    {
        T = mbedtls_calloc(T_size, sizeof(mbedtls_ecp_point));
        if (T == NULL) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_ALLOC_FAILED);
            goto cleanup;
        }

        for (i = 0; i < T_size; i++) {
            mbedtls_ecp_point_init(&T[i]);
        }

        T_ok = 0;
    }

    /* Compute table (or finish computing it) if not done already */
    if (!T_ok) {
        MBEDTLS_MPI_CHK(ecp_precompute_comb(grp, T, P, w, d, rs_ctx, diagnostics));

        if (p_eq_g) {
            /* almost transfer ownership of T to the group, but keep a copy of
             * the pointer to use for calling the next function more easily */
            grp->T = T;
            grp->T_size = T_size;
        }
    }

    /* Actual comb multiplication using precomputed points */
    MBEDTLS_MPI_CHK(ecp_mul_comb_after_precomp(grp, R, m,
                                               T, T_size, w, d,
                                               f_rng, p_rng, rs_ctx, diagnostics));

cleanup:

    /* does T belong to the group? */
    if (T == grp->T) {
        T = NULL;
    }

    /* does T belong to the restart context? */

    /* did T belong to us? then let's destroy it! */
    if (T != NULL) {
        for (i = 0; i < T_size; i++) {
            mbedtls_ecp_point_free(&T[i]);
        }
        mbedtls_free(T);
    }

    /* prevent caller from using invalid value */
    int should_free_R = (ret != 0);
    if (should_free_R) {
        mbedtls_ecp_point_free(R);
    }

    ECP_RS_LEAVE(rsm);

    MINTLS_RETURN(ret);
}

#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

/*
 * Restartable multiplication R = m * P
 *
 * This internal function can be called without an RNG in case where we know
 * the inputs are not sensitive.
 */
int ecp_mul_restartable_internal(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                                        const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng,
                                        mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

    (void) rs_ctx;

    int restarting = 0;
    /* skip argument check when restarting */
    if (!restarting) {
        /* check_privkey is free */
        MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_CHK);

        /* Common sanity checks */
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_privkey(grp, m, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_pubkey(grp, P, diagnostics));
    }

    ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MBEDTLS_MPI_CHK(ecp_mul_comb(grp, R, m, P, f_rng, p_rng, rs_ctx, diagnostics));
    }
#endif

cleanup:

    MINTLS_RETURN(ret);
}

/*
 * Restartable multiplication R = m * P
 */
int mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                                const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng,
                                mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(ecp_mul_restartable_internal(grp, R, m, P, f_rng, p_rng, rs_ctx, diagnostics));
}

/*
 * Multiplication R = m * P
 */
int mbedtls_ecp_mul(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                    const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, NULL, diagnostics));
}

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
/*
 * Check that an affine point is valid as a public key,
 * short weierstrass curves (SEC1 3.2.3.1)
 */
int ecp_check_pubkey_sw(const mbedtls_ecp_group *grp, const mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi YY, RHS;

    /* pt coordinates must be normalized for our checks */
    if (mbedtls_mpi_cmp_int(&pt->X, 0) < 0 ||
        mbedtls_mpi_cmp_int(&pt->Y, 0) < 0 ||
        mbedtls_mpi_cmp_mpi(&pt->X, &grp->P) >= 0 ||
        mbedtls_mpi_cmp_mpi(&pt->Y, &grp->P) >= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_INVALID_KEY);
    }

    mbedtls_mpi_init(&YY); mbedtls_mpi_init(&RHS);

    /*
     * YY = Y^2
     * RHS = X^3 + A X + B
     */
    MPI_ECP_SQR(&YY,  &pt->Y);
    MBEDTLS_MPI_CHK(ecp_sw_rhs(grp, &RHS, &pt->X, diagnostics));

    if (MPI_ECP_CMP(&YY, &RHS) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_INVALID_KEY);
    }

cleanup:

    mbedtls_mpi_free(&YY); mbedtls_mpi_free(&RHS);

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
/*
 * R = m * P with shortcuts for m == 0, m == 1 and m == -1
 * NOT constant-time - ONLY for short Weierstrass!
 */
int mbedtls_ecp_mul_shortcuts(mbedtls_ecp_group *grp,
                                     mbedtls_ecp_point *R,
                                     const mbedtls_mpi *m,
                                     const mbedtls_ecp_point *P,
                                     mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi tmp;
    mbedtls_mpi_init(&tmp);

    if (mbedtls_mpi_cmp_int(m, 0) == 0) {
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_pubkey(grp, P, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_ecp_set_zero(R, diagnostics));
    } else if (mbedtls_mpi_cmp_int(m, 1) == 0) {
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_pubkey(grp, P, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_ecp_copy(R, P, diagnostics));
    } else if (mbedtls_mpi_cmp_int(m, -1) == 0) {
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_pubkey(grp, P, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_ecp_copy(R, P, diagnostics));
        MPI_ECP_NEG(&R->Y);
    } else {
        MBEDTLS_MPI_CHK(ecp_mul_restartable_internal(grp, R, m, P,
                                                     NULL, NULL, rs_ctx, diagnostics));
    }

cleanup:
    mbedtls_mpi_free(&tmp);

    MINTLS_RETURN(ret);
}

/*
 * Restartable linear combination
 * NOT constant-time
 */
int mbedtls_ecp_muladd_restartable(
    mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
    const mbedtls_mpi *m, const mbedtls_ecp_point *P,
    const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
    mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_point mP;
    mbedtls_ecp_point *pmP = &mP;
    mbedtls_ecp_point *pR = R;
    mbedtls_mpi tmp[4];
    if (mbedtls_ecp_get_type(grp) != MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
    }

    mbedtls_ecp_point_init(&mP);
    mpi_init_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    ECP_RS_ENTER(ma);

    MBEDTLS_MPI_CHK(mbedtls_ecp_mul_shortcuts(grp, pmP, m, P, rs_ctx, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_ecp_mul_shortcuts(grp, pR,  n, Q, rs_ctx, diagnostics));

    MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_ADD);
    MBEDTLS_MPI_CHK(ecp_add_mixed(grp, pR, pmP, pR, tmp, diagnostics));
    MBEDTLS_ECP_BUDGET(MBEDTLS_ECP_OPS_INV);
    MBEDTLS_MPI_CHK(ecp_normalize_jac(grp, pR, diagnostics));

cleanup:

    mpi_free_many(tmp, sizeof(tmp) / sizeof(mbedtls_mpi));

    mbedtls_ecp_point_free(&mP);

    ECP_RS_LEAVE(ma);

    MINTLS_RETURN(ret);
}

/*
 * Linear combination
 * NOT constant-time
 */
int mbedtls_ecp_muladd(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                       const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                       const mbedtls_mpi *n, const mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_muladd_restartable(grp, R, m, P, n, Q, NULL, diagnostics));
}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

/*
 * Check that a point is valid as a public key
 */
int mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
                             const mbedtls_ecp_point *pt, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* Must use affine coordinates */
    if (mbedtls_mpi_cmp_int(&pt->Z, 1) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_INVALID_KEY);
    }

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MINTLS_RETURN(ecp_check_pubkey_sw(grp, pt, diagnostics));
    }
#endif
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
}

/*
 * Check that an mbedtls_mpi is valid as a private key
 */
int mbedtls_ecp_check_privkey(const mbedtls_ecp_group *grp,
                              const mbedtls_mpi *d, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        /* see SEC1 3.2 */
        if (mbedtls_mpi_cmp_int(d, 1) < 0 ||
            mbedtls_mpi_cmp_mpi(d, &grp->N) >= 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_INVALID_KEY);
        } else {
            MINTLS_RETURN(0);
        }
    }
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
}

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
int mbedtls_ecp_gen_privkey_sw(
    const mbedtls_mpi *N, mbedtls_mpi *d,
    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = mbedtls_mpi_random(d, 1, N, f_rng, p_rng, diagnostics);
    switch (ret) {
        case MBEDTLS_ERR_MPI_NOT_ACCEPTABLE:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_RANDOM_FAILED);
        default:
            MINTLS_RETURN(ret);
    }
}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

/*
 * Generate a private key
 */
int mbedtls_ecp_gen_privkey(const mbedtls_ecp_group *grp,
                            mbedtls_mpi *d,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();


#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MINTLS_RETURN(mbedtls_ecp_gen_privkey_sw(&grp->N, d, f_rng, p_rng, diagnostics));
    }
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
}

/*
 * Generate a keypair with configurable base point
 */
int mbedtls_ecp_gen_keypair_base(mbedtls_ecp_group *grp,
                                 const mbedtls_ecp_point *G,
                                 mbedtls_mpi *d, mbedtls_ecp_point *Q,
                                 int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                 void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    MBEDTLS_MPI_CHK(mbedtls_ecp_gen_privkey(grp, d, f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_ecp_mul(grp, Q, d, G, f_rng, p_rng, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Generate key pair, wrapper for conventional base point
 */
int mbedtls_ecp_gen_keypair(mbedtls_ecp_group *grp,
                            mbedtls_mpi *d, mbedtls_ecp_point *Q,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_gen_keypair_base(grp, &grp->G, d, Q, f_rng, p_rng, diagnostics));
}

/*
 * Generate a keypair, prettier wrapper
 */
int mbedtls_ecp_gen_key(mbedtls_ecp_group_id grp_id, mbedtls_ecp_keypair *key,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    if ((ret = mbedtls_ecp_group_load(&key->grp, grp_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(mbedtls_ecp_gen_keypair(&key->grp, &key->d, &key->Q, f_rng, p_rng, diagnostics));
}

int mbedtls_ecp_set_public_key(mbedtls_ecp_group_id grp_id,
                               mbedtls_ecp_keypair *key,
                               const mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (key->grp.id == MBEDTLS_ECP_DP_NONE) {
        /* Group not set yet */
        if ((ret = mbedtls_ecp_group_load(&key->grp, grp_id, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }
    } else if (key->grp.id != grp_id) {
        /* Group mismatch */
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
    MINTLS_RETURN(mbedtls_ecp_copy(&key->Q, Q, diagnostics));
}

#define ECP_CURVE25519_KEY_SIZE 32
#define ECP_CURVE448_KEY_SIZE   56
/*
 * Read a private key.
 */
int mbedtls_ecp_read_key(mbedtls_ecp_group_id grp_id, mbedtls_ecp_keypair *key,
                         const unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;

    if ((ret = mbedtls_ecp_group_load(&key->grp, grp_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(&key->grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&key->d, buf, buflen, diagnostics));
    }
#endif

    if (ret == 0) {
        MBEDTLS_MPI_CHK(mbedtls_ecp_check_privkey(&key->grp, &key->d, diagnostics));
    }

cleanup:

    if (ret != 0) {
        mbedtls_mpi_free(&key->d);
    }

    MINTLS_RETURN(ret);
}

/*
 * Write a private key.
 */
#if !defined MBEDTLS_DEPRECATED_REMOVED
int mbedtls_ecp_write_key(mbedtls_ecp_keypair *key,
                          unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(&key->grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&key->d, buf, buflen, diagnostics));
    }

#endif
cleanup:

    MINTLS_RETURN(ret);
}
#endif /* MBEDTLS_DEPRECATED_REMOVED */

int mbedtls_ecp_write_key_ext(const mbedtls_ecp_keypair *key,
                              size_t *olen, unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t len = (key->grp.nbits + 7) / 8;
    if (len > buflen) {
        /* For robustness, ensure *olen <= buflen even on error. */
        *olen = 0;
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
    }
    *olen = len;

    /* Private key not set */
    if (key->d.n == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

#if defined(MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED)
    if (mbedtls_ecp_get_type(&key->grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
        MINTLS_RETURN(mbedtls_mpi_write_binary(&key->d, buf, len, diagnostics));
    }
#endif

    /* Private key set but no recognized curve type? This shouldn't happen. */
    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED);
}

/*
 * Write a public key.
 */
int mbedtls_ecp_write_public_key(const mbedtls_ecp_keypair *key,
                                 int format, size_t *olen,
                                 unsigned char *buf, size_t buflen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_point_write_binary(&key->grp, &key->Q,
                                          format, olen, buf, buflen, diagnostics));
}

/*
 * Check a public-private key pair
 */
int mbedtls_ecp_check_pub_priv(
    const mbedtls_ecp_keypair *pub, const mbedtls_ecp_keypair *prv,
    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_point Q;
    mbedtls_ecp_group grp;
    if (pub->grp.id == MBEDTLS_ECP_DP_NONE ||
        pub->grp.id != prv->grp.id ||
        mbedtls_mpi_cmp_mpi(&pub->Q.X, &prv->Q.X) ||
        mbedtls_mpi_cmp_mpi(&pub->Q.Y, &prv->Q.Y) ||
        mbedtls_mpi_cmp_mpi(&pub->Q.Z, &prv->Q.Z)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    mbedtls_ecp_point_init(&Q);
    mbedtls_ecp_group_init(&grp);

    /* mbedtls_ecp_mul() needs a non-const group... */
    MBEDTLS_MPI_CHK(mbedtls_ecp_group_copy(&grp, &prv->grp, diagnostics));

    /* Also checks d is valid */
    MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &Q, &prv->d, &prv->grp.G, f_rng, p_rng, diagnostics));

    if (mbedtls_mpi_cmp_mpi(&Q.X, &prv->Q.X) ||
        mbedtls_mpi_cmp_mpi(&Q.Y, &prv->Q.Y) ||
        mbedtls_mpi_cmp_mpi(&Q.Z, &prv->Q.Z)) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
        goto cleanup;
    }

cleanup:
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);

    MINTLS_RETURN(ret);
}

int mbedtls_ecp_keypair_calc_public(mbedtls_ecp_keypair *key,
                                    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                    void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_mul(&key->grp, &key->Q, &key->d, &key->grp.G,
                           f_rng, p_rng, diagnostics));
}

mbedtls_ecp_group_id mbedtls_ecp_keypair_get_group_id(
    const mbedtls_ecp_keypair *key)
{
    return key->grp.id;
}

/*
 * Export generic key-pair parameters.
 */
int mbedtls_ecp_export(const mbedtls_ecp_keypair *key, mbedtls_ecp_group *grp,
                       mbedtls_mpi *d, mbedtls_ecp_point *Q, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (grp != NULL && (ret = mbedtls_ecp_group_copy(grp, &key->grp, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (d != NULL && (ret = mbedtls_mpi_copy(d, &key->d, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (Q != NULL && (ret = mbedtls_ecp_copy(Q, &key->Q, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}
