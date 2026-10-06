// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Elliptic curve DSA
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ecdsa.c.
 */

/*
 * References:
 *
 * SEC1 https://www.secg.org/sec1-v2.pdf
 */

#include "common.h"

#include "ecdsa.h"
#include "asn1write.h"
#include "bignum_internal.h"

#include <string.h>

#include "platform.h"

#include "platform_util.h"
#include "error.h"

#define ECDSA_RS_ECP    NULL

#define ECDSA_BUDGET(ops)     /* no-op; for compatibility */

#define ECDSA_RS_ENTER(SUB)   (void) rs_ctx
#define ECDSA_RS_LEAVE(SUB)   (void) rs_ctx

#if defined(MBEDTLS_ECDSA_DETERMINISTIC) || \
    !defined(MBEDTLS_ECDSA_SIGN_ALT)     || \
    !defined(MBEDTLS_ECDSA_VERIFY_ALT)
/*
 * Derive a suitable integer for group grp from a buffer of length len
 * SEC1 4.1.3 step 5 aka SEC1 4.1.4 step 3
 */
int derive_mpi(const mbedtls_ecp_group *grp, mbedtls_mpi *x,
                      const unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t n_size = (grp->nbits + 7) / 8;
    size_t use_size = blen > n_size ? n_size : blen;

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(x, buf, use_size, diagnostics));
    if (use_size * 8 > grp->nbits) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_shift_r(x, use_size * 8 - grp->nbits));
    }

    /* While at it, reduce modulo N */
    if (mbedtls_mpi_cmp_mpi(x, &grp->N) >= 0) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(x, x, &grp->N, diagnostics));
    }

cleanup:
    MINTLS_RETURN(ret);
}
#endif /* ECDSA_DETERMINISTIC || !ECDSA_SIGN_ALT || !ECDSA_VERIFY_ALT */

int mbedtls_ecdsa_can_do(mbedtls_ecp_group_id gid)
{
    switch (gid) {
        default: return 1;
    }
}

#if !defined(MBEDTLS_ECDSA_SIGN_ALT)
/*
 * Compute ECDSA signature of a hashed message (SEC1 4.1.3)
 * Obviously, compared to SEC1 4.1.3, we skip step 4 (hash message)
 */
int mbedtls_ecdsa_sign_restartable(mbedtls_ecp_group *grp,
                                   mbedtls_mpi *r, mbedtls_mpi *s,
                                   const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                                   int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng,
                                   int (*f_rng_blind)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                   void *p_rng_blind,
                                   mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, key_tries, sign_tries;
    int *p_sign_tries = &sign_tries, *p_key_tries = &key_tries;
    mbedtls_ecp_point R;
    mbedtls_mpi k, e;
    mbedtls_mpi *pk = &k, *pr = r;

    /* Fail cleanly on curves such as Curve25519 that can't be used for ECDSA */
    if (!mbedtls_ecdsa_can_do(grp->id) || grp->N.p == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    /* Make sure d is in range 1..n-1 */
    if (mbedtls_mpi_cmp_int(d, 1) < 0 || mbedtls_mpi_cmp_mpi(d, &grp->N) >= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_INVALID_KEY);
    }

    mbedtls_ecp_point_init(&R);
    mbedtls_mpi_init(&k); mbedtls_mpi_init(&e);

    ECDSA_RS_ENTER(sig);

    *p_sign_tries = 0;
    do {
        if ((*p_sign_tries)++ > 10) {
            ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_RANDOM_FAILED);
            goto cleanup;
        }

        /*
         * Steps 1-3: generate a suitable ephemeral keypair
         * and set r = xR mod n
         */
        *p_key_tries = 0;
        do {
            if ((*p_key_tries)++ > 10) {
                ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_RANDOM_FAILED);
                goto cleanup;
            }

            MBEDTLS_MPI_CHK(mbedtls_ecp_gen_privkey(grp, pk, f_rng, p_rng, diagnostics));

            MBEDTLS_MPI_CHK(mbedtls_ecp_mul_restartable(grp, &R, pk, &grp->G,
                                                        f_rng_blind,
                                                        p_rng_blind,
                                                        ECDSA_RS_ECP, diagnostics));
            MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(pr, &R.X, &grp->N, diagnostics));
        } while (mbedtls_mpi_cmp_int(pr, 0) == 0);

        /*
         * Accounting for everything up to the end of the loop
         * (step 6, but checking now avoids saving e and t)
         */
        ECDSA_BUDGET(MBEDTLS_ECP_OPS_INV + 4);

        /*
         * Step 5: derive MPI from hashed message
         */
        MBEDTLS_MPI_CHK(derive_mpi(grp, &e, buf, blen, diagnostics));

        /*
         * Step 6: compute s = (e + r * d) / k
         */
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(s, pr, d, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&e, &e, s, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(NULL, s, pk, &grp->N, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(s, s, &e, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(s, s, &grp->N, diagnostics));
    } while (mbedtls_mpi_cmp_int(s, 0) == 0);

cleanup:
    mbedtls_ecp_point_free(&R);
    mbedtls_mpi_free(&k); mbedtls_mpi_free(&e);

    ECDSA_RS_LEAVE(sig);

    MINTLS_RETURN(ret);
}

