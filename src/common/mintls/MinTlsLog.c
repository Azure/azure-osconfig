// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MinTlsLog.h"
#include "tls.h"
#include <errno.h>

bool MinTlsIsDiagnosticFailure(int result)
{
    return (result < 0) && (MBEDTLS_ERR_SSL_WANT_READ != result) &&
        (MBEDTLS_ERR_SSL_WANT_WRITE != result) && (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY != result);
}

const char* MinTlsCoreFailureReason(int result)
{
    if (result >= 0)
    {
        return NULL;
    }

    // Core errors combine a high-level code with an optional low-level code.
    switch (-(int)((0u - (unsigned int)result) & 0xFF80u))
    {
        case MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE:
            return "feature unavailable in the fixed MinTls profile";
        case MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE:
            return "unsupported elliptic-curve operation or group";
        case MBEDTLS_ERR_PK_UNKNOWN_PK_ALG:
        case MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG:
            return "unsupported key or certificate signature algorithm";
        case MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION:
            return "peer protocol version is outside the TLS 1.2 profile";
        case MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL:
            return "peer has no matching application protocol";
        case MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE:
            return "TLS handshake negotiation failed";
        case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
            return "peer rejected the TLS operation";
        case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED:
            return "certificate chain, identity, validity or policy verification failed";
        default:
            return NULL;
    }
}

void MinTlsBeginDiagnostic(MinTlsDiagnostics* diagnostics, MinTlsDiagnosticFrame* frame)
{
    frame->parent = diagnostics->frame;
    diagnostics->frame = frame;
}

void MinTlsEmitDiagnostic(MinTlsDiagnostics* diagnostics, const MinTlsDiagnostic* failure, int result)
{
    int savedErrno = errno;
    const char* reason = MinTlsCoreFailureReason(failure->result);

    if (!reason)
    {
        reason = MinTlsCoreFailureReason(result);
    }

    OsConfigLogError(diagnostics->log, "MinTls core: %s:%d: %s failed, core status: %d, originating status: %d, reason: %s", failure->file, failure->line, failure->operation, result, failure->result, reason ? reason : "see originating operation and status");

    errno = savedErrno;
}

int MinTlsEndDiagnostic(MinTlsDiagnostics* diagnostics, MinTlsDiagnosticFrame* frame,
    const char* operation, const char* file, int line, int result, bool direct)
{
    MinTlsDiagnostic failure = frame->failure;

    diagnostics->frame = frame->parent;

    if ((MinTlsIsDiagnosticFailure(result)) || ((frame->positiveIsFailure) && (result > 0)))
    {
        if ((direct) || (frame->result != result) || (!failure.operation))
        {
            failure = (MinTlsDiagnostic){operation, file, line, result};
        }

        if (frame->parent)
        {
            frame->parent->failure = failure;
            frame->parent->result = result;
        }
        else
        {
            MinTlsEmitDiagnostic(diagnostics, &failure, result);
        }
    }

    return result;
}

int MinTlsCombineDiagnostic(MinTlsDiagnostics* diagnostics, int high, int low)
{
    int result = high + low;

    if ((diagnostics->frame) && (diagnostics->frame->result == low))
    {
        diagnostics->frame->result = result;
    }

    return result;
}

int MinTlsAssignDiagnostic(MinTlsDiagnostics* diagnostics, const char* operation,
    const char* file, int line, int result)
{
    if (diagnostics->frame)
    {
        diagnostics->frame->failure = (MinTlsDiagnostic){operation, file, line, result};
        diagnostics->frame->result = result;
    }

    return result;
}

int MinTlsMapDiagnostic(MinTlsDiagnostics* diagnostics, int result)
{
    if (diagnostics->frame)
    {
        diagnostics->frame->result = result;
    }

    return result;
}

void MinTlsRecordDiagnostic(MinTlsDiagnostics* diagnostics, const char* operation,
    const char* file, int line, int result)
{
    MinTlsDiagnostic failure = {operation, file, line, result};

    if (MinTlsIsDiagnosticFailure(result))
    {
        if (diagnostics->frame)
        {
            if ((diagnostics->frame->result != result) || (!diagnostics->frame->failure.operation))
            {
                diagnostics->frame->failure = failure;
                diagnostics->frame->result = result;
            }
        }
        else
        {
            MinTlsEmitDiagnostic(diagnostics, &failure, result);
        }
    }
}
