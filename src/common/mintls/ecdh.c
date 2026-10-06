// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  Elliptic curve Diffie-Hellman
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/ecdh.c.
 */

/*
 * References:
 *
 * SEC1 https://www.secg.org/sec1-v2.pdf
 * RFC 4492
 */

#include "common.h"

#include "ecdh.h"
#include "platform_util.h"
#include "error.h"

#include <string.h>

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
typedef mbedtls_ecdh_context mbedtls_ecdh_context_mbed;
#endif

mbedtls_ecp_group_id mbedtls_ecdh_grp_id(
    const mbedtls_ecdh_context *ctx)
{
#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    return ctx->grp.id;
#else
    return ctx->grp_id;
#endif
}

int mbedtls_ecdh_can_do(mbedtls_ecp_group_id gid)
{
    /* At this time, all groups support ECDH. */
    (void) gid;
    return 1;
}

#if !defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
/*
 * Generate public key (restartable version)
 *
 * Note: this internal function relies on its caller preserving the value of
 * the output parameter 'd' across continuation calls. This would not be
 * acceptable for a public function but is OK here as we control call sites.
 */
int ecdh_gen_public_restartable(mbedtls_ecp_group *grp,
                                       mbedtls_mpi *d, mbedtls_ecp_point *Q,
                                       int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                       void *p_rng,
                                       mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    int restarting = 0;
    /* If multiplication is in progress, we already generated a privkey */
    if (!restarting) {
        MBEDTLS_MPI_CHK(mbedtls_ecp_gen_privkey(grp, d, f_rng, p_rng, diagnostics));
    }

    MBEDTLS_MPI_CHK(mbedtls_ecp_mul_restartable(grp, Q, d, &grp->G,
                                                f_rng, p_rng, rs_ctx, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Generate public key
 */
int mbedtls_ecdh_gen_public(mbedtls_ecp_group *grp, mbedtls_mpi *d, mbedtls_ecp_point *Q,
                            int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                            void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ecdh_gen_public_restartable(grp, d, Q, f_rng, p_rng, NULL, diagnostics));
}
#endif /* !MBEDTLS_ECDH_GEN_PUBLIC_ALT */

#if !defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
/*
 * Compute shared secret (SEC1 3.3.1)
 */
int ecdh_compute_shared_restartable(mbedtls_ecp_group *grp,
                                           mbedtls_mpi *z,
                                           const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                                           int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                           void *p_rng,
                                           mbedtls_ecp_restart_ctx *rs_ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_point P;

    mbedtls_ecp_point_init(&P);

    MBEDTLS_MPI_CHK(mbedtls_ecp_mul_restartable(grp, &P, d, Q,
                                                f_rng, p_rng, rs_ctx, diagnostics));

    if (mbedtls_ecp_is_zero(&P)) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
        goto cleanup;
    }

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(z, &P.X, diagnostics));

cleanup:
    mbedtls_ecp_point_free(&P);

    MINTLS_RETURN(ret);
}

/*
 * Compute shared secret (SEC1 3.3.1)
 */
int mbedtls_ecdh_compute_shared(mbedtls_ecp_group *grp, mbedtls_mpi *z,
                                const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(ecdh_compute_shared_restartable(grp, z, Q, d,
                                           f_rng, p_rng, NULL, diagnostics));
}
#endif /* !MBEDTLS_ECDH_COMPUTE_SHARED_ALT */

void ecdh_init_internal(mbedtls_ecdh_context_mbed *ctx)
{
    mbedtls_ecp_group_init(&ctx->grp);
    mbedtls_mpi_init(&ctx->d);
    mbedtls_ecp_point_init(&ctx->Q);
    mbedtls_ecp_point_init(&ctx->Qp);
    mbedtls_mpi_init(&ctx->z);

}

mbedtls_ecp_group_id mbedtls_ecdh_get_grp_id(mbedtls_ecdh_context *ctx)
{
#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    return ctx->MBEDTLS_PRIVATE(grp).id;
#else
    return ctx->MBEDTLS_PRIVATE(grp_id);
#endif
}

/*
 * Initialize context
 */
void mbedtls_ecdh_init(mbedtls_ecdh_context *ctx)
{
#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    ecdh_init_internal(ctx);
    mbedtls_ecp_point_init(&ctx->Vi);
    mbedtls_ecp_point_init(&ctx->Vf);
    mbedtls_mpi_init(&ctx->_d);
#else
    memset(ctx, 0, sizeof(mbedtls_ecdh_context));

    ctx->var = MBEDTLS_ECDH_VARIANT_NONE;
#endif
    ctx->point_format = MBEDTLS_ECP_PF_UNCOMPRESSED;
}

int ecdh_setup_internal(mbedtls_ecdh_context_mbed *ctx,
                               mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    ret = mbedtls_ecp_group_load(&ctx->grp, grp_id, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE);
    }

    MINTLS_RETURN(0);
}

/*
 * Setup context
 */
