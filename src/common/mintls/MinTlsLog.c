// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MinTlsLog.h"
#include "ssl.h"
#include <errno.h>

bool MinTlsIsDiagnosticFailure(int result)
{
    return (result < 0) && (MBEDTLS_ERR_SSL_WANT_READ != result) &&
        (MBEDTLS_ERR_SSL_WANT_WRITE != result) && (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY != result);
}

void MinTlsBeginDiagnostic(MinTlsDiagnostics* diagnostics, MinTlsDiagnosticFrame* frame)
{
    frame->parent = diagnostics->frame;
    diagnostics->frame = frame;
}

void MinTlsEmitDiagnostic(MinTlsDiagnostics* diagnostics, const MinTlsDiagnostic* failure, int result)
{
    int savedErrno = errno;

    OsConfigLogError(diagnostics->log, "MinTls core: %s:%d: %s failed, core status: %d, originating status: %d", failure->file, failure->line, failure->operation, result, failure->result);

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
