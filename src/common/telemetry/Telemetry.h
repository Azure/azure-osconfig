// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <version.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"
#define TELEMETRY_TIMEOUT_SECONDS 10

#define TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR "activityId"
#define TELEMETRY_RULECODENAME_ENVIRONMENT_VAR "_RuleCodename"

// Buffer sizes for string conversion of numeric values
#define MAX_NUM_STRING_LENGTH 32    // Accommodates 64-bit int/long values

#ifdef __cplusplus
extern "C" {
#endif

void OSConfigTelemetryInit(void);
void OSConfigTelemetryCleanup(void);
void OSConfigTelemetryAppendJSON(const char* jsonString);
FILE* OSConfigTelemetryGetFile(void);
const char* OSConfigTelemetryGetFileName(void);
const char* OSConfigTelemetryGetModuleDirectory(void);
int OSConfigTelemetryIsInitialized(void);

#ifdef __cplusplus
}
#endif

#ifdef DEBUG
#define VERBOSE_FLAG_IF_DEBUG "-v"
#else
#define VERBOSE_FLAG_IF_DEBUG ""
#endif

#define OSConfigProcessTelemetryFile() { \
    FILE* telemetryFileMacro = OSConfigTelemetryGetFile(); \
    const char* telemetryFileNameMacro = OSConfigTelemetryGetFileName(); \
    const char* telemetryModuleDirectoryMacro = OSConfigTelemetryGetModuleDirectory(); \
    if (telemetryFileMacro && telemetryFileNameMacro && telemetryModuleDirectoryMacro) { \
        char* command = FormatAllocateString("%s/%s %s %s %d", telemetryModuleDirectoryMacro, TELEMETRY_BINARY_NAME, VERBOSE_FLAG_IF_DEBUG, telemetryFileNameMacro, TELEMETRY_TIMEOUT_SECONDS); \
        OSConfigTelemetryCleanup(); \
        if (NULL != command) { \
            ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_TIMEOUT_SECONDS, NULL, NULL, NULL); \
        } \
        FREE_MEMORY(command); \
    } \
}

#define OSConfigTelemetryStatusTrace(callingFunctionName, status) { \
    OSConfigTelemetryStatusTraceImpl((callingFunctionName), (status), __LINE__); \
}
#define OSConfigTelemetryStatusTraceImpl(callingFunctionName, status, line) { \
    char status_str[MAX_NUM_STRING_LENGTH] = {0}; \
    snprintf(status_str, sizeof(status_str), "%d", (status)); \
    char line_str[MAX_NUM_STRING_LENGTH] = {0}; \
    snprintf(line_str, sizeof(line_str), "%d", (line)); \
    const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR); \
    const char* ruleCodename = getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR); \
    const char* timestamp = GetFormattedTime(); \
    char* distroName = GetOsName(NULL); \
    char* telemetry_json = FormatAllocateString("{" \
        "\"EventName\":\"StatusTrace\"," \
        "\"Timestamp\":\"%s\"," \
        "\"FileName\":\"%s\"," \
        "\"LineNumber\":\"%s\"," \
        "\"FunctionName\":\"%s\"," \
        "\"RuleCodename\":\"%s\"," \
        "\"CallingFunctionName\":\"%s\"," \
        "\"ResultCode\":\"%s\"," \
        "\"DistroName\":\"%s\"," \
        "\"CorrelationId\":\"%s\"," \
        "\"Version\":\"%s\"" \
        "}", timestamp ? timestamp : "", __FILE__, line_str, __func__, ruleCodename ? ruleCodename : "", (callingFunctionName) ? (callingFunctionName) : "-", status_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); \
    if (NULL != telemetry_json) { \
        OSConfigTelemetryAppendJSON(telemetry_json); \
    } \
    FREE_MEMORY(distroName); \
    FREE_MEMORY(telemetry_json); \
}

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds) { \
    char durationSeconds_str[MAX_NUM_STRING_LENGTH] = {0}; \
    snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds)); \
    const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR); \
    const char* timestamp = GetFormattedTime(); \
    char* distroName = GetOsName(NULL); \
    char* telemetry_json = FormatAllocateString("{" \
        "\"EventName\":\"BaselineRun\"," \
        "\"Timestamp\":\"%s\"," \
        "\"BaselineName\":\"%s\"," \
        "\"Mode\":\"%s\"," \
        "\"DurationSeconds\":\"%s\"," \
        "\"DistroName\":\"%s\"," \
        "\"CorrelationId\":\"%s\"," \
        "\"Version\":\"%s\"" \
        "}", timestamp ? timestamp : "", (baselineName) ? (baselineName) : "N/A", (mode) ? (mode) : "N/A", durationSeconds_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); \
    if (NULL != telemetry_json) { \
        OSConfigTelemetryAppendJSON(telemetry_json); \
    } \
    FREE_MEMORY(distroName); \
    FREE_MEMORY(telemetry_json); \
}

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds) { \
    char objectResult_str[MAX_NUM_STRING_LENGTH] = {0}; \
    snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult)); \
    char microseconds_str[MAX_NUM_STRING_LENGTH] = {0}; \
    snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds)); \
    const char* correlationId = getenv("activityId"); \
    const char* timestamp = GetFormattedTime(); \
    char* distroName = GetOsName(NULL); \
    char* telemetry_json = FormatAllocateString("{" \
        "\"EventName\":\"RuleComplete\"," \
        "\"Timestamp\":\"%s\"," \
        "\"ComponentName\":\"%s\"," \
        "\"ObjectName\":\"%s\"," \
        "\"ObjectResult\":\"%s\"," \
        "\"Microseconds\":\"%s\"," \
        "\"DistroName\":\"%s\"," \
        "\"CorrelationId\":\"%s\"," \
        "\"Version\":\"%s\"" \
        "}", timestamp ? timestamp : "", (componentName) ? (componentName) : "N/A", (objectName) ? (objectName) : "N/A", objectResult_str, microseconds_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); \
    if (NULL != telemetry_json) { \
        OSConfigTelemetryAppendJSON(telemetry_json); \
    } \
    FREE_MEMORY(distroName); \
    FREE_MEMORY(telemetry_json); \
}

#endif // TELEMETRY_H