int mbedtls_ecdh_setup(mbedtls_ecdh_context *ctx, mbedtls_ecp_group_id grp_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_setup_internal(ctx, grp_id, diagnostics));
#else
    switch (grp_id) {
        default:
            ctx->point_format = MBEDTLS_ECP_PF_UNCOMPRESSED;
            ctx->var = MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0;
            ctx->grp_id = grp_id;
            ecdh_init_internal(&ctx->ctx.mbed_ecdh);
            MINTLS_RETURN(ecdh_setup_internal(&ctx->ctx.mbed_ecdh, grp_id, diagnostics));
    }
#endif
}

void ecdh_free_internal(mbedtls_ecdh_context_mbed *ctx)
{
    mbedtls_ecp_group_free(&ctx->grp);
    mbedtls_mpi_free(&ctx->d);
    mbedtls_ecp_point_free(&ctx->Q);
    mbedtls_ecp_point_free(&ctx->Qp);
    mbedtls_mpi_free(&ctx->z);

}

/*
 * Free context
 */
void mbedtls_ecdh_free(mbedtls_ecdh_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    mbedtls_ecp_point_free(&ctx->Vi);
    mbedtls_ecp_point_free(&ctx->Vf);
    mbedtls_mpi_free(&ctx->_d);
    ecdh_free_internal(ctx);
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            ecdh_free_internal(&ctx->ctx.mbed_ecdh);
            break;
        default:
            break;
    }

    ctx->point_format = MBEDTLS_ECP_PF_UNCOMPRESSED;
    ctx->var = MBEDTLS_ECDH_VARIANT_NONE;
    ctx->grp_id = MBEDTLS_ECP_DP_NONE;
#endif
}