/*
 * Compute ECDSA signature of a hashed message
 */
int mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                       const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                       int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    /* Use the same RNG for both blinding and ephemeral key generation */
    MINTLS_RETURN(mbedtls_ecdsa_sign_restartable(grp, r, s, d, buf, blen,
                                          f_rng, p_rng, f_rng, p_rng, NULL, diagnostics));
}
#endif /* !MBEDTLS_ECDSA_SIGN_ALT */

#if !defined(MBEDTLS_ECDSA_VERIFY_ALT)
/*
 * Verify ECDSA signature of hashed message (SEC1 4.1.4)
 * Obviously, compared to SEC1 4.1.3, we skip step 2 (hash message)
 */
int mbedtls_ecdsa_verify_restartable(mbedtls_ecp_group *grp,
                                     const unsigned char *buf, size_t blen,
                                     const mbedtls_ecp_point *Q,
                                     const mbedtls_mpi *r,
                                     const mbedtls_mpi *s,
                                     mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi e, s_inv, u1, u2;
    mbedtls_ecp_point R;
    mbedtls_mpi *pu1 = &u1, *pu2 = &u2;

    mbedtls_ecp_point_init(&R);
    mbedtls_mpi_init(&e); mbedtls_mpi_init(&s_inv);
    mbedtls_mpi_init(&u1); mbedtls_mpi_init(&u2);

    /* Fail cleanly on curves such as Curve25519 that can't be used for ECDSA */
    if (!mbedtls_ecdsa_can_do(grp->id) || grp->N.p == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    ECDSA_RS_ENTER(ver);

    /*
     * Step 1: make sure r and s are in range 1..n-1
     */
    if (mbedtls_mpi_cmp_int(r, 1) < 0 || mbedtls_mpi_cmp_mpi(r, &grp->N) >= 0 ||
        mbedtls_mpi_cmp_int(s, 1) < 0 || mbedtls_mpi_cmp_mpi(s, &grp->N) >= 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_VERIFY_FAILED);
        goto cleanup;
    }

    /*
     * Step 3: derive MPI from hashed message
     */
    MBEDTLS_MPI_CHK(derive_mpi(grp, &e, buf, blen, diagnostics));

    /*
     * Step 4: u1 = e / s mod n, u2 = r / s mod n
     */
    ECDSA_BUDGET(MBEDTLS_ECP_OPS_CHK + MBEDTLS_ECP_OPS_INV + 2);

    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(NULL, &s_inv, s, &grp->N, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(pu1, &e, &s_inv, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(pu1, pu1, &grp->N, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(pu2, r, &s_inv, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(pu2, pu2, &grp->N, diagnostics));

    /*
     * Step 5: R = u1 G + u2 Q
     */
    MBEDTLS_MPI_CHK(mbedtls_ecp_muladd_restartable(grp,
                                                   &R, pu1, &grp->G, pu2, Q, ECDSA_RS_ECP, diagnostics));

    if (mbedtls_ecp_is_zero(&R)) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_VERIFY_FAILED);
        goto cleanup;
    }

    /*
     * Step 6: convert xR to an integer (no-op)
     * Step 7: reduce xR mod n (gives v)
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&R.X, &R.X, &grp->N, diagnostics));

    /*
     * Step 8: check if v (that is, R.X) is equal to r
     */
    if (mbedtls_mpi_cmp_mpi(&R.X, r) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_VERIFY_FAILED);
        goto cleanup;
    }

cleanup:
    mbedtls_ecp_point_free(&R);
    mbedtls_mpi_free(&e); mbedtls_mpi_free(&s_inv);
    mbedtls_mpi_free(&u1); mbedtls_mpi_free(&u2);

    ECDSA_RS_LEAVE(ver);

    MINTLS_RETURN(ret);
}

/*
 * Verify ECDSA signature of hashed message
 */
int mbedtls_ecdsa_verify(mbedtls_ecp_group *grp,
                         const unsigned char *buf, size_t blen,
                         const mbedtls_ecp_point *Q,
                         const mbedtls_mpi *r,
                         const mbedtls_mpi *s, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecdsa_verify_restartable(grp, buf, blen, Q, r, s, NULL, diagnostics));
}
#endif /* !MBEDTLS_ECDSA_VERIFY_ALT */

/*
 * Convert a signature (given by context) to ASN.1
 */
int ecdsa_signature_to_asn1(const mbedtls_mpi *r, const mbedtls_mpi *s,
                                   unsigned char *sig, size_t sig_size,
                                   size_t *slen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char buf[MBEDTLS_ECDSA_MAX_LEN] = { 0 };
    unsigned char *p = buf + sizeof(buf);
    size_t len = 0;

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_mpi(&p, buf, s, diagnostics));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_mpi(&p, buf, r, diagnostics));

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&p, buf, len, diagnostics));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&p, buf,
                                                     MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE, diagnostics));

    if (len > sig_size) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
    }

    memcpy(sig, p, len);
    *slen = len;

    MINTLS_RETURN(0);
}

