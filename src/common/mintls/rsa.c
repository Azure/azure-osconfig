// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/rsa.c.
 */

/*
 *  The following sources were referenced in the design of this implementation
 *  of the RSA algorithm:
 *
 *  [1] A method for obtaining digital signatures and public-key cryptosystems
 *      R Rivest, A Shamir, and L Adleman
 *      http://people.csail.mit.edu/rivest/pubs.html#RSA78
 *
 *  [2] Handbook of Applied Cryptography - 1997, Chapter 8
 *      Menezes, van Oorschot and Vanstone
 *
 *  [3] Malware Guard Extension: Using SGX to Conceal Cache Attacks
 *      Michael Schwarz, Samuel Weiser, Daniel Gruss, Clémentine Maurice and
 *      Stefan Mangard
 *      https://arxiv.org/abs/1702.08719v2
 *
 */

#include "common.h"

#include "rsa.h"
#include "bignum_core.h"
#include "bignum_internal.h"
#include "rsa_alt_helpers.h"
#include "rsa_internal.h"
#include "oid.h"
#include "asn1write.h"
#include "platform_util.h"
#include "error.h"
#include "constant_time_internal.h"
#include "constant_time.h"

#include <string.h>

#if defined(MBEDTLS_PKCS1_V15) && !defined(__OpenBSD__) && !defined(__NetBSD__)
#include <stdlib.h>
#endif

#include "platform.h"

/*
 * Wrapper around mbedtls_asn1_get_mpi() that rejects zero.
 *
 * The value zero is:
 * - never a valid value for an RSA parameter
 * - interpreted as "omitted, please reconstruct" by mbedtls_rsa_complete().
 *
 * Since values can't be omitted in PKCS#1, passing a zero value to
 * rsa_complete() would be incorrect, so reject zero values early.
 */
int asn1_get_nonzero_mpi(unsigned char **p,
                                const unsigned char *end,
                                mbedtls_mpi *X, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    ret = mbedtls_asn1_get_mpi(p, end, X, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    if (mbedtls_mpi_cmp_int(X, 0) == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(0);
}

int mbedtls_rsa_parse_key(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret, version;
    size_t len;
    unsigned char *p, *end;

    mbedtls_mpi T;
    mbedtls_mpi_init(&T);

    p = (unsigned char *) key;
    end = p + keylen;

    /*
     * This function parses the RSAPrivateKey (PKCS#1)
     *
     *  RSAPrivateKey ::= SEQUENCE {
     *      version           Version,
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER,  -- e
     *      privateExponent   INTEGER,  -- d
     *      prime1            INTEGER,  -- p
     *      prime2            INTEGER,  -- q
     *      exponent1         INTEGER,  -- d mod (p-1)
     *      exponent2         INTEGER,  -- d mod (q-1)
     *      coefficient       INTEGER,  -- (inverse of q) mod p
     *      otherPrimeInfos   OtherPrimeInfos OPTIONAL
     *  }
     */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (end != p + len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if ((ret = mbedtls_asn1_get_int(&p, end, &version, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (version != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /* Import N */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, &T, NULL, NULL,
                                  NULL, NULL, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import E */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, NULL,
                                  NULL, &T, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import D */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, NULL,
                                  &T, NULL, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import P */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, &T, NULL,
                                  NULL, NULL, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import Q */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, &T,
                                  NULL, NULL, diagnostics)) != 0) {
        goto cleanup;
    }

#if !defined(MBEDTLS_RSA_NO_CRT) && !defined(MBEDTLS_RSA_ALT)
    /*
     * The RSA CRT parameters DP, DQ and QP are nominally redundant, in
     * that they can be easily recomputed from D, P and Q. However by
     * parsing them from the PKCS1 structure it is possible to avoid
     * recalculating them which both reduces the overhead of loading
     * RSA private keys into memory and also avoids side channels which
     * can arise when computing those values, since all of D, P, and Q
     * are secret. See https://eprint.iacr.org/2020/055 for a
     * description of one such attack.
     */

    /* Import DP */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->DP, &T, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import DQ */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->DQ, &T, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Import QP */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->QP, &T, diagnostics)) != 0) {
        goto cleanup;
    }

#else
    /* Verify existence of the CRT params */
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0 ||
        (ret = asn1_get_nonzero_mpi(&p, end, &T, diagnostics)) != 0) {
        goto cleanup;
    }
#endif

    /* rsa_complete() doesn't complete anything with the default
     * implementation but is still called:
     * - for the benefit of alternative implementation that may want to
     *   pre-compute stuff beyond what's provided (eg Montgomery factors)
     * - as is also sanity-checks the key
     *
     * Furthermore, we also check the public part for consistency with
     * mbedtls_pk_parse_pubkey(), as it includes size minima for example.
     */
    if ((ret = mbedtls_rsa_complete(rsa, diagnostics)) != 0 ||
        (ret = mbedtls_rsa_check_pubkey(rsa, diagnostics)) != 0) {
        goto cleanup;
    }

    if (p != end) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_ASN1_LENGTH_MISMATCH);
    }

cleanup:

    mbedtls_mpi_free(&T);

    if (ret != 0) {
        mbedtls_rsa_free(rsa);
    }

    MINTLS_RETURN(ret);
}