int ecdh_make_params_internal(mbedtls_ecdh_context_mbed *ctx,
                                     size_t *olen, int point_format,
                                     unsigned char *buf, size_t blen,
                                     int (*f_rng)(void *,
                                                  unsigned char *,
                                                  size_t, MinTlsDiagnostics* diagnostics),
                                     void *p_rng,
                                     int restart_enabled, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t grp_len, pt_len;

    if (ctx->grp.pbits == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    (void) restart_enabled;

    if ((ret = mbedtls_ecdh_gen_public(&ctx->grp, &ctx->d, &ctx->Q,
                                       f_rng, p_rng, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_ecp_tls_write_group(&ctx->grp, &grp_len, buf,
                                           blen, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    buf += grp_len;
    blen -= grp_len;

    if ((ret = mbedtls_ecp_tls_write_point(&ctx->grp, &ctx->Q, point_format,
                                           &pt_len, buf, blen, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    *olen = grp_len + pt_len;
    MINTLS_RETURN(0);
}

/*
 * Setup and write the ServerKeyExchange parameters (RFC 4492)
 *      struct {
 *          ECParameters    curve_params;
 *          ECPoint         public;
 *      } ServerECDHParams;
 */
int mbedtls_ecdh_make_params(mbedtls_ecdh_context *ctx, size_t *olen,
                             unsigned char *buf, size_t blen,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                             void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int restart_enabled = 0;
    (void) restart_enabled;

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_make_params_internal(ctx, olen, ctx->point_format, buf, blen,
                                     f_rng, p_rng, restart_enabled, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_make_params_internal(&ctx->ctx.mbed_ecdh, olen,
                                             ctx->point_format, buf, blen,
                                             f_rng, p_rng,
                                             restart_enabled, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}

int ecdh_read_params_internal(mbedtls_ecdh_context_mbed *ctx,
                                     const unsigned char **buf,
                                     const unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(mbedtls_ecp_tls_read_point(&ctx->grp, &ctx->Qp, buf,
                                      (size_t) (end - *buf), diagnostics));
}

/*
 * Read the ServerKeyExchange parameters (RFC 4492)
 *      struct {
 *          ECParameters    curve_params;
 *          ECPoint         public;
 *      } ServerECDHParams;
 */
int mbedtls_ecdh_read_params(mbedtls_ecdh_context *ctx,
                             const unsigned char **buf,
                             const unsigned char *end, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ecp_group_id grp_id;
    if ((ret = mbedtls_ecp_tls_read_group_id(&grp_id, buf, (size_t) (end - *buf), diagnostics))
        != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_ecdh_setup(ctx, grp_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_read_params_internal(ctx, buf, end, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_read_params_internal(&ctx->ctx.mbed_ecdh,
                                             buf, end, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}

int ecdh_get_params_internal(mbedtls_ecdh_context_mbed *ctx,
                                    const mbedtls_ecp_keypair *key,
                                    mbedtls_ecdh_side side, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    /* If it's not our key, just import the public part as Qp */
    if (side == MBEDTLS_ECDH_THEIRS) {
        MINTLS_RETURN(mbedtls_ecp_copy(&ctx->Qp, &key->Q, diagnostics));
    }

    /* Our key: import public (as Q) and private parts */
    if (side != MBEDTLS_ECDH_OURS) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    if ((ret = mbedtls_ecp_copy(&ctx->Q, &key->Q, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_copy(&ctx->d, &key->d, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}

/*
 * Get parameters from a keypair
 */
int mbedtls_ecdh_get_params(mbedtls_ecdh_context *ctx,
                            const mbedtls_ecp_keypair *key,
                            mbedtls_ecdh_side side, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    if (side != MBEDTLS_ECDH_OURS && side != MBEDTLS_ECDH_THEIRS) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    if (mbedtls_ecdh_grp_id(ctx) == MBEDTLS_ECP_DP_NONE) {
        /* This is the first call to get_params(). Set up the context
         * for use with the group. */
        if ((ret = mbedtls_ecdh_setup(ctx, key->grp.id, diagnostics)) != 0) {
            MINTLS_RETURN(ret);
        }
    } else {
        /* This is not the first call to get_params(). Check that the
         * current key's group is the same as the context's, which was set
         * from the first key's group. */
        if (mbedtls_ecdh_grp_id(ctx) != key->grp.id) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
        }
    }

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_get_params_internal(ctx, key, side, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_get_params_internal(&ctx->ctx.mbed_ecdh,
                                            key, side, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}

int ecdh_make_public_internal(mbedtls_ecdh_context_mbed *ctx,
                                     size_t *olen, int point_format,
                                     unsigned char *buf, size_t blen,
                                     int (*f_rng)(void *,
                                                  unsigned char *,
                                                  size_t, MinTlsDiagnostics* diagnostics),
                                     void *p_rng,
                                     int restart_enabled, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ctx->grp.pbits == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    (void) restart_enabled;

    if ((ret = mbedtls_ecdh_gen_public(&ctx->grp, &ctx->d, &ctx->Q,
                                       f_rng, p_rng, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(mbedtls_ecp_tls_write_point(&ctx->grp, &ctx->Q, point_format, olen,
                                       buf, blen, diagnostics));
}

/*
 * Setup and export the client public value
 */
int mbedtls_ecdh_make_public(mbedtls_ecdh_context *ctx, size_t *olen,
                             unsigned char *buf, size_t blen,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                             void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int restart_enabled = 0;

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_make_public_internal(ctx, olen, ctx->point_format, buf, blen,
                                     f_rng, p_rng, restart_enabled, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_make_public_internal(&ctx->ctx.mbed_ecdh, olen,
                                             ctx->point_format, buf, blen,
                                             f_rng, p_rng,
                                             restart_enabled, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}

int ecdh_read_public_internal(mbedtls_ecdh_context_mbed *ctx,
                                     const unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const unsigned char *p = buf;

    if ((ret = mbedtls_ecp_tls_read_point(&ctx->grp, &ctx->Qp, &p,
                                          blen, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((size_t) (p - buf) != blen) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(0);
}

/*
 * Parse and import the client's public value
 */
int mbedtls_ecdh_read_public(mbedtls_ecdh_context *ctx,
                             const unsigned char *buf, size_t blen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_read_public_internal(ctx, buf, blen, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_read_public_internal(&ctx->ctx.mbed_ecdh,
                                             buf, blen, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}

int ecdh_calc_secret_internal(mbedtls_ecdh_context_mbed *ctx,
                                     size_t *olen, unsigned char *buf,
                                     size_t blen,
                                     int (*f_rng)(void *,
                                                  unsigned char *,
                                                  size_t, MinTlsDiagnostics* diagnostics),
                                     void *p_rng,
                                     int restart_enabled, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if (ctx == NULL || ctx->grp.pbits == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }

    (void) restart_enabled;

    if ((ret = mbedtls_ecdh_compute_shared(&ctx->grp, &ctx->z, &ctx->Qp,
                                           &ctx->d, f_rng, p_rng, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    size_t p_bytes = ctx->grp.pbits / 8 + ((ctx->grp.pbits % 8) != 0);

    if (p_bytes > blen) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL);
    }

    *olen = p_bytes;

    if (mbedtls_ecp_get_type(&ctx->grp) == MBEDTLS_ECP_TYPE_MONTGOMERY) {
        MINTLS_RETURN(mbedtls_mpi_write_binary_le(&ctx->z, buf, *olen, diagnostics));
    }

    MINTLS_RETURN(mbedtls_mpi_write_binary(&ctx->z, buf, *olen, diagnostics));
}

/*
 * Derive and export the shared secret
 */
int mbedtls_ecdh_calc_secret(mbedtls_ecdh_context *ctx, size_t *olen,
                             unsigned char *buf, size_t blen,
                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                             void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int restart_enabled = 0;

#if defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    MINTLS_RETURN(ecdh_calc_secret_internal(ctx, olen, buf, blen, f_rng, p_rng,
                                     restart_enabled, diagnostics));
#else
    switch (ctx->var) {
        case MBEDTLS_ECDH_VARIANT_MBEDTLS_2_0:
            MINTLS_RETURN(ecdh_calc_secret_internal(&ctx->ctx.mbed_ecdh, olen, buf,
                                             blen, f_rng, p_rng,
                                             restart_enabled, diagnostics));
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_ECP_BAD_INPUT_DATA);
    }
#endif
}
