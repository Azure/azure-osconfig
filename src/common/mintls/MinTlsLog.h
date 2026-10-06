// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MIN_TLS_LOG_H
#define MIN_TLS_LOG_H

#include <Logging.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MinTlsDiagnostic
{
    const char* operation;
    const char* file;
    int line;
    int result;
} MinTlsDiagnostic;

typedef struct MinTlsDiagnosticFrame
{
    struct MinTlsDiagnosticFrame* parent;
    MinTlsDiagnostic failure;
    int result;
    bool positiveIsFailure;
} MinTlsDiagnosticFrame;

typedef struct MinTlsDiagnostics
{
    OsConfigLogHandle log;
    MinTlsDiagnosticFrame* frame;
} MinTlsDiagnostics;

// Keep speculative failures local to their call. Only a failed outermost
// operation emits a diagnostic; successful parser/verification probes discard it.
// Frames start zeroed. Positive lengths are success unless the function's
// contract explicitly uses positive failure counts or alert codes.
void MinTlsBeginDiagnostic(MinTlsDiagnostics* diagnostics, MinTlsDiagnosticFrame* frame);
int MinTlsEndDiagnostic(MinTlsDiagnostics* diagnostics, MinTlsDiagnosticFrame* frame,
    const char* operation, const char* file, int line, int result, bool direct);
int MinTlsCombineDiagnostic(MinTlsDiagnostics* diagnostics, int high, int low);
int MinTlsMapDiagnostic(MinTlsDiagnostics* diagnostics, int result);
int MinTlsAssignDiagnostic(MinTlsDiagnostics* diagnostics, const char* operation,
    const char* file, int line, int result);
void MinTlsRecordDiagnostic(MinTlsDiagnostics* diagnostics, const char* operation,
    const char* file, int line, int result);

#define MINTLS_BEGIN_DIAGNOSTIC() \
    MinTlsDiagnosticFrame diagnosticFrame = {NULL, {NULL, NULL, 0, 0}, 0, false}; \
    MinTlsBeginDiagnostic(diagnostics, &diagnosticFrame)
#define MINTLS_RETURN(result) \
    return MinTlsEndDiagnostic(diagnostics, &diagnosticFrame, __func__, __FILE__, __LINE__, (result), false)
#define MINTLS_RETURN_ERROR(result) \
    return MinTlsEndDiagnostic(diagnostics, &diagnosticFrame, __func__, __FILE__, __LINE__, (result), true)
// Use only when the error branch translates a failed child's result.
#define MINTLS_RETURN_CAUSE(result) \
    return MinTlsEndDiagnostic(diagnostics, &diagnosticFrame, __func__, __FILE__, __LINE__, MinTlsMapDiagnostic(diagnostics, (result)), false)

#ifdef __cplusplus
}
#endif

#endif