int mbedtls_rsa_parse_pubkey(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char *p = (unsigned char *) key;
    unsigned char *end = (unsigned char *) (key + keylen);
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    /*
     *  RSAPublicKey ::= SEQUENCE {
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER   -- e
     *  }
     */

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if (end != p + len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /* Import N */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_rsa_import_raw(rsa, p, len, NULL, 0, NULL, 0,
                                      NULL, 0, NULL, 0, diagnostics)) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    p += len;

    /* Import E */
    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    if ((ret = mbedtls_rsa_import_raw(rsa, NULL, 0, NULL, 0, NULL, 0,
                                      NULL, 0, p, len, diagnostics)) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    p += len;

    if (mbedtls_rsa_complete(rsa, diagnostics) != 0 ||
        mbedtls_rsa_check_pubkey(rsa, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (p != end) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_ASN1_LENGTH_MISMATCH);
    }

    MINTLS_RETURN(0);
}

int mbedtls_rsa_write_key(const mbedtls_rsa_context *rsa, unsigned char *start,
                          unsigned char **p, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t len = 0;
    int ret;

    mbedtls_mpi T; /* Temporary holding the exported parameters */

    /*
     * Export the parameters one after another to avoid simultaneous copies.
     */

    mbedtls_mpi_init(&T);

    /* Export QP */
    if ((ret = mbedtls_rsa_export_crt(rsa, NULL, NULL, &T, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export DQ */
    if ((ret = mbedtls_rsa_export_crt(rsa, NULL, &T, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export DP */
    if ((ret = mbedtls_rsa_export_crt(rsa, &T, NULL, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export Q */
    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, &T, NULL, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export P */
    if ((ret = mbedtls_rsa_export(rsa, NULL, &T, NULL, NULL, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export D */
    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, &T, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export E */
    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, NULL, &T, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export N */
    if ((ret = mbedtls_rsa_export(rsa, &T, NULL, NULL, NULL, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

end_of_export:

    mbedtls_mpi_free(&T);
    if (ret < 0) {
        MINTLS_RETURN(ret);
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, 0, diagnostics));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len, diagnostics));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                                                     MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE, diagnostics));

    MINTLS_RETURN((int) len);
}

/*
 *  RSAPublicKey ::= SEQUENCE {
 *      modulus           INTEGER,  -- n
 *      publicExponent    INTEGER   -- e
 *  }
 */
int mbedtls_rsa_write_pubkey(const mbedtls_rsa_context *rsa, unsigned char *start,
                             unsigned char **p, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len = 0;
    mbedtls_mpi T;

    mbedtls_mpi_init(&T);

    /* Export E */
    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, NULL, &T, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export N */
    if ((ret = mbedtls_rsa_export(rsa, &T, NULL, NULL, NULL, NULL, diagnostics)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T, diagnostics)) < 0) {
        goto end_of_export;
    }
    len += ret;

end_of_export:

    mbedtls_mpi_free(&T);
    if (ret < 0) {
        MINTLS_RETURN(ret);
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len, diagnostics));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE, diagnostics));

    MINTLS_RETURN((int) len);
}

/*
 * EME-PKCS1-v1_5 decoding, see documentation in rsa_internal.h
 */
MBEDTLS_STATIC_TESTABLE int mbedtls_ct_rsaes_pkcs1_v15_unpadding(
    unsigned char *input, size_t ilen,
    unsigned char *output, size_t output_max_len, size_t *olen)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t i, plaintext_max_size;

    /* The following variables take sensitive values: their value must
     * not leak into the observable behavior of the function other than
     * the designated outputs (output, olen, return value). Otherwise
     * this would open the execution of the function to
     * side-channel-based variants of the Bleichenbacher padding oracle
     * attack. Potential side channels include overall timing, memory
     * access patterns (especially visible to an adversary who has access
     * to a shared memory cache), and branches (especially visible to
     * an adversary who has access to a shared code cache or to a shared
     * branch predictor). */
    size_t pad_count = 0;
    mbedtls_ct_condition_t bad;
    mbedtls_ct_condition_t pad_done;
    size_t plaintext_size = 0;
    mbedtls_ct_condition_t output_too_large;

    plaintext_max_size = (output_max_len > ilen - 11) ? ilen - 11
                                                        : output_max_len;

    /* Check and get padding length in constant time and constant
     * memory trace. The first byte must be 0. */
    bad = mbedtls_ct_bool(input[0]);

    /* Decode EME-PKCS1-v1_5 padding: 0x00 || 0x02 || PS || 0x00
     * where PS must be at least 8 nonzero bytes. */
    bad = mbedtls_ct_bool_or(bad, mbedtls_ct_uint_ne(input[1], MBEDTLS_RSA_CRYPT));

    /* Read the whole buffer. Set pad_done to nonzero if we find
     * the 0x00 byte and remember the padding length in pad_count. */
    pad_done = MBEDTLS_CT_FALSE;
    for (i = 2; i < ilen; i++) {
        mbedtls_ct_condition_t found = mbedtls_ct_uint_eq(input[i], 0);
        pad_done   = mbedtls_ct_bool_or(pad_done, found);
        pad_count += mbedtls_ct_uint_if_else_0(mbedtls_ct_bool_not(pad_done), 1);
    }

    /* If pad_done is still zero, there's no data, only unfinished padding. */
    bad = mbedtls_ct_bool_or(bad, mbedtls_ct_bool_not(pad_done));

    /* There must be at least 8 bytes of padding. */
    bad = mbedtls_ct_bool_or(bad, mbedtls_ct_uint_gt(8, pad_count));

    /* If the padding is valid, set plaintext_size to the number of
     * remaining bytes after stripping the padding. If the padding
     * is invalid, avoid leaking this fact through the size of the
     * output: use the maximum message size that fits in the output
     * buffer. Do it without branches to avoid leaking the padding
     * validity through timing. RSA keys are small enough that all the
     * size_t values involved fit in unsigned int. */
    plaintext_size = mbedtls_ct_uint_if(
        bad, (unsigned) plaintext_max_size,
        (unsigned) (ilen - pad_count - 3));

    /* Set output_too_large to 0 if the plaintext fits in the output
     * buffer and to 1 otherwise. */
    output_too_large = mbedtls_ct_uint_gt(plaintext_size,
                                          plaintext_max_size);

    /* Set ret without branches to avoid timing attacks. Return:
     * - INVALID_PADDING if the padding is bad (bad != 0).
     * - OUTPUT_TOO_LARGE if the padding is good but the decrypted
     *   plaintext does not fit in the output buffer.
     * - 0 if the padding is correct. */
    ret = mbedtls_ct_error_if(
        bad,
        MBEDTLS_ERR_RSA_INVALID_PADDING,
        mbedtls_ct_error_if_else_0(output_too_large, MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE)
        );

    /* If the padding is bad or the plaintext is too large, zero the
     * data that we're about to copy to the output buffer.
     * We need to copy the same amount of data
     * from the same buffer whether the padding is good or not to
     * avoid leaking the padding validity through overall timing or
     * through memory or cache access patterns. */
    mbedtls_ct_zeroize_if(mbedtls_ct_bool_or(bad, output_too_large), input + 11, ilen - 11);

    /* If the plaintext is too large, truncate it to the buffer size.
     * Copy anyway to avoid revealing the length through timing, because
     * revealing the length is as bad as revealing the padding validity
     * for a Bleichenbacher attack. */
    plaintext_size = mbedtls_ct_uint_if(output_too_large,
                                        (unsigned) plaintext_max_size,
                                        (unsigned) plaintext_size);

    /* Move the plaintext to the leftmost position where it can start in
     * the working buffer, i.e. make it start plaintext_max_size from
     * the end of the buffer. Do this with a memory access trace that
     * does not depend on the plaintext size. After this move, the
     * starting location of the plaintext is no longer sensitive
     * information. */
    mbedtls_ct_memmove_left(input + ilen - plaintext_max_size,
                            plaintext_max_size,
                            plaintext_max_size - plaintext_size);

    /* Finally copy the decrypted plaintext plus trailing zeros into the output
     * buffer. If output_max_len is 0, then output may be an invalid pointer
     * and the result of memcpy() would be undefined; prevent undefined
     * behavior making sure to depend only on output_max_len (the size of the
     * user-provided output buffer), which is independent from plaintext
     * length, validity of padding, success of the decryption, and other
     * secrets. */
    if (output_max_len != 0) {
        memcpy(output, input + ilen - plaintext_max_size, plaintext_max_size);
    }

    /* Report the amount of data we copied to the output buffer. In case
     * of errors (bad padding or output too large), the value of *olen
     * when this function returns is not specified. Making it equivalent
     * to the good case limits the risks of leaking the padding validity. */
    *olen = plaintext_size;

    return ret;
}

int mbedtls_rsa_decrypt_decompose_ret(
    int invalid_padding_in, int invalid_padding_out,
    int output_too_large_in, int output_too_large_out,
    int combined_ret,
    int *problem)
{
    *problem = 0;
    int ret = combined_ret;

    const mbedtls_ct_condition_t invalid_padding_cond =
        mbedtls_ct_uint_eq((mbedtls_ct_uint_t) ret,
                           (mbedtls_ct_uint_t) invalid_padding_in);
    *problem = mbedtls_ct_error_if(invalid_padding_cond, invalid_padding_out, *problem);
    ret = mbedtls_ct_error_if(invalid_padding_cond, 0, ret);

    const mbedtls_ct_condition_t output_too_large_cond =
        mbedtls_ct_uint_eq((mbedtls_ct_uint_t) ret,
                           (mbedtls_ct_uint_t) output_too_large_in);
    *problem = mbedtls_ct_error_if(output_too_large_cond, output_too_large_out, *problem);
    ret = mbedtls_ct_error_if(output_too_large_cond, 0, ret);

    return ret;
}

int mbedtls_rsa_import(mbedtls_rsa_context *ctx,
                       const mbedtls_mpi *N,
                       const mbedtls_mpi *P, const mbedtls_mpi *Q,
                       const mbedtls_mpi *D, const mbedtls_mpi *E, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if ((N != NULL && (ret = mbedtls_mpi_copy(&ctx->N, N, diagnostics)) != 0) ||
        (P != NULL && (ret = mbedtls_mpi_copy(&ctx->P, P, diagnostics)) != 0) ||
        (Q != NULL && (ret = mbedtls_mpi_copy(&ctx->Q, Q, diagnostics)) != 0) ||
        (D != NULL && (ret = mbedtls_mpi_copy(&ctx->D, D, diagnostics)) != 0) ||
        (E != NULL && (ret = mbedtls_mpi_copy(&ctx->E, E, diagnostics)) != 0)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
    }

    if (N != NULL) {
        ctx->len = mbedtls_mpi_size(&ctx->N);
    }

    MINTLS_RETURN(0);
}

int mbedtls_rsa_import_raw(mbedtls_rsa_context *ctx,
                           unsigned char const *N, size_t N_len,
                           unsigned char const *P, size_t P_len,
                           unsigned char const *Q, size_t Q_len,
                           unsigned char const *D, size_t D_len,
                           unsigned char const *E, size_t E_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;

    if (N != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ctx->N, N, N_len, diagnostics));
        ctx->len = mbedtls_mpi_size(&ctx->N);
    }

    if (P != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ctx->P, P, P_len, diagnostics));
    }

    if (Q != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ctx->Q, Q, Q_len, diagnostics));
    }

    if (D != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ctx->D, D, D_len, diagnostics));
    }

    if (E != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ctx->E, E, E_len, diagnostics));
    }

cleanup:

    if (ret != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
    }

    MINTLS_RETURN(0);
}

/*
 * Checks whether the context fields are set in such a way
 * that the RSA primitives will be able to execute without error.
 * It does *not* make guarantees for consistency of the parameters.
 */
int rsa_check_context(mbedtls_rsa_context const *ctx, int is_priv,
                             int blinding_needed, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if !defined(MBEDTLS_RSA_NO_CRT)
    /* blinding_needed is only used for NO_CRT to decide whether
     * P,Q need to be present or not. */
    ((void) blinding_needed);
#endif

    if (ctx->len != mbedtls_mpi_size(&ctx->N) ||
        ctx->len > MBEDTLS_MPI_MAX_SIZE) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /*
     * 1. Modular exponentiation needs positive, odd moduli.
     */

    /* Modular exponentiation wrt. N is always used for
     * RSA public key operations. */
    if (mbedtls_mpi_cmp_int(&ctx->N, 0) <= 0 ||
        mbedtls_mpi_get_bit(&ctx->N, 0) == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

#if !defined(MBEDTLS_RSA_NO_CRT)
    /* Modular exponentiation for P and Q is only
     * used for private key operations and if CRT
     * is used. */
    if (is_priv &&
        (mbedtls_mpi_cmp_int(&ctx->P, 0) <= 0 ||
         mbedtls_mpi_get_bit(&ctx->P, 0) == 0 ||
         mbedtls_mpi_cmp_int(&ctx->Q, 0) <= 0 ||
         mbedtls_mpi_get_bit(&ctx->Q, 0) == 0)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
#endif /* !MBEDTLS_RSA_NO_CRT */

    /*
     * 2. Exponents must be positive
     */

    /* Always need E for public key operations */
    if (mbedtls_mpi_cmp_int(&ctx->E, 0) <= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

#if defined(MBEDTLS_RSA_NO_CRT)
    /* For private key operations, use D or DP & DQ
     * as (unblinded) exponents. */
    if (is_priv && mbedtls_mpi_cmp_int(&ctx->D, 0) <= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
#else
    if (is_priv &&
        (mbedtls_mpi_cmp_int(&ctx->DP, 0) <= 0 ||
         mbedtls_mpi_cmp_int(&ctx->DQ, 0) <= 0)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
#endif /* MBEDTLS_RSA_NO_CRT */

    /* Blinding shouldn't make exponents negative either,
     * so check that P, Q >= 1 if that hasn't yet been
     * done as part of 1. */
#if defined(MBEDTLS_RSA_NO_CRT)
    if (is_priv && blinding_needed &&
        (mbedtls_mpi_cmp_int(&ctx->P, 0) <= 0 ||
         mbedtls_mpi_cmp_int(&ctx->Q, 0) <= 0)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
#endif

    /* It wouldn't lead to an error if it wasn't satisfied,
     * but check for QP >= 1 nonetheless. */
#if !defined(MBEDTLS_RSA_NO_CRT)
    if (is_priv &&
        mbedtls_mpi_cmp_int(&ctx->QP, 0) <= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
#endif

    MINTLS_RETURN(0);
}

int mbedtls_rsa_complete(mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    int have_N, have_P, have_Q, have_D, have_E;
#if !defined(MBEDTLS_RSA_NO_CRT)
    int have_DP, have_DQ, have_QP;
#endif
    int n_missing, pq_missing, d_missing, is_pub, is_priv;

    have_N = (mbedtls_mpi_cmp_int(&ctx->N, 0) != 0);
    have_P = (mbedtls_mpi_cmp_int(&ctx->P, 0) != 0);
    have_Q = (mbedtls_mpi_cmp_int(&ctx->Q, 0) != 0);
    have_D = (mbedtls_mpi_cmp_int(&ctx->D, 0) != 0);
    have_E = (mbedtls_mpi_cmp_int(&ctx->E, 0) != 0);

#if !defined(MBEDTLS_RSA_NO_CRT)
    have_DP = (mbedtls_mpi_cmp_int(&ctx->DP, 0) != 0);
    have_DQ = (mbedtls_mpi_cmp_int(&ctx->DQ, 0) != 0);
    have_QP = (mbedtls_mpi_cmp_int(&ctx->QP, 0) != 0);
#endif

    /*
     * Check whether provided parameters are enough
     * to deduce all others. The following incomplete
     * parameter sets for private keys are supported:
     *
     * (1) P, Q missing.
     * (2) D and potentially N missing.
     *
     */

    n_missing  =              have_P &&  have_Q &&  have_D && have_E;
    pq_missing =   have_N && !have_P && !have_Q &&  have_D && have_E;
    d_missing  =              have_P &&  have_Q && !have_D && have_E;
    is_pub     =   have_N && !have_P && !have_Q && !have_D && have_E;

    /* These three alternatives are mutually exclusive */
    is_priv = n_missing || pq_missing || d_missing;

    if (!is_priv && !is_pub) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /*
     * Step 1: Deduce N if P, Q are provided.
     */

    if (!have_N && have_P && have_Q) {
        if ((ret = mbedtls_mpi_mul_mpi(&ctx->N, &ctx->P,
                                       &ctx->Q, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
        }

        ctx->len = mbedtls_mpi_size(&ctx->N);
    }

    /*
     * Step 2: Deduce and verify all remaining core parameters.
     */

    if (pq_missing) {
        ret = mbedtls_rsa_deduce_primes(&ctx->N, &ctx->E, &ctx->D,
                                        &ctx->P, &ctx->Q, diagnostics);
        if (ret != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
        }

    } else if (d_missing) {
        if ((ret = mbedtls_rsa_deduce_private_exponent(&ctx->P,
                                                       &ctx->Q,
                                                       &ctx->E,
                                                       &ctx->D, diagnostics)) != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
        }
    }

    /*
     * Step 3: Deduce all additional parameters specific
     *         to our current RSA implementation.
     */

#if !defined(MBEDTLS_RSA_NO_CRT)
    if (is_priv && !(have_DP && have_DQ && have_QP)) {
        ret = mbedtls_rsa_deduce_crt(&ctx->P,  &ctx->Q,  &ctx->D,
                                     &ctx->DP, &ctx->DQ, &ctx->QP, diagnostics);
        if (ret != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
        }
    }
#endif /* MBEDTLS_RSA_NO_CRT */

    /*
     * Step 3: Basic sanity checks
     */

    MINTLS_RETURN(rsa_check_context(ctx, is_priv, 1, diagnostics));
}

int mbedtls_rsa_export_raw(const mbedtls_rsa_context *ctx,
                           unsigned char *N, size_t N_len,
                           unsigned char *P, size_t P_len,
                           unsigned char *Q, size_t Q_len,
                           unsigned char *D, size_t D_len,
                           unsigned char *E, size_t E_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    int is_priv;

    /* Check if key is private or public */
    is_priv =
        mbedtls_mpi_cmp_int(&ctx->N, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->P, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->Q, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->D, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->E, 0) != 0;

    if (!is_priv) {
        /* If we're trying to export private parameters for a public key,
         * something must be wrong. */
        if (P != NULL || Q != NULL || D != NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

    }

    if (N != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&ctx->N, N, N_len, diagnostics));
    }

    if (P != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&ctx->P, P, P_len, diagnostics));
    }

    if (Q != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&ctx->Q, Q, Q_len, diagnostics));
    }

    if (D != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&ctx->D, D, D_len, diagnostics));
    }

    if (E != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&ctx->E, E, E_len, diagnostics));
    }

cleanup:

    MINTLS_RETURN(ret);
}

