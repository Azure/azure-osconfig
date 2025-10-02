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
#include <sys/wait.h>
#include <unistd.h>

#include <Logging.h>
#include <CommonUtils.h>

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
// Based on maximum possible digits for each type plus sign and null terminator
#define MAX_INT_STRING_LENGTH 16    // Accommodates 32-bit int values
#define MAX_LONG_STRING_LENGTH 32   // Accommodates 64-bit long values

// Static file handle for telemetry logging with thread safety
static FILE* g_telemetryFile __attribute__((unused)) = NULL;
static char* g_fileName __attribute__((unused)) = NULL;
static char* g_moduleDirectory __attribute__((unused)) = NULL;
static int g_telemetryFileInitialized __attribute__((unused)) = 0;

#ifdef DEBUG
#define VERBOSE_FLAG_IF_DEBUG "-v"
#else
#define VERBOSE_FLAG_IF_DEBUG ""
#endif

// Initialize telemetry file - call once at program start (thread-safe)
#define OSConfigTelemetryInit() do { if (!g_telemetryFileInitialized) { static int initLock = 0; if (__sync_bool_compare_and_swap(&initLock, 0, 1)) { if (!g_telemetryFile) { g_moduleDirectory = getModuleDirectory(); g_fileName = generateRandomFilename(); g_telemetryFile = fopen(g_fileName, "a"); if (g_telemetryFile) { g_telemetryFileInitialized = 1; } } } else { while (!g_telemetryFileInitialized) { usleep(1000); } } } } while(0)

// Cleanup telemetry file - call on program exit
#define OSConfigProcessTelemetryFile() do { if (g_telemetryFile && g_fileName && g_moduleDirectory) { char* command = FormatAllocateString("%s/%s %s %s %d", g_moduleDirectory, TELEMETRY_BINARY_NAME, VERBOSE_FLAG_IF_DEBUG, g_fileName, TELEMETRY_TIMEOUT_SECONDS); if (NULL != command) { ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_TIMEOUT_SECONDS, NULL, NULL, NULL); } FREE_MEMORY(command); } g_telemetryFile = NULL; g_telemetryFileInitialized = 0; FREE_MEMORY(g_fileName); FREE_MEMORY(g_moduleDirectory); } while(0)

// Helper macro to append raw JSON string to telemetry file
#define OSConfigTelemetryAppendJSON(jsonString) do { if (!g_telemetryFileInitialized) { OSConfigTelemetryInit(); } if (g_telemetryFile) { fprintf(g_telemetryFile, "%s\n", jsonString); fflush(g_telemetryFile); } } while(0)

#define OSConfigTelemetryStatusTrace(callingFunctionName, status) do { char status_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(status_str, sizeof(status_str), "%d", (status)); char line_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(line_str, sizeof(line_str), "%d", __LINE__); const char* distroName = GetOsName(NULL); const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR); const char* ruleCodename = getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR); const char* timestamp = GetFormattedTime(); char* telemetry_json = FormatAllocateString("{\"EventName\":\"StatusTrace\",\"Timestamp\":\"%s\",\"Filename\":\"%s\",\"LineNumber\":\"%s\",\"FunctionName\":\"%s\",\"RuleCodename\":\"%s\",\"CallingFunctionName\":\"%s\",\"ResultCode\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", timestamp ? timestamp : "", __FILE__, line_str, __func__, ruleCodename ? ruleCodename : "", (callingFunctionName) ? (callingFunctionName) : "-", status_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); if (NULL != telemetry_json) { OSConfigTelemetryAppendJSON(telemetry_json); } FREE_MEMORY(telemetry_json); } while(0)

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds) do { char durationSeconds_str[MAX_LONG_STRING_LENGTH] = {0}; snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds)); const char* distroName = GetOsName(NULL); const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR); const char* timestamp = GetFormattedTime(); char* telemetry_json = FormatAllocateString("{\"EventName\":\"BaselineRun\",\"Timestamp\":\"%s\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"DurationSeconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", timestamp ? timestamp : "", (baselineName) ? (baselineName) : "N/A", (mode) ? (mode) : "N/A", durationSeconds_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); if (NULL != telemetry_json) { OSConfigTelemetryAppendJSON(telemetry_json); } FREE_MEMORY(telemetry_json); } while(0)

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds) do { char objectResult_str[MAX_INT_STRING_LENGTH] = {0}; snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult)); char microseconds_str[MAX_LONG_STRING_LENGTH] = {0}; snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds)); const char* distroName = GetOsName(NULL); const char* correlationId = getenv("activityId"); const char* timestamp = GetFormattedTime(); char* telemetry_json = FormatAllocateString("{\"EventName\":\"RuleComplete\",\"Timestamp\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s\",\"Microseconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", timestamp ? timestamp : "", (componentName) ? (componentName) : "N/A", (objectName) ? (objectName) : "N/A", objectResult_str, microseconds_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION); if (NULL != telemetry_json) { OSConfigTelemetryAppendJSON(telemetry_json); } FREE_MEMORY(telemetry_json); } while(0)

#endif // TELEMETRY_H
