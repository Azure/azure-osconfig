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

// Helper macro to generate random filename
#define generateRandomFilename() ({ \
    char temp_template[] = "/tmp/telemetry_XXXXXX"; \
    int fd = mkstemp(temp_template); \
    char* filename_result = NULL; \
    if (fd != -1) { \
        close(fd); \
        if (unlink(temp_template) == 0) { \
            filename_result = (char*)malloc(strlen(temp_template) + 6); \
            if (filename_result) { \
                strcpy(filename_result, temp_template); \
                strcat(filename_result, ".json"); \
            } \
        } \
    } \
    filename_result; \
})

// Helper macro to get module directory
#define getModuleDirectory() ({ \
    Dl_info dl_info; \
    char* directory_result = NULL; \
    if (dladdr((void*)__func__, &dl_info) != 0) { \
        if (dl_info.dli_fname != NULL) { \
            const char* module_fullPath = dl_info.dli_fname; \
            const char* lastSlash = strrchr(module_fullPath, '/'); \
            if (lastSlash != NULL) { \
                size_t dirLen = lastSlash - module_fullPath; \
                directory_result = (char*)malloc(dirLen + 1); \
                if (directory_result) { \
                    strncpy(directory_result, module_fullPath, dirLen); \
                    directory_result[dirLen] = '\0'; \
                } \
            } \
        } \
    } \
    directory_result; \
})

// Buffer sizes for string conversion of numeric values
#define MAX_NUM_STRING_LENGTH 32    // Accommodates 64-bit int/long values

// Static file handle for telemetry logging
static FILE* g_telemetryFile __attribute__((unused)) = NULL;
static char* g_fileName __attribute__((unused)) = NULL;
static char* g_moduleDirectory __attribute__((unused)) = NULL;
static int g_telemetryFileInitialized __attribute__((unused)) = 0;

#ifdef DEBUG
#define VERBOSE_FLAG_IF_DEBUG "-v"
#else
#define VERBOSE_FLAG_IF_DEBUG ""
#endif

// Initialize telemetry file - call once at start
#define OSConfigTelemetryInit() { \
    if (!g_telemetryFileInitialized) { \
        if (!g_telemetryFileInitialized && !g_telemetryFile) { \
            g_moduleDirectory = getModuleDirectory(); \
            g_fileName = generateRandomFilename(); \
            g_telemetryFile = fopen(g_fileName, "a"); \
            if (g_telemetryFile) { \
                g_telemetryFileInitialized = 1; \
            } \
        } \
    } \
}

// Cleanup telemetry file - call on program exit
#define OSConfigProcessTelemetryFile() { \
    if (g_telemetryFile && g_fileName && g_moduleDirectory) { \
        char* command = FormatAllocateString("%s/%s %s %s %d", g_moduleDirectory, TELEMETRY_BINARY_NAME, VERBOSE_FLAG_IF_DEBUG, g_fileName, TELEMETRY_TIMEOUT_SECONDS); \
        if (NULL != command) { \
            ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_TIMEOUT_SECONDS, NULL, NULL, NULL); \
        } \
        FREE_MEMORY(command); \
    } \
    g_telemetryFile = NULL; \
    g_telemetryFileInitialized = 0; \
    FREE_MEMORY(g_fileName); \
    FREE_MEMORY(g_moduleDirectory); \
}

// Helper macro to append raw JSON string to telemetry file
#define OSConfigTelemetryAppendJSON(jsonString) { \
    if (!g_telemetryFileInitialized) { \
        OSConfigTelemetryInit(); \
    } \
    if (g_telemetryFile) { \
        fprintf(g_telemetryFile, "%s\n", jsonString); \
        fflush(g_telemetryFile); \
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
        "\"Filename\":\"%s\"," \
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