/*
 * Compute and write signature
 */
int mbedtls_ecdsa_write_signature_restartable(mbedtls_ecdsa_context *ctx,
                                              mbedtls_md_type_t md_alg,
                                              const unsigned char *hash, size_t hlen,
                                              unsigned char *sig, size_t sig_size, size_t *slen,
                                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                              void *p_rng,
                                              mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi r, s;
    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    (void) md_alg;

#if defined(MBEDTLS_ECDSA_SIGN_ALT)
    (void) rs_ctx;

    MBEDTLS_MPI_CHK(mbedtls_ecdsa_sign(&ctx->grp, &r, &s, &ctx->d,
                                       hash, hlen, f_rng, p_rng, diagnostics));
#else
    /* Use the same RNG for both blinding and ephemeral key generation */
    MBEDTLS_MPI_CHK(mbedtls_ecdsa_sign_restartable(&ctx->grp, &r, &s, &ctx->d,
                                                   hash, hlen, f_rng, p_rng, f_rng,
                                                   p_rng, rs_ctx, diagnostics));
#endif /* MBEDTLS_ECDSA_SIGN_ALT */

    MBEDTLS_MPI_CHK(ecdsa_signature_to_asn1(&r, &s, sig, sig_size, slen, diagnostics));

cleanup:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    MINTLS_RETURN(ret);
}

/*
 * Compute and write signature
 */
int mbedtls_ecdsa_write_signature(mbedtls_ecdsa_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hlen,
                                  unsigned char *sig, size_t sig_size, size_t *slen,
                                  int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                  void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecdsa_write_signature_restartable(
        ctx, md_alg, hash, hlen, sig, sig_size, slen,
        f_rng, p_rng, NULL, diagnostics));
}

/*
 * Read and check signature
 */
int mbedtls_ecdsa_read_signature(mbedtls_ecdsa_context *ctx,
                                 const unsigned char *hash, size_t hlen,
                                 const unsigned char *sig, size_t slen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecdsa_read_signature_restartable(
        ctx, hash, hlen, sig, slen, NULL, diagnostics));
}

/*
 * Restartable read and check signature
 */
int mbedtls_ecdsa_read_signature_restartable(mbedtls_ecdsa_context *ctx,
                                             const unsigned char *hash, size_t hlen,
                                             const unsigned char *sig, size_t slen,
                                             mbedtls_ecdsa_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    size_t len;
    mbedtls_mpi r, s;
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if (p + len != end) {
        ret = MBEDTLS_ERROR_ADD(MBEDTLS_ERR_ECP_BAD_INPUT_DATA,
                                MBEDTLS_ERR_ASN1_LENGTH_MISMATCH);
        goto cleanup;
    }

    if ((ret = mbedtls_asn1_get_mpi(&p, end, &r, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_get_mpi(&p, end, &s, diagnostics)) != 0) {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }
#if defined(MBEDTLS_ECDSA_VERIFY_ALT)
    (void) rs_ctx;

    if ((ret = mbedtls_ecdsa_verify(&ctx->grp, hash, hlen,
                                    &ctx->Q, &r, &s, diagnostics)) != 0) {
        goto cleanup;
    }
#else
    if ((ret = mbedtls_ecdsa_verify_restartable(&ctx->grp, hash, hlen,
                                                &ctx->Q, &r, &s, rs_ctx, diagnostics)) != 0) {
        goto cleanup;
    }
#endif /* MBEDTLS_ECDSA_VERIFY_ALT */

    /* At this point we know that the buffer starts with a valid signature.
     * Return 0 if the buffer just contains the signature, and a specific
     * error code if the valid signature is followed by more data. */
    if (p != end) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH);
    }

cleanup:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    MINTLS_RETURN(ret);
}

#if !defined(MBEDTLS_ECDSA_GENKEY_ALT)
/*
 * Generate key pair
 */
int mbedtls_ecdsa_genkey(mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
                         int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    ret = mbedtls_ecp_group_load(&ctx->grp, gid, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(mbedtls_ecp_gen_keypair(&ctx->grp, &ctx->d,
                                   &ctx->Q, f_rng, p_rng, diagnostics));
}
#endif /* !MBEDTLS_ECDSA_GENKEY_ALT */

/*
 * Set context from an mbedtls_ecp_keypair
 */
int mbedtls_ecdsa_from_keypair(mbedtls_ecdsa_context *ctx, const mbedtls_ecp_keypair *key, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    if ((ret = mbedtls_ecp_group_copy(&ctx->grp, &key->grp, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_copy(&ctx->d, &key->d, diagnostics)) != 0 ||
        (ret = mbedtls_ecp_copy(&ctx->Q, &key->Q, diagnostics)) != 0) {
        mbedtls_ecdsa_free(ctx);
    }

    MINTLS_RETURN(ret);
}

/*
 * Initialize context
 */
void mbedtls_ecdsa_init(mbedtls_ecdsa_context *ctx)
{
    mbedtls_ecp_keypair_init(ctx);
}

/*
 * Free context
 */
void mbedtls_ecdsa_free(mbedtls_ecdsa_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_ecp_keypair_free(ctx);
}

