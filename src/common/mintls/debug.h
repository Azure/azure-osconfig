/**
 * \file debug.h
 *
 * \brief Functions for controlling and providing debug output from the library.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * Modified by Microsoft for OSConfig: private MinTls profile and flat
 * source layout. Original Mbed TLS 3.6.7 file: include/mbedtls/debug.h.
 */
#ifndef MBEDTLS_DEBUG_H
#define MBEDTLS_DEBUG_H

#include "build_info.h"

#include "ssl.h"

#include "ecp.h"

#define MBEDTLS_SSL_DEBUG_MSG(level, args)            do { } while (0)
#define MBEDTLS_SSL_DEBUG_RET(level, text, ret) MinTlsLogResult(ssl, level, __FILE__, __LINE__, text, ret)
#define MBEDTLS_SSL_DEBUG_BUF(level, text, buf, len)  do { } while (0)
#define MBEDTLS_SSL_DEBUG_MPI(level, text, X)         do { } while (0)
#define MBEDTLS_SSL_DEBUG_ECP(level, text, X)         do { } while (0)
#define MBEDTLS_SSL_DEBUG_CRT(level, text, crt)       do { } while (0)
#define MBEDTLS_SSL_DEBUG_ECDH(level, ecdh, attr)     do { } while (0)

/**
 * \def MBEDTLS_PRINTF_ATTRIBUTE
 *
 * Mark a function as having printf attributes, and thus enable checking
 * via -wFormat and other flags. This does nothing on builds with compilers
 * that do not support the format attribute
 *
 * Module:  library/debug.c
 * Caller:
 *
 * This module provides debugging functions.
 */
#if defined(__has_attribute)
#if __has_attribute(format)
#if defined(__MINGW32__) && __USE_MINGW_ANSI_STDIO == 1
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((__format__(gnu_printf, string_index, first_to_check)))
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((format(printf, string_index, first_to_check)))
#endif
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif /* __has_attribute(format) */
#else
#define MBEDTLS_PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif

/**
 * \def MBEDTLS_PRINTF_SIZET
 *
 * MBEDTLS_PRINTF_xxx: Due to issues with older window compilers
 * and MinGW we need to define the printf specifier for size_t
 * and long long per platform.
 *
 * Module:  library/debug.c
 * Caller:
 *
 * This module provides debugging functions.
 */
#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900)
   #include <inttypes.h>
   #define MBEDTLS_PRINTF_SIZET     PRIuPTR
   #define MBEDTLS_PRINTF_LONGLONG  PRId64
#else
   #define MBEDTLS_PRINTF_SIZET     "zu"
   #define MBEDTLS_PRINTF_LONGLONG  "lld"
#endif \
    /* defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900) */

#if !defined(MBEDTLS_PRINTF_MS_TIME)
#include <inttypes.h>
#if !defined(PRId64)
#define MBEDTLS_PRINTF_MS_TIME MBEDTLS_PRINTF_LONGLONG
#else
#define MBEDTLS_PRINTF_MS_TIME PRId64
#endif
#endif /* MBEDTLS_PRINTF_MS_TIME */

#ifdef __cplusplus
extern "C" {
#endif

void MinTlsLogResult(const mbedtls_ssl_context* ssl, int level, const char* file,
    int line, const char* operation, int result);
void MinTlsLogCallback(void* context, int level, const char* file, int line, const char* message);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_DEBUG_H */
