/**
 * \file private_access.h
 *
 * \brief Macro wrapper for struct's members.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: include/mbedtls/private_access.h.
 */

#ifndef MBEDTLS_PRIVATE_ACCESS_H
#define MBEDTLS_PRIVATE_ACCESS_H

#ifndef MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_PRIVATE(member) private_##member
#else
#define MBEDTLS_PRIVATE(member) member
#endif

#endif /* MBEDTLS_PRIVATE_ACCESS_H */