int mbedtls_rsa_export(const mbedtls_rsa_context *ctx,
                       mbedtls_mpi *N, mbedtls_mpi *P, mbedtls_mpi *Q,
                       mbedtls_mpi *D, mbedtls_mpi *E, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int is_priv;

    /* Check if key is private or public */
    is_priv =
        mbedtls_mpi_cmp_int(&ctx->N, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->P, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->Q, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->D, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->E, 0) != 0;

    if (!is_priv) {
        /* If we're trying to export private parameters for a public key,
         * something must be wrong. */
        if (P != NULL || Q != NULL || D != NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

    }

    /* Export all requested core parameters. */

    if ((N != NULL && (ret = mbedtls_mpi_copy(N, &ctx->N, diagnostics)) != 0) ||
        (P != NULL && (ret = mbedtls_mpi_copy(P, &ctx->P, diagnostics)) != 0) ||
        (Q != NULL && (ret = mbedtls_mpi_copy(Q, &ctx->Q, diagnostics)) != 0) ||
        (D != NULL && (ret = mbedtls_mpi_copy(D, &ctx->D, diagnostics)) != 0) ||
        (E != NULL && (ret = mbedtls_mpi_copy(E, &ctx->E, diagnostics)) != 0)) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}

/*
 * Export CRT parameters
 * This must also be implemented if CRT is not used, for being able to
 * write DER encoded RSA keys. The helper function mbedtls_rsa_deduce_crt
 * can be used in this case.
 */
int mbedtls_rsa_export_crt(const mbedtls_rsa_context *ctx,
                           mbedtls_mpi *DP, mbedtls_mpi *DQ, mbedtls_mpi *QP, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int is_priv;

    /* Check if key is private or public */
    is_priv =
        mbedtls_mpi_cmp_int(&ctx->N, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->P, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->Q, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->D, 0) != 0 &&
        mbedtls_mpi_cmp_int(&ctx->E, 0) != 0;

    if (!is_priv) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

#if !defined(MBEDTLS_RSA_NO_CRT)
    /* Export all requested blinding parameters. */
    if ((DP != NULL && (ret = mbedtls_mpi_copy(DP, &ctx->DP, diagnostics)) != 0) ||
        (DQ != NULL && (ret = mbedtls_mpi_copy(DQ, &ctx->DQ, diagnostics)) != 0) ||
        (QP != NULL && (ret = mbedtls_mpi_copy(QP, &ctx->QP, diagnostics)) != 0)) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
    }
#else
    if ((ret = mbedtls_rsa_deduce_crt(&ctx->P, &ctx->Q, &ctx->D,
                                      DP, DQ, QP, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_BAD_INPUT_DATA, ret));
    }
#endif

    MINTLS_RETURN(0);
}

/*
 * Initialize an RSA context
 */
void mbedtls_rsa_init(mbedtls_rsa_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_rsa_context));

    ctx->padding = MBEDTLS_RSA_PKCS_V15;
    ctx->hash_id = MBEDTLS_MD_NONE;

}

/*
 * Set padding for an existing RSA context
 */
int mbedtls_rsa_set_padding(mbedtls_rsa_context *ctx, int padding,
                            mbedtls_md_type_t hash_id, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    switch (padding) {
        case MBEDTLS_RSA_PKCS_V15:
            break;

        case MBEDTLS_RSA_PKCS_V21:
            break;
        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }

    if ((padding == MBEDTLS_RSA_PKCS_V21) &&
        (hash_id != MBEDTLS_MD_NONE)) {
        /* Just make sure this hash is supported in this build. */
        if (mbedtls_md_info_from_type(hash_id) == NULL) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
        }
    }

    ctx->padding = padding;
    ctx->hash_id = hash_id;

    MINTLS_RETURN(0);
}

/*
 * Get padding mode of initialized RSA context
 */
int mbedtls_rsa_get_padding_mode(const mbedtls_rsa_context *ctx)
{
    return ctx->padding;
}

/*
 * Get hash identifier of mbedtls_md_type_t type
 */
int mbedtls_rsa_get_md_alg(const mbedtls_rsa_context *ctx)
{
    return ctx->hash_id;
}

/*
 * Get length in bits of RSA modulus
 */
size_t mbedtls_rsa_get_bitlen(const mbedtls_rsa_context *ctx)
{
    return mbedtls_mpi_bitlen(&ctx->N);
}

/*
 * Get length in bytes of RSA modulus
 */
size_t mbedtls_rsa_get_len(const mbedtls_rsa_context *ctx)
{
    return ctx->len;
}

#if defined(MBEDTLS_GENPRIME)

/*
 * Generate an RSA keypair
 *
 * This generation method follows the RSA key pair generation procedure of
 * FIPS 186-4 if 2^16 < exponent < 2^256 and nbits = 2048 or nbits = 3072.
 */
int mbedtls_rsa_gen_key(mbedtls_rsa_context *ctx,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng,
                        unsigned int nbits, int exponent, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi H;
    int prime_quality = 0;

    /*
     * If the modulus is 1024 bit long or shorter, then the security strength of
     * the RSA algorithm is less than or equal to 80 bits and therefore an error
     * rate of 2^-80 is sufficient.
     */
    if (nbits > 1024) {
        prime_quality = MBEDTLS_MPI_GEN_PRIME_FLAG_LOW_ERR;
    }

    mbedtls_mpi_init(&H);

    if (exponent < 3 || nbits % 2 != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        goto cleanup;
    }

    if (nbits < MBEDTLS_RSA_GEN_KEY_MIN_BITS) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        goto cleanup;
    }

    /*
     * find primes P and Q with Q < P so that:
     * 1.  |P-Q| > 2^( nbits / 2 - 100 )
     * 2.  GCD( E, (P-1)*(Q-1) ) == 1
     * 3.  E^-1 mod LCM(P-1, Q-1) > 2^( nbits / 2 )
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&ctx->E, exponent, diagnostics));

    do {
        MBEDTLS_MPI_CHK(mbedtls_mpi_gen_prime(&ctx->P, nbits >> 1,
                                              prime_quality, f_rng, p_rng, diagnostics));

        MBEDTLS_MPI_CHK(mbedtls_mpi_gen_prime(&ctx->Q, nbits >> 1,
                                              prime_quality, f_rng, p_rng, diagnostics));

        /* make sure the difference between p and q is not too small (FIPS 186-4 §B.3.3 step 5.4) */
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&H, &ctx->P, &ctx->Q, diagnostics));
        if (mbedtls_mpi_bitlen(&H) <= ((nbits >= 200) ? ((nbits >> 1) - 99) : 0)) {
            continue;
        }

        /* not required by any standards, but some users rely on the fact that P > Q */
        if (H.s < 0) {
            mbedtls_mpi_swap(&ctx->P, &ctx->Q);
        }

        /* Compute D = E^-1 mod LCM(P-1, Q-1) (FIPS 186-4 §B.3.1 criterion 3(b))
         * if it exists (FIPS 186-4 §B.3.1 criterion 2(a)) */
        ret = mbedtls_rsa_deduce_private_exponent(&ctx->P, &ctx->Q, &ctx->E, &ctx->D, diagnostics);
        if (ret == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE) {
            continue;
        }
        if (ret != 0) {
            goto cleanup;
        }

        /* (FIPS 186-4 §B.3.1 criterion 3(a)) */
        if (mbedtls_mpi_bitlen(&ctx->D) <= ((nbits + 1) / 2)) {
            continue;
        }

        break;
    } while (1);

    /* N = P * Q */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&ctx->N, &ctx->P, &ctx->Q, diagnostics));
    ctx->len = mbedtls_mpi_size(&ctx->N);

