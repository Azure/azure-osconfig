// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * \file rsa_internal.h
 *
 * \brief Internal-only RSA public-key cryptosystem API.
 *
 * This file declares RSA-related functions that are to be used
 * only from within the Mbed TLS library itself.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig on 2026-10-06: private MinTls profile, flat
 * source layout and per-call failure diagnostics. Original Mbed TLS 3.6.7 file: library/rsa_internal.h.
 */
#ifndef MBEDTLS_RSA_INTERNAL_H
#define MBEDTLS_RSA_INTERNAL_H

#include "rsa.h"
#include "asn1.h"

/**
 * \brief           Parse a PKCS#1 (ASN.1) encoded private RSA key.
 *
 * \param rsa       The RSA context where parsed data will be stored.
 * \param key       The buffer that contains the key.
 * \param keylen    The length of the key buffer in bytes.
 *
 * \return          0 on success.
 * \return          MBEDTLS_ERR_ASN1_xxx in case of ASN.1 parsing errors.
 * \return          MBEDTLS_ERR_RSA_xxx in case of RSA internal failures while
 *                  parsing data.
 * \return          MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if validity checks on the
 *                  provided key fail.
 */
int mbedtls_rsa_parse_key(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Parse a PKCS#1 (ASN.1) encoded public RSA key.
 *
 * \param rsa       The RSA context where parsed data will be stored.
 * \param key       The buffer that contains the key.
 * \param keylen    The length of the key buffer in bytes.
 *
 * \return          0 on success.
 * \return          MBEDTLS_ERR_ASN1_xxx in case of ASN.1 parsing errors.
 * \return          MBEDTLS_ERR_RSA_xxx in case of RSA internal failures while
 *                  parsing data.
 * \return          MBEDTLS_ERR_RSA_KEY_CHECK_FAILED if validity checks on the
 *                  provided key fail.
 */
int mbedtls_rsa_parse_pubkey(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Write a PKCS#1 (ASN.1) encoded private RSA key.
 *
 * \param rsa       The RSA context which contains the data to be written.
 * \param start     Beginning of the buffer that will be filled with the
 *                  private key.
 * \param p         End of the buffer that will be filled with the private key.
 *                  On successful return, the referenced pointer will be
 *                  updated in order to point to the beginning of written data.
 *
 * \return          On success, the number of bytes written to the output buffer
 *                  (i.e. a value > 0).
 * \return          MBEDTLS_ERR_RSA_BAD_INPUT_DATA if the RSA context does not
 *                  contain a valid key pair.
 * \return          MBEDTLS_ERR_ASN1_xxx in case of failure while writing to the
 *                  output buffer.
 *
 * \note            The output buffer is filled backward, i.e. starting from its
 *                  end and moving toward its start.
 */
int mbedtls_rsa_write_key(const mbedtls_rsa_context *rsa, unsigned char *start,
                          unsigned char **p, MinTlsDiagnostics* diagnostics);

/**
 * \brief           Parse a PKCS#1 (ASN.1) encoded public RSA key.
 *
 * \param rsa       The RSA context which contains the data to be written.
 * \param start     Beginning of the buffer that will be filled with the
 *                  private key.
 * \param p         End of the buffer that will be filled with the private key.
 *                  On successful return, the referenced pointer will be
 *                  updated in order to point to the beginning of written data.
 *
 * \return          On success, the number of bytes written to the output buffer
 *                  (i.e. a value > 0).
 * \return          MBEDTLS_ERR_RSA_BAD_INPUT_DATA if the RSA context does not
 *                  contain a valid public key.
 * \return          MBEDTLS_ERR_ASN1_xxx in case of failure while writing to the
 *                  output buffer.
 *
 * \note            The output buffer is filled backward, i.e. starting from its
 *                  end and moving toward its start.
 */
int mbedtls_rsa_write_pubkey(const mbedtls_rsa_context *rsa, unsigned char *start,
                             unsigned char **p, MinTlsDiagnostics* diagnostics);

/**
 * \brief This function is analogue to \c mbedtls_rsa_rsassa_pss_sign().
 *        The only difference between them is that this function is more flexible
 *        on the parameters of \p ctx that are set with \c mbedtls_rsa_set_padding().
 *
 * \note  Compared to its counterpart, this function:
 *        - does not check the padding setting of \p ctx.
 *        - allows the hash_id of \p ctx to be MBEDTLS_MD_NONE,
 *          in which case it uses \p md_alg as the hash_id.
 *
 * \note  Refer to \c mbedtls_rsa_rsassa_pss_sign() for a description
 *        of the functioning and parameters of this function.
 */
int mbedtls_rsa_rsassa_pss_sign_no_mode_check(mbedtls_rsa_context *ctx,
                                              int (*f_rng)(void *, unsigned char *, size_t, MinTlsDiagnostics* diagnostics),
                                              void *p_rng,
                                              mbedtls_md_type_t md_alg,
                                              unsigned int hashlen,
                                              const unsigned char *hash,
                                              unsigned char *sig, MinTlsDiagnostics* diagnostics);

/* This would normally be in rsa_invasive.h but it didn't exist before 3.6
 * became an LTS, and I'd rather not add files in LTS if it can be avoided. */

/** Decompose sensitive return values out of a return code, in constant time.
 *
 * \param invalid_padding_in    The value of \p combined_ret that indicates
 *                              invalid padding.
 * \param invalid_padding_out   The value to set \p problem to in case of
 *                              invalid padding.
 * \param output_too_large_in   The value of \p combined_ret that indicates
 *                              an insufficient output buffer size.
 * \param output_too_large_out  The value to set \p problem to in case of
 *                              an insufficient output buffer size.
 * \param combined_ret          The value to decompose.
 * \param[out] problem          On output:
 *                              - \p invalid_padding_out,
 *                                if \p combined_ret = \p invalid_padding_in;
 *                              - \p output_too_large_out,
 *                                if \p combined_ret = \p output_too_large_in;
 *                              - otherwise \c 0.
 *
 * \return                      - \c 0 if \p combined_ret = \p invalid_padding_in
 *                                or \p combined_ret = \p output_too_large_in;
 *                              - otherwise \c combined_ret.
 */
int mbedtls_rsa_decrypt_decompose_ret(
    int invalid_padding_in, int invalid_padding_out,
    int output_too_large_in, int output_too_large_out,
    int combined_ret,
    int *problem);

#endif /* rsa_internal.h */
