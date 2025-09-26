// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef INTERNAL_H
#define INTERNAL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <mntent.h>
#include <dirent.h>
#include <math.h>
#include <libgen.h>
#include <shadow.h>
#include <parson.h>
#include <Logging.h>
#include <Reasons.h>
#include <CommonUtils.h>
#include <Telemetry.h>
#include <version.h>

#include "../asb/Asb.h"

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 30))
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define INT_ENOENT -999

#define MAX_STRING_LENGTH 512

// Buffer sizes for string conversion of numeric values
// Based on maximum possible digits for each type plus sign and null terminator
#define MAX_INT_STRING_LENGTH 16    // Accommodates 32-bit int values
#define MAX_LONG_STRING_LENGTH 32   // Accommodates 64-bit long values

// Static file handle for telemetry logging with thread safety
static FILE* g_telemetryFile = NULL;
static int g_telemetryFileInitialized = 0;

// Initialize telemetry file - call once at program start (thread-safe)
#define OSConfigTelemetryInit() do { \
    if (!g_telemetryFileInitialized) { \
        static int initLock = 0; \
        if (__sync_bool_compare_and_swap(&initLock, 0, 1)) { \
            if (!g_telemetryFile) { \
                g_telemetryFile = fopen("/tmp/osconfig_telemetry.json", "a"); \
                if (g_telemetryFile) { \
                    atexit(OSConfigTelemetryCleanup); \
                    g_telemetryFileInitialized = 1; \
                } \
            } \
        } else { \
            while (!g_telemetryFileInitialized) { \
                usleep(1000); \
            } \
        } \
    } \
} while(0)

// Cleanup telemetry file - automatically called at program exit
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void OSConfigTelemetryCleanup(void) {
    if (g_telemetryFile) {
        fclose(g_telemetryFile);
        g_telemetryFile = NULL;
        g_telemetryFileInitialized = 0;
    }
}
#pragma GCC diagnostic pop

// Helper macro to append raw JSON string to telemetry file
#define OSConfigTelemetryAppendJSON(jsonString) do { \
    if (!g_telemetryFileInitialized) { \
        OSConfigTelemetryInit(); \
    } \
    if (g_telemetryFile) { \
        fprintf(g_telemetryFile, "%s\n", (jsonString)); \
        fflush(g_telemetryFile); \
    } \
} while(0)

#define OSConfigTelemetryStatusTrace(callingFunctionName, status) do { \
    char status_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(status_str, sizeof(status_str), "%d", (status)); \
    char line_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(line_str, sizeof(line_str), "%d", __LINE__); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"StatusTrace\",\"Filename\":\"%s\",\"LineNumber\":\"%s\",\"FunctionName\":\"%s\",\"CallingFunctionName\":\"%s\",\"ResultCode\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        __FILE__, line_str, __func__, \
        (callingFunctionName) ? (callingFunctionName) : "-", \
        status_str, \
        distroName ? distroName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds) do { \
    char durationSeconds_str[MAX_LONG_STRING_LENGTH] = {0}; \
    snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds)); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"BaselineRun\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"DurationSeconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        (baselineName) ? (baselineName) : "N/A", \
        (mode) ? (mode) : "N/A", \
        durationSeconds_str, \
        distroName ? distroName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds) do { \
    char objectResult_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult)); \
    char microseconds_str[MAX_LONG_STRING_LENGTH] = {0}; \
    snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds)); \
    const char* correlationId = getenv("activityId"); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"RuleComplete\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s\",\"Microseconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        (componentName) ? (componentName) : "N/A", \
        (objectName) ? (objectName) : "N/A", \
        objectResult_str, \
        microseconds_str, \
        g_prettyName ? g_prettyName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#endif // INTERNAL_H