#if !defined(MBEDTLS_RSA_NO_CRT)
    /*
     * DP = D mod (P - 1)
     * DQ = D mod (Q - 1)
     * QP = Q^-1 mod P
     */
    MBEDTLS_MPI_CHK(mbedtls_rsa_deduce_crt(&ctx->P, &ctx->Q, &ctx->D,
                                           &ctx->DP, &ctx->DQ, &ctx->QP, diagnostics));
#endif /* MBEDTLS_RSA_NO_CRT */

    /* Double-check */
    MBEDTLS_MPI_CHK(mbedtls_rsa_check_privkey(ctx, diagnostics));

cleanup:

    mbedtls_mpi_free(&H);

    if (ret != 0) {
        mbedtls_rsa_free(ctx);

        if ((-ret & ~0x7f) == 0) {
            ret = MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_KEY_GEN_FAILED, ret);
        }
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(0);
}

#endif /* MBEDTLS_GENPRIME */

/*
 * Check a public RSA key
 */
int mbedtls_rsa_check_pubkey(const mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (rsa_check_context(ctx, 0 /* public */, 0 /* no blinding */, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    if (mbedtls_mpi_bitlen(&ctx->N) < 128) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    if (mbedtls_mpi_get_bit(&ctx->E, 0) == 0 ||
        mbedtls_mpi_bitlen(&ctx->E)     < 2  ||
        mbedtls_mpi_cmp_mpi(&ctx->E, &ctx->N) >= 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    MINTLS_RETURN(0);
}

/*
 * Check for the consistency of all fields in an RSA private key context
 */
int mbedtls_rsa_check_privkey(const mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (mbedtls_rsa_check_pubkey(ctx, diagnostics) != 0 ||
        rsa_check_context(ctx, 1 /* private */, 1 /* blinding */, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    if (mbedtls_rsa_validate_params(&ctx->N, &ctx->P, &ctx->Q,
                                    &ctx->D, &ctx->E, NULL, NULL, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

#if !defined(MBEDTLS_RSA_NO_CRT)
    else if (mbedtls_rsa_validate_crt(&ctx->P, &ctx->Q, &ctx->D,
                                      &ctx->DP, &ctx->DQ, &ctx->QP, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }
#endif

    MINTLS_RETURN(0);
}

/*
 * Check if contexts holding a public and private key match
 */
int mbedtls_rsa_check_pub_priv(const mbedtls_rsa_context *pub,
                               const mbedtls_rsa_context *prv, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (mbedtls_rsa_check_pubkey(pub, diagnostics)  != 0 ||
        mbedtls_rsa_check_privkey(prv, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    if (mbedtls_mpi_cmp_mpi(&pub->N, &prv->N) != 0 ||
        mbedtls_mpi_cmp_mpi(&pub->E, &prv->E) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED);
    }

    MINTLS_RETURN(0);
}

/*
 * Do an RSA public key operation
 */
int mbedtls_rsa_public(mbedtls_rsa_context *ctx,
                       const unsigned char *input,
                       unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t olen;
    mbedtls_mpi T;

    if (rsa_check_context(ctx, 0 /* public */, 0 /* no blinding */, diagnostics)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    mbedtls_mpi_init(&T);

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&T, input, ctx->len, diagnostics));

    if (mbedtls_mpi_cmp_mpi(&T, &ctx->N) >= 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_MPI_BAD_INPUT_DATA);
        goto cleanup;
    }

    olen = ctx->len;
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod_unsafe(&T, &T, &ctx->E, &ctx->N, &ctx->RN, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&T, output, olen, diagnostics));

cleanup:

    mbedtls_mpi_free(&T);

    if (ret != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_PUBLIC_FAILED, ret));
    }

    MINTLS_RETURN(0);
}

#if !defined(MBEDTLS_RSA_NO_CRT)
/*
 * Compute T such that T = TP mod P and T = TQ mod Q.
 * (This is the Chinese Remainder Theorem - CRT.)
 */
int rsa_apply_crt(mbedtls_mpi *T,
                         const mbedtls_mpi *TP,
                         const mbedtls_mpi *TQ,
                         const mbedtls_rsa_context *ctx, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    /*
     * Set T = ((TP - TQ) * (Q^-1 mod P) mod P) * Q + TQ
     *
     * That way we have both:
     * mod P: T = (TP - TQ) * (Q^-1 * Q) + TQ = (TP - TQ) * 1 + TQ = TP
     * mod Q: T = (...) * Q + TQ = TQ
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(T, TP, TQ, diagnostics));        // T = TP - TQ
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(T, T, &ctx->QP, diagnostics));   // T *= Q^-1 mod P
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(T, T, &ctx->P, diagnostics));    // T %= P
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(T, T, &ctx->Q, diagnostics));    // T *= Q
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(T, T, TQ, diagnostics));         // T += TQ

cleanup:
    MINTLS_RETURN(ret);
}
#endif

/* Generate random A and B such that A^-1 = B mod N */
int rsa_gen_rand_with_inverse(const mbedtls_rsa_context *ctx,
                                     mbedtls_mpi *A,
                                     mbedtls_mpi *B,
                                     int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                     void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

#if defined(MBEDTLS_RSA_NO_CRT)
    int ret;
    mbedtls_mpi G;

    mbedtls_mpi_init(&G);

    MBEDTLS_MPI_CHK(mbedtls_mpi_random(A, 1, &ctx->N, f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(&G, B, A, &ctx->N, diagnostics));

    if (mbedtls_mpi_cmp_int(&G, 1) != 0) {
        /* This happens if we're unlucky enough to draw a multiple of P or Q,
         * or if (at least) one of them is not a prime, and we drew a multiple
         * of one of its factors. */
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_RNG_FAILED);
        goto cleanup;
    }

cleanup:
    mbedtls_mpi_free(&G);

    MINTLS_RETURN(ret);
#else
    int ret;
    mbedtls_mpi Ap, Aq, Bp, Bq, G;

    mbedtls_mpi_init(&Ap); mbedtls_mpi_init(&Aq);
    mbedtls_mpi_init(&Bp); mbedtls_mpi_init(&Bq);
    mbedtls_mpi_init(&G);

    /*
     * Instead of generating A, B = A^-1 (mod N) directly, generate one Ap, Bp
     * pair (mod P) and one pair (mod Q) and use Chinese Remainder Theorem to
     * construct an A and B from those.
     *
     * This works because the CRT correspondence is a ring isomorphism between
     * Z/NZ (integers mod N) and Z/PZ x Z/QZ (pairs of integers mod P and Q):
     * - it is a bijection (one-to-one correspondence);
     * - doing a ring operation (modular +, -, *, ^-1 when possible) on one side is
     *   the same as doing it on the other side.
     * So, drawing uniformly at random an invertible A mod N is the same as
     * drawing uniformly at random pairs of invertible Ap mod P, Aq mod Q.
     */

    /* Generate Ap in [1, P) and compute Bp = Ap^-1 mod P */
    MBEDTLS_MPI_CHK(mbedtls_mpi_random(&Ap, 1, &ctx->P, f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(&G, &Bp, &Ap, &ctx->P, diagnostics));
    if (mbedtls_mpi_cmp_int(&G, 1) != 0) {
        /* This can only happen if P was not a prime. */
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_RNG_FAILED);
        goto cleanup;
    }

    /* Generate Aq in [1, Q) and compute Bq = Aq^-1 mod Q */
    MBEDTLS_MPI_CHK(mbedtls_mpi_random(&Aq, 1, &ctx->Q, f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(&G, &Bq, &Aq, &ctx->Q, diagnostics));
    if (mbedtls_mpi_cmp_int(&G, 1) != 0) {
        /* This can only happen if Q was not a prime. */
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_RNG_FAILED);
        goto cleanup;
    }

    /* Reconstruct A and B */
    MBEDTLS_MPI_CHK(rsa_apply_crt(A, &Ap, &Aq, ctx, diagnostics));
    MBEDTLS_MPI_CHK(rsa_apply_crt(B, &Bp, &Bq, ctx, diagnostics));

cleanup:
    mbedtls_mpi_free(&Ap); mbedtls_mpi_free(&Aq);
    mbedtls_mpi_free(&Bp); mbedtls_mpi_free(&Bq);
    mbedtls_mpi_free(&G);

    MINTLS_RETURN(ret);
#endif
}

/*
 * Generate or update blinding values, see section 10 of:
 *  KOCHER, Paul C. Timing attacks on implementations of Diffie-Hellman, RSA,
 *  DSS, and other systems. In : Advances in Cryptology-CRYPTO'96. Springer
 *  Berlin Heidelberg, 1996. p. 104-113.
 */
int rsa_prepare_blinding(mbedtls_rsa_context *ctx,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics), void *p_rng, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret;

    if (ctx->Vf.p != NULL) {
        /* We already have blinding values, just update them by squaring */
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&ctx->Vi, &ctx->Vi, &ctx->Vi, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&ctx->Vi, &ctx->Vi, &ctx->N, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&ctx->Vf, &ctx->Vf, &ctx->Vf, diagnostics));
        MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&ctx->Vf, &ctx->Vf, &ctx->N, diagnostics));
        goto cleanup;
    }

    /* Unblinding value: Vf = random number, invertible mod N */
    MBEDTLS_MPI_CHK(rsa_gen_rand_with_inverse(ctx, &ctx->Vf, &ctx->Vi, f_rng, p_rng, diagnostics));

    /* Blinding value: Vi = Vf^(-e) mod N
     * (Vi already contains Vf^-1 at this point) */
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&ctx->Vi, &ctx->Vi, &ctx->E, &ctx->N, &ctx->RN, diagnostics));

