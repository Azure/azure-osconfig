// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "common.h"
#include "debug.h"
#include <Logging.h>
#include <errno.h>
#include <stdio.h>

void MinTlsLogResult(const mbedtls_ssl_context* ssl, int level, const char* file,
    int line, const char* operation, int result)
{
    int savedErrno = errno;
    char message[256] = {0};

    if ((result < 0) && (MBEDTLS_ERR_SSL_WANT_READ != result) &&
        (MBEDTLS_ERR_SSL_WANT_WRITE != result) && (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY != result) &&
        (ssl) && (ssl->conf) && (ssl->conf->f_dbg))
    {
        snprintf(message, sizeof(message), "%s, core status: %d", operation, result);
        ssl->conf->f_dbg(ssl->conf->p_dbg, level, file, line, message);
    }

    errno = savedErrno;
}

void MinTlsLogCallback(void* context, int level, const char* file, int line, const char* message)
{
    OsConfigLogHandle log = context;
    int savedErrno = errno;

    if (level <= 1)
    {
        OsConfigLogError(log, "MinTls core: %s:%d: %s", file, line, message);
    }
    else
    {
        OsConfigLogDebug(log, "MinTls core: %s:%d: %s", file, line, message);
    }

    errno = savedErrno;
}