cleanup:
    MINTLS_RETURN(ret);
}

/*
 * Unblind
 * T = T * Vf mod N
 */
int rsa_unblind(mbedtls_mpi *T, mbedtls_mpi *Vf, const mbedtls_mpi *N, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_mpi_uint mm = mbedtls_mpi_core_montmul_init(N->p);
    const size_t nlimbs = N->n;
    const size_t tlimbs = mbedtls_mpi_core_montmul_working_limbs(nlimbs);
    mbedtls_mpi RR, M_T;

    mbedtls_mpi_init(&RR);
    mbedtls_mpi_init(&M_T);

    MBEDTLS_MPI_CHK(mbedtls_mpi_core_get_mont_r2_unsafe(&RR, N, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&M_T, tlimbs, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_grow(T, nlimbs, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_grow(Vf, nlimbs, diagnostics));

    /* T = T * Vf mod N
     * Reminder: montmul(A, B, N) = A * B * R^-1 mod N
     * Usually both operands are multiplied by R mod N beforehand (by calling
     * `to_mont_rep()` on them), yielding a result that's also * R mod N (aka
     * "in the Montgomery domain"). Here we only multiply one operand by R mod
     * N, so the result is directly what we want - no need to call
     * `from_mont_rep()` on it. */
    mbedtls_mpi_core_to_mont_rep(T->p, T->p, N->p, nlimbs, mm, RR.p, M_T.p);
    mbedtls_mpi_core_montmul(T->p, T->p, Vf->p, nlimbs, N->p, nlimbs, mm, M_T.p);

cleanup:

    mbedtls_mpi_free(&RR);
    mbedtls_mpi_free(&M_T);

    MINTLS_RETURN(ret);
}

/*
 * Exponent blinding supposed to prevent side-channel attacks using multiple
 * traces of measurements to recover the RSA key. The more collisions are there,
 * the more bits of the key can be recovered. See [3].
 *
 * Collecting n collisions with m bit long blinding value requires 2^(m-m/n)
 * observations on average.
 *
 * For example with 28 byte blinding to achieve 2 collisions the adversary has
 * to make 2^112 observations on average.
 *
 * (With the currently (as of 2017 April) known best algorithms breaking 2048
 * bit RSA requires approximately as much time as trying out 2^112 random keys.
 * Thus in this sense with 28 byte blinding the security is not reduced by
 * side-channel attacks like the one in [3])
 *
 * This countermeasure does not help if the key recovery is possible with a
 * single trace.
 */
#define RSA_EXPONENT_BLINDING 28

/*
 * Do an RSA private key operation
 */
int mbedtls_rsa_private(mbedtls_rsa_context *ctx,
                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                        void *p_rng,
                        const unsigned char *input,
                        unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t olen;

    /* Temporary holding the result */
    mbedtls_mpi T;

    /* Temporaries holding P-1, Q-1 and the
     * exponent blinding factor, respectively. */
    mbedtls_mpi P1, Q1, R;

#if !defined(MBEDTLS_RSA_NO_CRT)
    /* Temporaries holding the results mod p resp. mod q. */
    mbedtls_mpi TP, TQ;

    /* Temporaries holding the blinded exponents for
     * the mod p resp. mod q computation (if used). */
    mbedtls_mpi DP_blind, DQ_blind;
#else
    /* Temporary holding the blinded exponent (if used). */
    mbedtls_mpi D_blind;
#endif /* MBEDTLS_RSA_NO_CRT */

    /* Temporaries holding the initial input and the double
     * checked result; should be the same in the end. */
    mbedtls_mpi input_blinded, check_result_blinded;

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (rsa_check_context(ctx, 1 /* private key checks */,
                          1 /* blinding on        */, diagnostics) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /* MPI Initialization */
    mbedtls_mpi_init(&T);

    mbedtls_mpi_init(&P1);
    mbedtls_mpi_init(&Q1);
    mbedtls_mpi_init(&R);

#if defined(MBEDTLS_RSA_NO_CRT)
    mbedtls_mpi_init(&D_blind);
#else
    mbedtls_mpi_init(&DP_blind);
    mbedtls_mpi_init(&DQ_blind);
#endif

#if !defined(MBEDTLS_RSA_NO_CRT)
    mbedtls_mpi_init(&TP); mbedtls_mpi_init(&TQ);
#endif

    mbedtls_mpi_init(&input_blinded);
    mbedtls_mpi_init(&check_result_blinded);

    /* End of MPI initialization */

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&T, input, ctx->len, diagnostics));
    if (mbedtls_mpi_cmp_mpi(&T, &ctx->N) >= 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_MPI_BAD_INPUT_DATA);
        goto cleanup;
    }

    /*
     * Blinding
     * T = T * Vi mod N
     */
    MBEDTLS_MPI_CHK(rsa_prepare_blinding(ctx, f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T, &T, &ctx->Vi, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&T, &T, &ctx->N, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&input_blinded, &T, diagnostics));

    /*
     * Exponent blinding
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&P1, &ctx->P, 1, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&Q1, &ctx->Q, 1, diagnostics));

#if defined(MBEDTLS_RSA_NO_CRT)
    /*
     * D_blind = ( P - 1 ) * ( Q - 1 ) * R + D
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_fill_random(&R, RSA_EXPONENT_BLINDING,
                                            f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&D_blind, &P1, &Q1, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&D_blind, &D_blind, &R, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&D_blind, &D_blind, &ctx->D, diagnostics));
#else
    /*
     * DP_blind = ( P - 1 ) * R + DP
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_fill_random(&R, RSA_EXPONENT_BLINDING,
                                            f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&DP_blind, &P1, &R, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&DP_blind, &DP_blind,
                                        &ctx->DP, diagnostics));

    /*
     * DQ_blind = ( Q - 1 ) * R + DQ
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_fill_random(&R, RSA_EXPONENT_BLINDING,
                                            f_rng, p_rng, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&DQ_blind, &Q1, &R, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&DQ_blind, &DQ_blind,
                                        &ctx->DQ, diagnostics));
#endif /* MBEDTLS_RSA_NO_CRT */

#if defined(MBEDTLS_RSA_NO_CRT)
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&T, &T, &D_blind, &ctx->N, &ctx->RN, diagnostics));
#else
    /*
     * Faster decryption using the CRT
     *
     * TP = input ^ dP mod P
     * TQ = input ^ dQ mod Q
     */

    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&TP, &T, &DP_blind, &ctx->P, &ctx->RP, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&TQ, &T, &DQ_blind, &ctx->Q, &ctx->RQ, diagnostics));
    MBEDTLS_MPI_CHK(rsa_apply_crt(&T, &TP, &TQ, ctx, diagnostics));
#endif /* MBEDTLS_RSA_NO_CRT */

    /* Verify the result to prevent glitching attacks. */
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&check_result_blinded, &T, &ctx->E,
                                        &ctx->N, &ctx->RN, diagnostics));
    if (mbedtls_mpi_cmp_mpi(&check_result_blinded, &input_blinded) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_VERIFY_FAILED);
        goto cleanup;
    }

    /*
     * Unblind
     * T = T * Vf mod N
     */
    MBEDTLS_MPI_CHK(rsa_unblind(&T, &ctx->Vf, &ctx->N, diagnostics));

    olen = ctx->len;
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&T, output, olen, diagnostics));

cleanup:

    mbedtls_mpi_free(&P1);
    mbedtls_mpi_free(&Q1);
    mbedtls_mpi_free(&R);

#if defined(MBEDTLS_RSA_NO_CRT)
    mbedtls_mpi_free(&D_blind);
#else
    mbedtls_mpi_free(&DP_blind);
    mbedtls_mpi_free(&DQ_blind);
#endif

    mbedtls_mpi_free(&T);

#if !defined(MBEDTLS_RSA_NO_CRT)
    mbedtls_mpi_free(&TP); mbedtls_mpi_free(&TQ);
#endif

    mbedtls_mpi_free(&check_result_blinded);
    mbedtls_mpi_free(&input_blinded);

    if (ret != 0 && ret >= -0x007f) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_PRIVATE_FAILED, ret));
    }

    MINTLS_RETURN(ret);
}

/**
 * Generate and apply the MGF1 operation (from PKCS#1 v2.1) to a buffer.
 *
 * \param dst       buffer to mask
 * \param dlen      length of destination buffer
 * \param src       source of the mask generation
 * \param slen      length of the source buffer
 * \param md_alg    message digest to use
 */
int mgf_mask(unsigned char *dst, size_t dlen, unsigned char *src,
                    size_t slen, mbedtls_md_type_t md_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    unsigned char counter[4];
    unsigned char *p;
    unsigned int hlen;
    size_t i, use_len;
    unsigned char mask[MBEDTLS_MD_MAX_SIZE];
    int ret = 0;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    mbedtls_md_init(&md_ctx);
    md_info = mbedtls_md_info_from_type(md_alg);
    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    mbedtls_md_init(&md_ctx);
    if ((ret = mbedtls_md_setup(&md_ctx, md_info, 0, diagnostics)) != 0) {
        goto exit;
    }

    hlen = mbedtls_md_get_size(md_info);

    memset(mask, 0, sizeof(mask));
    memset(counter, 0, 4);

    /* Generate and apply dbMask */
    p = dst;

    while (dlen > 0) {
        use_len = hlen;
        if (dlen < hlen) {
            use_len = dlen;
        }

        if ((ret = mbedtls_md_starts(&md_ctx, diagnostics)) != 0) {
            goto exit;
        }
        if ((ret = mbedtls_md_update(&md_ctx, src, slen, diagnostics)) != 0) {
            goto exit;
        }
        if ((ret = mbedtls_md_update(&md_ctx, counter, 4, diagnostics)) != 0) {
            goto exit;
        }
        if ((ret = mbedtls_md_finish(&md_ctx, mask, diagnostics)) != 0) {
            goto exit;
        }

        for (i = 0; i < use_len; ++i) {
            *p++ ^= mask[i];
        }

        counter[3]++;

        dlen -= use_len;
    }

exit:
    mbedtls_platform_zeroize(mask, sizeof(mask));
    mbedtls_md_free(&md_ctx);

    MINTLS_RETURN(ret);
}

/**
 * Generate Hash(M') as in RFC 8017 page 43 points 5 and 6.
 *
 * \param hash      the input hash
 * \param hlen      length of the input hash
 * \param salt      the input salt
 * \param slen      length of the input salt
 * \param out       the output buffer - must be large enough for \p md_alg
 * \param md_alg    message digest to use
 */
int hash_mprime(const unsigned char *hash, size_t hlen,
                       const unsigned char *salt, size_t slen,
                       unsigned char *out, mbedtls_md_type_t md_alg, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const unsigned char zeros[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    mbedtls_md_context_t md_ctx;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_alg);
    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    mbedtls_md_init(&md_ctx);
    if ((ret = mbedtls_md_setup(&md_ctx, md_info, 0, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_starts(&md_ctx, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md_ctx, zeros, sizeof(zeros), diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md_ctx, hash, hlen, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_update(&md_ctx, salt, slen, diagnostics)) != 0) {
        goto exit;
    }
    if ((ret = mbedtls_md_finish(&md_ctx, out, diagnostics)) != 0) {
        goto exit;
    }

exit:
    mbedtls_md_free(&md_ctx);

    MINTLS_RETURN(ret);
}

/**
 * Compute a hash.
 *
 * \param md_alg    algorithm to use
 * \param input     input message to hash
 * \param ilen      input length
 * \param output    the output buffer - must be large enough for \p md_alg
 */
int compute_hash(mbedtls_md_type_t md_alg,
                        const unsigned char *input, size_t ilen,
                        unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    const mbedtls_md_info_t *md_info;

    md_info = mbedtls_md_info_from_type(md_alg);
    if (md_info == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(mbedtls_md(md_info, input, ilen, output, diagnostics));
}

/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-ENCRYPT function
 */
int mbedtls_rsa_rsaes_oaep_encrypt(mbedtls_rsa_context *ctx,
                                   int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                   void *p_rng,
                                   const unsigned char *label, size_t label_len,
                                   size_t ilen,
                                   const unsigned char *input,
                                   unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t olen;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = output;
    unsigned int hlen;

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    hlen = mbedtls_md_get_size_from_type((mbedtls_md_type_t) ctx->hash_id);
    if (hlen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    olen = ctx->len;

    /* first comparison checks for overflow */
    if (ilen + 2 * hlen + 2 < ilen || olen < ilen + 2 * hlen + 2) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    memset(output, 0, olen);

    *p++ = 0;

    /* Generate a random octet string seed */
    if ((ret = f_rng(p_rng, p, hlen, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_RNG_FAILED, ret));
    }

    p += hlen;

    /* Construct DB */
    ret = compute_hash((mbedtls_md_type_t) ctx->hash_id, label, label_len, p, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }
    p += hlen;
    p += olen - 2 * hlen - 2 - ilen;
    *p++ = 1;
    if (ilen != 0) {
        memcpy(p, input, ilen);
    }

    /* maskedDB: Apply dbMask to DB */
    if ((ret = mgf_mask(output + hlen + 1, olen - hlen - 1, output + 1, hlen,
                        (mbedtls_md_type_t) ctx->hash_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    /* maskedSeed: Apply seedMask to seed */
    if ((ret = mgf_mask(output + 1, hlen, output + hlen + 1, olen - hlen - 1,
                        (mbedtls_md_type_t) ctx->hash_id, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    MINTLS_RETURN(mbedtls_rsa_public(ctx, output, output, diagnostics));
}

/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-ENCRYPT function
 */
int mbedtls_rsa_rsaes_pkcs1_v15_encrypt(mbedtls_rsa_context *ctx,
                                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                        void *p_rng, size_t ilen,
                                        const unsigned char *input,
                                        unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t nb_pad, olen;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = output;

    olen = ctx->len;

    /* first comparison checks for overflow */
    if (ilen + 11 < ilen || olen < ilen + 11) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    nb_pad = olen - 3 - ilen;

    *p++ = 0;

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    *p++ = MBEDTLS_RSA_CRYPT;

    while (nb_pad-- > 0) {
        int rng_dl = 100;

        do {
            ret = f_rng(p_rng, p, 1, diagnostics);
        } while (*p == 0 && --rng_dl && ret == 0);

        /* Check if RNG failed to generate data */
        if (rng_dl == 0 || ret != 0) {
            MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_RNG_FAILED, ret));
        }

        p++;
    }

    *p++ = 0;
    if (ilen != 0) {
        memcpy(p, input, ilen);
    }

    MINTLS_RETURN(mbedtls_rsa_public(ctx, output, output, diagnostics));
}

/*
 * Add the message padding, then do an RSA operation
 */
int mbedtls_rsa_pkcs1_encrypt(mbedtls_rsa_context *ctx,
                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                              void *p_rng,
                              size_t ilen,
                              const unsigned char *input,
                              unsigned char *output, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    switch (ctx->padding) {
        case MBEDTLS_RSA_PKCS_V15:
            MINTLS_RETURN(mbedtls_rsa_rsaes_pkcs1_v15_encrypt(ctx, f_rng, p_rng,
                                                       ilen, input, output, diagnostics));

        case MBEDTLS_RSA_PKCS_V21:
            MINTLS_RETURN(mbedtls_rsa_rsaes_oaep_encrypt(ctx, f_rng, p_rng, NULL, 0,
                                                  ilen, input, output, diagnostics));

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }
}

/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-DECRYPT function
 */
int mbedtls_rsa_rsaes_oaep_decrypt(mbedtls_rsa_context *ctx,
                                   int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                   void *p_rng,
                                   const unsigned char *label, size_t label_len,
                                   size_t *olen,
                                   const unsigned char *input,
                                   unsigned char *output,
                                   size_t output_max_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t ilen, i, pad_len;
    unsigned char *p;
    mbedtls_ct_condition_t bad, in_padding;
    unsigned char buf[MBEDTLS_MPI_MAX_SIZE];
    unsigned char lhash[MBEDTLS_MD_MAX_SIZE];
    unsigned int hlen;

    /*
     * Parameters sanity checks
     */
    if (ctx->padding != MBEDTLS_RSA_PKCS_V21) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    ilen = ctx->len;

    if (ilen < 16 || ilen > sizeof(buf)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    hlen = mbedtls_md_get_size_from_type((mbedtls_md_type_t) ctx->hash_id);
    if (hlen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    // checking for integer underflow
    if (2 * hlen + 2 > ilen) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /*
     * RSA operation
     */
    ret = mbedtls_rsa_private(ctx, f_rng, p_rng, input, buf, diagnostics);

    if (ret != 0) {
        goto cleanup;
    }

    /*
     * Unmask data and generate lHash
     */
    /* seed: Apply seedMask to maskedSeed */
    if ((ret = mgf_mask(buf + 1, hlen, buf + hlen + 1, ilen - hlen - 1,
                        (mbedtls_md_type_t) ctx->hash_id, diagnostics)) != 0 ||
        /* DB: Apply dbMask to maskedDB */
        (ret = mgf_mask(buf + hlen + 1, ilen - hlen - 1, buf + 1, hlen,
                        (mbedtls_md_type_t) ctx->hash_id, diagnostics)) != 0) {
        goto cleanup;
    }

    /* Generate lHash */
    ret = compute_hash((mbedtls_md_type_t) ctx->hash_id,
                       label, label_len, lhash, diagnostics);
    if (ret != 0) {
        goto cleanup;
    }

    /*
     * Check contents, in "constant-time"
     */
    p = buf;

    bad = mbedtls_ct_bool(*p++); /* First byte must be 0 */

    p += hlen; /* Skip seed */

    /* Check lHash */
    bad = mbedtls_ct_bool_or(bad, mbedtls_ct_bool(mbedtls_ct_memcmp(lhash, p, hlen)));
    p += hlen;

    /* Get zero-padding len, but always read till end of buffer
     * (minus one, for the 01 byte) */
    pad_len = 0;
    in_padding = MBEDTLS_CT_TRUE;
    for (i = 0; i < ilen - 2 * hlen - 2; i++) {
        in_padding = mbedtls_ct_bool_and(in_padding, mbedtls_ct_uint_eq(p[i], 0));
        pad_len += mbedtls_ct_uint_if_else_0(in_padding, 1);
    }

    p += pad_len;
    bad = mbedtls_ct_bool_or(bad, mbedtls_ct_uint_ne(*p++, 0x01));

    /*
     * The only information "leaked" is whether the padding was correct or not
     * (eg, no data is copied if it was not correct). This meets the
     * recommendations in PKCS#1 v2.2: an opponent cannot distinguish between
     * the different error conditions.
     */
    if (bad != MBEDTLS_CT_FALSE) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_INVALID_PADDING);
        goto cleanup;
    }

    if (ilen - ((size_t) (p - buf)) > output_max_len) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE);
        goto cleanup;
    }

    *olen = ilen - ((size_t) (p - buf));
    if (*olen != 0) {
        memcpy(output, p, *olen);
    }
    ret = 0;

cleanup:
    mbedtls_platform_zeroize(buf, sizeof(buf));
    mbedtls_platform_zeroize(lhash, sizeof(lhash));

    MINTLS_RETURN(ret);
}

/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-DECRYPT function
 */
int mbedtls_rsa_rsaes_pkcs1_v15_decrypt(mbedtls_rsa_context *ctx,
                                        int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                        void *p_rng,
                                        size_t *olen,
                                        const unsigned char *input,
                                        unsigned char *output,
                                        size_t output_max_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t ilen;
    unsigned char buf[MBEDTLS_MPI_MAX_SIZE];

    ilen = ctx->len;

    if (ctx->padding != MBEDTLS_RSA_PKCS_V15) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (ilen < 16 || ilen > sizeof(buf)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    ret = mbedtls_rsa_private(ctx, f_rng, p_rng, input, buf, diagnostics);

    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_ct_rsaes_pkcs1_v15_unpadding(buf, ilen,
                                               output, output_max_len, olen);

cleanup:
    mbedtls_platform_zeroize(buf, sizeof(buf));

    MINTLS_RETURN(ret);
}

/*
 * Do an RSA operation, then remove the message padding
 */
int mbedtls_rsa_pkcs1_decrypt(mbedtls_rsa_context *ctx,
                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                              void *p_rng,
                              size_t *olen,
                              const unsigned char *input,
                              unsigned char *output,
                              size_t output_max_len, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    switch (ctx->padding) {
        case MBEDTLS_RSA_PKCS_V15:
            MINTLS_RETURN(mbedtls_rsa_rsaes_pkcs1_v15_decrypt(ctx, f_rng, p_rng, olen,
                                                       input, output, output_max_len, diagnostics));

        case MBEDTLS_RSA_PKCS_V21:
            MINTLS_RETURN(mbedtls_rsa_rsaes_oaep_decrypt(ctx, f_rng, p_rng, NULL, 0,
                                                  olen, input, output,
                                                  output_max_len, diagnostics));

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }
}

int rsa_rsassa_pss_sign_no_mode_check(mbedtls_rsa_context *ctx,
                                             int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                             void *p_rng,
                                             mbedtls_md_type_t md_alg,
                                             unsigned int hashlen,
                                             const unsigned char *hash,
                                             int saltlen,
                                             unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t olen;
    unsigned char *p = sig;
    unsigned char *salt = NULL;
    size_t slen, min_slen, hlen, offset = 0;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t msb;
    mbedtls_md_type_t hash_id;

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (f_rng == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    olen = ctx->len;

    if (md_alg != MBEDTLS_MD_NONE) {
        /* Gather length of hash to sign */
        size_t exp_hashlen = mbedtls_md_get_size_from_type(md_alg);
        if (exp_hashlen == 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        if (hashlen != exp_hashlen) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }
    }

    hash_id = (mbedtls_md_type_t) ctx->hash_id;
    if (hash_id == MBEDTLS_MD_NONE) {
        hash_id = md_alg;
    }
    hlen = mbedtls_md_get_size_from_type(hash_id);
    if (hlen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (saltlen == MBEDTLS_RSA_SALT_LEN_ANY) {
        /* Calculate the largest possible salt length, up to the hash size.
         * Normally this is the hash length, which is the maximum salt length
         * according to FIPS 185-4 §5.5 (e) and common practice. If there is not
         * enough room, use the maximum salt length that fits. The constraint is
         * that the hash length plus the salt length plus 2 bytes must be at most
         * the key length. This complies with FIPS 186-4 §5.5 (e) and RFC 8017
         * (PKCS#1 v2.2) §9.1.1 step 3. */
        min_slen = hlen - 2;
        if (olen < hlen + min_slen + 2) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        } else if (olen >= hlen + hlen + 2) {
            slen = hlen;
        } else {
            slen = olen - hlen - 2;
        }
    } else if ((saltlen < 0) || (saltlen + hlen + 2 > olen)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    } else {
        slen = (size_t) saltlen;
    }

    memset(sig, 0, olen);

    /* Note: EMSA-PSS encoding is over the length of N - 1 bits */
    msb = mbedtls_mpi_bitlen(&ctx->N) - 1;
    p += olen - hlen - slen - 2;
    *p++ = 0x01;

    /* Generate salt of length slen in place in the encoded message */
    salt = p;
    if ((ret = f_rng(p_rng, salt, slen, diagnostics)) != 0) {
        MINTLS_RETURN(MBEDTLS_ERROR_ADD(MBEDTLS_ERR_RSA_RNG_FAILED, ret));
    }

    p += slen;

    /* Generate H = Hash( M' ) */
    ret = hash_mprime(hash, hashlen, salt, slen, p, hash_id, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    /* Compensate for boundary condition when applying mask */
    if (msb % 8 == 0) {
        offset = 1;
    }

    /* maskedDB: Apply dbMask to DB */
    ret = mgf_mask(sig + offset, olen - hlen - 1 - offset, p, hlen, hash_id, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    msb = mbedtls_mpi_bitlen(&ctx->N) - 1;
    sig[0] &= 0xFF >> (olen * 8 - msb);

    p += hlen;
    *p++ = 0xBC;

    MINTLS_RETURN(mbedtls_rsa_private(ctx, f_rng, p_rng, sig, sig, diagnostics));
}

int rsa_rsassa_pss_sign(mbedtls_rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                               void *p_rng,
                               mbedtls_md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               int saltlen,
                               unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if (ctx->padding != MBEDTLS_RSA_PKCS_V21) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
    if ((ctx->hash_id == MBEDTLS_MD_NONE) && (md_alg == MBEDTLS_MD_NONE)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
    MINTLS_RETURN(rsa_rsassa_pss_sign_no_mode_check(ctx, f_rng, p_rng, md_alg, hashlen, hash, saltlen,
                                             sig, diagnostics));
}

int mbedtls_rsa_rsassa_pss_sign_no_mode_check(mbedtls_rsa_context *ctx,
                                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                              void *p_rng,
                                              mbedtls_md_type_t md_alg,
                                              unsigned int hashlen,
                                              const unsigned char *hash,
                                              unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(rsa_rsassa_pss_sign_no_mode_check(ctx, f_rng, p_rng, md_alg,
                                             hashlen, hash, MBEDTLS_RSA_SALT_LEN_ANY, sig, diagnostics));
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-SIGN function with
 * the option to pass in the salt length.
 */
int mbedtls_rsa_rsassa_pss_sign_ext(mbedtls_rsa_context *ctx,
                                    int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                    void *p_rng,
                                    mbedtls_md_type_t md_alg,
                                    unsigned int hashlen,
                                    const unsigned char *hash,
                                    int saltlen,
                                    unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(rsa_rsassa_pss_sign(ctx, f_rng, p_rng, md_alg,
                               hashlen, hash, saltlen, sig, diagnostics));
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-SIGN function
 */
int mbedtls_rsa_rsassa_pss_sign(mbedtls_rsa_context *ctx,
                                int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                void *p_rng,
                                mbedtls_md_type_t md_alg,
                                unsigned int hashlen,
                                const unsigned char *hash,
                                unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN(rsa_rsassa_pss_sign(ctx, f_rng, p_rng, md_alg,
                               hashlen, hash, MBEDTLS_RSA_SALT_LEN_ANY, sig, diagnostics));
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-V1_5-SIGN function
 */

/* Construct a PKCS v1.5 encoding of a hashed message
 *
 * This is used both for signature generation and verification.
 *
 * Parameters:
 * - md_alg:  Identifies the hash algorithm used to generate the given hash;
 *            MBEDTLS_MD_NONE if raw data is signed.
 * - hashlen: Length of hash. Must match md_alg if that's not NONE.
 * - hash:    Buffer containing the hashed message or the raw data.
 * - dst_len: Length of the encoded message.
 * - dst:     Buffer to hold the encoded message.
 *
 * Assumptions:
 * - hash has size hashlen.
 * - dst points to a buffer of size at least dst_len.
 *
 */
int rsa_rsassa_pkcs1_v15_encode(mbedtls_md_type_t md_alg,
                                       unsigned int hashlen,
                                       const unsigned char *hash,
                                       size_t dst_len,
                                       unsigned char *dst, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    size_t oid_size  = 0;
    size_t nb_pad    = dst_len;
    unsigned char *p = dst;
    const char *oid  = NULL;

    /* Are we signing hashed or raw data? */
    if (md_alg != MBEDTLS_MD_NONE) {
        unsigned char md_size = mbedtls_md_get_size_from_type(md_alg);
        if (md_size == 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        if (mbedtls_oid_get_oid_by_md(md_alg, &oid, &oid_size, diagnostics) != 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        if (hashlen != md_size) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        /* Double-check that 8 + hashlen + oid_size can be used as a
         * 1-byte ASN.1 length encoding and that there's no overflow. */
        if (8 + hashlen + oid_size  >= 0x80         ||
            10 + hashlen            <  hashlen      ||
            10 + hashlen + oid_size <  10 + hashlen) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        /*
         * Static bounds check:
         * - Need 10 bytes for five tag-length pairs.
         *   (Insist on 1-byte length encodings to protect against variants of
         *    Bleichenbacher's forgery attack against lax PKCS#1v1.5 verification)
         * - Need hashlen bytes for hash
         * - Need oid_size bytes for hash alg OID.
         */
        if (nb_pad < 10 + hashlen + oid_size) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }
        nb_pad -= 10 + hashlen + oid_size;
    } else {
        if (nb_pad < hashlen) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        nb_pad -= hashlen;
    }

    /* Need space for signature header and padding delimiter (3 bytes),
     * and 8 bytes for the minimal padding */
    if (nb_pad < 3 + 8) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
    nb_pad -= 3;

    /* Now nb_pad is the amount of memory to be filled
     * with padding, and at least 8 bytes long. */

    /* Write signature header and padding */
    *p++ = 0;
    *p++ = MBEDTLS_RSA_SIGN;
    memset(p, 0xFF, nb_pad);
    p += nb_pad;
    *p++ = 0;

    /* Are we signing raw data? */
    if (md_alg == MBEDTLS_MD_NONE) {
        memcpy(p, hash, hashlen);
        MINTLS_RETURN(0);
    }

    /* Signing hashed data, add corresponding ASN.1 structure
     *
     * DigestInfo ::= SEQUENCE {
     *   digestAlgorithm DigestAlgorithmIdentifier,
     *   digest Digest }
     * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
     * Digest ::= OCTET STRING
     *
     * Schematic:
     * TAG-SEQ + LEN [ TAG-SEQ + LEN [ TAG-OID  + LEN [ OID  ]
     *                                 TAG-NULL + LEN [ NULL ] ]
     *                 TAG-OCTET + LEN [ HASH ] ]
     */
    *p++ = MBEDTLS_ASN1_SEQUENCE | MBEDTLS_ASN1_CONSTRUCTED;
    *p++ = (unsigned char) (0x08 + oid_size + hashlen);
    *p++ = MBEDTLS_ASN1_SEQUENCE | MBEDTLS_ASN1_CONSTRUCTED;
    *p++ = (unsigned char) (0x04 + oid_size);
    *p++ = MBEDTLS_ASN1_OID;
    *p++ = (unsigned char) oid_size;
    memcpy(p, oid, oid_size);
    p += oid_size;
    *p++ = MBEDTLS_ASN1_NULL;
    *p++ = 0x00;
    *p++ = MBEDTLS_ASN1_OCTET_STRING;
    *p++ = (unsigned char) hashlen;
    memcpy(p, hash, hashlen);
    p += hashlen;

    /* Just a sanity-check, should be automatic
     * after the initial bounds check. */
    if (p != dst + dst_len) {
        mbedtls_platform_zeroize(dst, dst_len);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    MINTLS_RETURN(0);
}

/*
 * Do an RSA operation to sign the message digest
 */
int mbedtls_rsa_rsassa_pkcs1_v15_sign(mbedtls_rsa_context *ctx,
                                      int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                      void *p_rng,
                                      mbedtls_md_type_t md_alg,
                                      unsigned int hashlen,
                                      const unsigned char *hash,
                                      unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *sig_try = NULL, *verif = NULL;

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    if (ctx->padding != MBEDTLS_RSA_PKCS_V15) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /*
     * Prepare PKCS1-v1.5 encoding (padding and hash identifier)
     */

    if ((ret = rsa_rsassa_pkcs1_v15_encode(md_alg, hashlen, hash,
                                           ctx->len, sig, diagnostics)) != 0) {
        MINTLS_RETURN(ret);
    }

    /* Private key operation
     *
     * In order to prevent Lenstra's attack, make the signature in a
     * temporary buffer and check it before returning it.
     */

    sig_try = mbedtls_calloc(1, ctx->len);
    if (sig_try == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MPI_ALLOC_FAILED);
    }

    verif = mbedtls_calloc(1, ctx->len);
    if (verif == NULL) {
        mbedtls_free(sig_try);
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_MPI_ALLOC_FAILED);
    }

    MBEDTLS_MPI_CHK(mbedtls_rsa_private(ctx, f_rng, p_rng, sig, sig_try, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_rsa_public(ctx, sig_try, verif, diagnostics));

    if (mbedtls_ct_memcmp(verif, sig, ctx->len) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_PRIVATE_FAILED);
        goto cleanup;
    }

    memcpy(sig, sig_try, ctx->len);

cleanup:
    mbedtls_zeroize_and_free(sig_try, ctx->len);
    mbedtls_zeroize_and_free(verif, ctx->len);

    if (ret != 0) {
        memset(sig, '!', ctx->len);
    }
    MINTLS_RETURN(ret);
}

/*
 * Do an RSA operation to sign the message digest
 */
int mbedtls_rsa_pkcs1_sign(mbedtls_rsa_context *ctx,
                           int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                           void *p_rng,
                           mbedtls_md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    switch (ctx->padding) {
        case MBEDTLS_RSA_PKCS_V15:
            MINTLS_RETURN(mbedtls_rsa_rsassa_pkcs1_v15_sign(ctx, f_rng, p_rng,
                                                     md_alg, hashlen, hash, sig, diagnostics));

        case MBEDTLS_RSA_PKCS_V21:
            MINTLS_RETURN(mbedtls_rsa_rsassa_pss_sign(ctx, f_rng, p_rng, md_alg,
                                               hashlen, hash, sig, diagnostics));

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }
}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int mbedtls_rsa_rsassa_pss_verify_ext(mbedtls_rsa_context *ctx,
                                      mbedtls_md_type_t md_alg,
                                      unsigned int hashlen,
                                      const unsigned char *hash,
                                      mbedtls_md_type_t mgf1_hash_id,
                                      int expected_salt_len,
                                      const unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t siglen;
    unsigned char *p;
    unsigned char *hash_start;
    unsigned char result[MBEDTLS_MD_MAX_SIZE];
    unsigned int hlen;
    size_t observed_salt_len, msb;
    unsigned char buf[MBEDTLS_MPI_MAX_SIZE] = { 0 };

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    siglen = ctx->len;

    if (siglen < 16 || siglen > sizeof(buf)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    ret = mbedtls_rsa_public(ctx, sig, buf, diagnostics);

    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    p = buf;

    if (buf[siglen - 1] != 0xBC) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }

    if (md_alg != MBEDTLS_MD_NONE) {
        /* Gather length of hash to sign */
        size_t exp_hashlen = mbedtls_md_get_size_from_type(md_alg);
        if (exp_hashlen == 0) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }

        if (hashlen != exp_hashlen) {
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
        }
    }

    hlen = mbedtls_md_get_size_from_type(mgf1_hash_id);
    if (hlen == 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /*
     * Note: EMSA-PSS verification is over the length of N - 1 bits
     */
    msb = mbedtls_mpi_bitlen(&ctx->N) - 1;

    if (buf[0] >> (8 - siglen * 8 + msb)) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    /* Compensate for boundary condition when applying mask */
    if (msb % 8 == 0) {
        p++;
        siglen -= 1;
    }

    if (siglen < hlen + 2) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }
    hash_start = p + siglen - hlen - 1;

    ret = mgf_mask(p, siglen - hlen - 1, hash_start, hlen, mgf1_hash_id, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    buf[0] &= 0xFF >> (siglen * 8 - msb);

    while (p < hash_start - 1 && *p == 0) {
        p++;
    }

    if (*p++ != 0x01) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }

    observed_salt_len = (size_t) (hash_start - p);

    if (expected_salt_len != MBEDTLS_RSA_SALT_LEN_ANY &&
        observed_salt_len != (size_t) expected_salt_len) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }

    /*
     * Generate H = Hash( M' )
     */
    ret = hash_mprime(hash, hashlen, p, observed_salt_len,
                      result, mgf1_hash_id, diagnostics);
    if (ret != 0) {
        MINTLS_RETURN(ret);
    }

    if (memcmp(hash_start, result, hlen) != 0) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_VERIFY_FAILED);
    }

    MINTLS_RETURN(0);
}

/*
 * Simplified PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int mbedtls_rsa_rsassa_pss_verify(mbedtls_rsa_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  unsigned int hashlen,
                                  const unsigned char *hash,
                                  const unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    mbedtls_md_type_t mgf1_hash_id;
    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    mgf1_hash_id = (ctx->hash_id != MBEDTLS_MD_NONE)
                             ? (mbedtls_md_type_t) ctx->hash_id
                             : md_alg;

    MINTLS_RETURN(mbedtls_rsa_rsassa_pss_verify_ext(ctx,
                                             md_alg, hashlen, hash,
                                             mgf1_hash_id,
                                             MBEDTLS_RSA_SALT_LEN_ANY,
                                             sig, diagnostics));

}

/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-v1_5-VERIFY function
 */
int mbedtls_rsa_rsassa_pkcs1_v15_verify(mbedtls_rsa_context *ctx,
                                        mbedtls_md_type_t md_alg,
                                        unsigned int hashlen,
                                        const unsigned char *hash,
                                        const unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = 0;
    size_t sig_len;
    unsigned char *encoded = NULL, *encoded_expected = NULL;

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    sig_len = ctx->len;

    /*
     * Prepare expected PKCS1 v1.5 encoding of hash.
     */

    if ((encoded          = mbedtls_calloc(1, sig_len)) == NULL ||
        (encoded_expected = mbedtls_calloc(1, sig_len)) == NULL) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_MPI_ALLOC_FAILED);
        goto cleanup;
    }

    if ((ret = rsa_rsassa_pkcs1_v15_encode(md_alg, hashlen, hash, sig_len,
                                           encoded_expected, diagnostics)) != 0) {
        goto cleanup;
    }

    /*
     * Apply RSA primitive to get what should be PKCS1 encoded hash.
     */

    ret = mbedtls_rsa_public(ctx, sig, encoded, diagnostics);
    if (ret != 0) {
        goto cleanup;
    }

    /*
     * Compare
     */

    if ((ret = mbedtls_ct_memcmp(encoded, encoded_expected,
                                 sig_len)) != 0) {
        ret = MinTlsAssignDiagnostic(diagnostics, __func__, __FILE__, __LINE__, MBEDTLS_ERR_RSA_VERIFY_FAILED);
        goto cleanup;
    }

cleanup:

    if (encoded != NULL) {
        mbedtls_zeroize_and_free(encoded, sig_len);
    }

    if (encoded_expected != NULL) {
        mbedtls_zeroize_and_free(encoded_expected, sig_len);
    }

    MINTLS_RETURN(ret);
}

/*
 * Do an RSA operation and check the message digest
 */
int mbedtls_rsa_pkcs1_verify(mbedtls_rsa_context *ctx,
                             mbedtls_md_type_t md_alg,
                             unsigned int hashlen,
                             const unsigned char *hash,
                             const unsigned char *sig, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    if ((md_alg != MBEDTLS_MD_NONE || hashlen != 0) && hash == NULL) {
        MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_BAD_INPUT_DATA);
    }

    switch (ctx->padding) {
        case MBEDTLS_RSA_PKCS_V15:
            MINTLS_RETURN(mbedtls_rsa_rsassa_pkcs1_v15_verify(ctx, md_alg,
                                                       hashlen, hash, sig, diagnostics));

        case MBEDTLS_RSA_PKCS_V21:
            MINTLS_RETURN(mbedtls_rsa_rsassa_pss_verify(ctx, md_alg,
                                                 hashlen, hash, sig, diagnostics));

        default:
            MINTLS_RETURN_ERROR(MBEDTLS_ERR_RSA_INVALID_PADDING);
    }
}

/*
 * Copy the components of an RSA key
 */
int mbedtls_rsa_copy(mbedtls_rsa_context *dst, const mbedtls_rsa_context *src, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    dst->len = src->len;

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->N, &src->N, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->E, &src->E, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->D, &src->D, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->P, &src->P, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Q, &src->Q, diagnostics));

#if !defined(MBEDTLS_RSA_NO_CRT)
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->DP, &src->DP, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->DQ, &src->DQ, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->QP, &src->QP, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RP, &src->RP, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RQ, &src->RQ, diagnostics));
#endif

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RN, &src->RN, diagnostics));

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Vi, &src->Vi, diagnostics));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Vf, &src->Vf, diagnostics));

    dst->padding = src->padding;
    dst->hash_id = src->hash_id;

cleanup:
    if (ret != 0) {
        mbedtls_rsa_free(dst);
    }

    MINTLS_RETURN(ret);
}

/*
 * Free the components of an RSA key
 */
void mbedtls_rsa_free(mbedtls_rsa_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_mpi_free(&ctx->Vi);
    mbedtls_mpi_free(&ctx->Vf);
    mbedtls_mpi_free(&ctx->RN);
    mbedtls_mpi_free(&ctx->D);
    mbedtls_mpi_free(&ctx->Q);
    mbedtls_mpi_free(&ctx->P);
    mbedtls_mpi_free(&ctx->E);
    mbedtls_mpi_free(&ctx->N);

#if !defined(MBEDTLS_RSA_NO_CRT)
    mbedtls_mpi_free(&ctx->RQ);
    mbedtls_mpi_free(&ctx->RP);
    mbedtls_mpi_free(&ctx->QP);
    mbedtls_mpi_free(&ctx->DQ);
    mbedtls_mpi_free(&ctx->DP);
#endif /* MBEDTLS_RSA_NO_CRT */

}

