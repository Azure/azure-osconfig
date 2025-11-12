// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Logging.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <version.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"
#define TELEMETRY_TIMEOUT_SECONDS 10

#define TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR "activityId"
#define TELEMETRY_RULECODENAME_ENVIRONMENT_VAR "_RuleCodename"
#define TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR "_ScenarioName"
#define TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR "_Microseconds"

// Buffer sizes for string conversion of numeric values
#define MAX_NUM_STRING_LENGTH 32 // Accommodates 64-bit int/long values

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef BUILD_TELEMETRY
void OSConfigTelemetryInit(void);
void OSConfigTelemetryCleanup(void);
void OSConfigTelemetrySetLogger(const OsConfigLogHandle log);
void OSConfigTelemetryAppendJSON(const char* jsonString);
FILE* OSConfigTelemetryGetFile(void);
const char* OSConfigTelemetryGetFileName(void);
const char* OSConfigTelemetryGetModuleDirectory(void);
int OSConfigTelemetryIsInitialized(void);
#else
static inline void OSConfigTelemetryInit(void)
{
}
static inline void OSConfigTelemetryCleanup(void)
{
}
static inline void OSConfigTelemetrySetLogger(const OsConfigLogHandle log)
{
    (void)log;
}
static inline void OSConfigTelemetryAppendJSON(const char* jsonString)
{
    (void)jsonString;
}
static inline FILE* OSConfigTelemetryGetFile(void)
{
    return NULL;
}
static inline const char* OSConfigTelemetryGetFileName(void)
{
    return NULL;
}
static inline const char* OSConfigTelemetryGetModuleDirectory(void)
{
    return NULL;
}
static inline int OSConfigTelemetryIsInitialized(void)
{
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define VERBOSE_FLAG_IF_DEBUG "-v"
#else
#define VERBOSE_FLAG_IF_DEBUG ""
#endif

static inline int64_t TsToUs(struct timespec ts)
{
    return (int64_t)ts.tv_sec * 1000000LL + (int64_t)ts.tv_nsec / 1000LL;
}

#ifdef BUILD_TELEMETRY

#define OSConfigTimeStampSave()                                                                                                                        \
    {                                                                                                                                                  \
        struct timespec _start;                                                                                                                        \
        clock_gettime(CLOCK_MONOTONIC, &_start);                                                                                                       \
        char _start_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                                  \
        snprintf(_start_str, sizeof(_start_str), "%" PRId64, TsToUs(_start));                                                                          \
        setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR, _start_str, 1);                                                                                 \
    }

#define OSConfigGetElapsedTime(elapsed_us_var)                                                                                                         \
    {                                                                                                                                                  \
        struct timespec _end;                                                                                                                          \
        const char* _start_str = getenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR);                                                                       \
        char* end;                                                                                                                                     \
        int64_t _start_us = (_start_str != NULL) ? strtoll(_start_str, &end, 10) : 0;                                                                  \
        if (_start_us == 0)                                                                                                                            \
        {                                                                                                                                              \
            (elapsed_us_var) = 0;                                                                                                                      \
        }                                                                                                                                              \
        else                                                                                                                                           \
        {                                                                                                                                              \
            clock_gettime(CLOCK_MONOTONIC, &_end);                                                                                                     \
            (elapsed_us_var) = TsToUs(_end) - _start_us;                                                                                               \
        }                                                                                                                                              \
    }

#define OSConfigProcessTelemetryFile()                                                                                                                 \
    {                                                                                                                                                  \
        FILE* telemetryFileMacro = OSConfigTelemetryGetFile();                                                                                         \
        const char* telemetryFileNameMacro = OSConfigTelemetryGetFileName();                                                                           \
        const char* telemetryModuleDirectoryMacro = OSConfigTelemetryGetModuleDirectory();                                                             \
        if (telemetryFileMacro && telemetryFileNameMacro && telemetryModuleDirectoryMacro)                                                             \
        {                                                                                                                                              \
            char* command = FormatAllocateString("%s/%s -f %s -t %d %s", telemetryModuleDirectoryMacro, TELEMETRY_BINARY_NAME, telemetryFileNameMacro, \
                TELEMETRY_TIMEOUT_SECONDS, VERBOSE_FLAG_IF_DEBUG);                                                                                     \
            if (NULL != command)                                                                                                                       \
            {                                                                                                                                          \
                ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_TIMEOUT_SECONDS, NULL, NULL, NULL);                                           \
                FREE_MEMORY(command);                                                                                                                  \
            }                                                                                                                                          \
            OSConfigTelemetryCleanup();                                                                                                                \
        }                                                                                                                                              \
    }

#define OSConfigTelemetryStatusTrace(callingFunctionName, status)                                                                                      \
    {                                                                                                                                                  \
        OSConfigTelemetryStatusTraceImpl((callingFunctionName), (status), __LINE__);                                                                   \
    }
#define OSConfigTelemetryStatusTraceImpl(callingFunctionName, status, line)                                                                                      \
    {                                                                                                                                                            \
        char* telemetry_json = NULL;                                                                                                                             \
        char status_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                                            \
        char line_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                                              \
        const char* _correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);                                                                            \
        const char* _ruleCodename = getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR);                                                                              \
        const char* _scenarioName = getenv(TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR);                                                                              \
        const char* _timestamp = GetFormattedTime();                                                                                                             \
        int64_t _elapsed_us = 0;                                                                                                                                 \
        char* _distroName = GetOsName(NULL);                                                                                                                     \
        snprintf(status_str, sizeof(status_str), "%d", (status));                                                                                                \
        snprintf(line_str, sizeof(line_str), "%d", (line));                                                                                                      \
        OSConfigGetElapsedTime(_elapsed_us);                                                                                                                     \
        telemetry_json = FormatAllocateString(                                                                                                                   \
            "{"                                                                                                                                                  \
            "\"EventName\":\"StatusTrace\","                                                                                                                     \
            "\"Timestamp\":\"%s\","                                                                                                                              \
            "\"FileName\":\"%s\","                                                                                                                               \
            "\"LineNumber\":\"%s\","                                                                                                                             \
            "\"FunctionName\":\"%s\","                                                                                                                           \
            "\"RuleCodename\":\"%s\","                                                                                                                           \
            "\"CallingFunctionName\":\"%s\","                                                                                                                    \
            "\"ResultCode\":\"%s\","                                                                                                                             \
            "\"ScenarioName\":\"%s\","                                                                                                                           \
            "\"Microseconds\":\"%" PRId64                                                                                                                        \
            "\","                                                                                                                                                \
            "\"DistroName\":\"%s\","                                                                                                                             \
            "\"CorrelationId\":\"%s\","                                                                                                                          \
            "\"Version\":\"%s\""                                                                                                                                 \
            "}",                                                                                                                                                 \
            _timestamp ? _timestamp : "", __FILE__, line_str, __func__, _ruleCodename ? _ruleCodename : "", (callingFunctionName) ? (callingFunctionName) : "-", \
            status_str, _scenarioName, _elapsed_us, _distroName ? _distroName : "unknown", _correlationId ? _correlationId : "", OSCONFIG_VERSION);              \
        if (NULL != telemetry_json)                                                                                                                              \
        {                                                                                                                                                        \
            OSConfigTelemetryAppendJSON(telemetry_json);                                                                                                         \
        }                                                                                                                                                        \
        FREE_MEMORY(_distroName);                                                                                                                                \
        FREE_MEMORY(telemetry_json);                                                                                                                             \
    }

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds)                                                                              \
    {                                                                                                                                                  \
        char* telemetry_json = NULL;                                                                                                                   \
        char durationSeconds_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                         \
        const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);                                                                   \
        const char* timestamp = GetFormattedTime();                                                                                                    \
        char* distroName = GetOsName(NULL);                                                                                                            \
        snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds));                                                 \
        telemetry_json = FormatAllocateString(                                                                                                         \
            "{"                                                                                                                                        \
            "\"EventName\":\"BaselineRun\","                                                                                                           \
            "\"Timestamp\":\"%s\","                                                                                                                    \
            "\"BaselineName\":\"%s\","                                                                                                                 \
            "\"Mode\":\"%s\","                                                                                                                         \
            "\"DurationSeconds\":\"%s\","                                                                                                              \
            "\"DistroName\":\"%s\","                                                                                                                   \
            "\"CorrelationId\":\"%s\","                                                                                                                \
            "\"Version\":\"%s\""                                                                                                                       \
            "}",                                                                                                                                       \
            timestamp ? timestamp : "", (baselineName) ? (baselineName) : "N/A", (mode) ? (mode) : "N/A", durationSeconds_str,                         \
            distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION);                                                \
        if (NULL != telemetry_json)                                                                                                                    \
        {                                                                                                                                              \
            OSConfigTelemetryAppendJSON(telemetry_json);                                                                                               \
        }                                                                                                                                              \
        FREE_MEMORY(distroName);                                                                                                                       \
        FREE_MEMORY(telemetry_json);                                                                                                                   \
    }

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds)                                                           \
    {                                                                                                                                                  \
        char* telemetry_json = NULL;                                                                                                                   \
        char objectResult_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                            \
        char microseconds_str[MAX_NUM_STRING_LENGTH] = {0};                                                                                            \
        const char* correlationId = getenv("activityId");                                                                                              \
        const char* timestamp = GetFormattedTime();                                                                                                    \
        char* distroName = GetOsName(NULL);                                                                                                            \
        snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult));                                                                    \
        snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds));                                                             \
        telemetry_json = FormatAllocateString(                                                                                                         \
            "{"                                                                                                                                        \
            "\"EventName\":\"RuleComplete\","                                                                                                          \
            "\"Timestamp\":\"%s\","                                                                                                                    \
            "\"ComponentName\":\"%s\","                                                                                                                \
            "\"ObjectName\":\"%s\","                                                                                                                   \
            "\"ObjectResult\":\"%s\","                                                                                                                 \
            "\"Microseconds\":\"%s\","                                                                                                                 \
            "\"DistroName\":\"%s\","                                                                                                                   \
            "\"CorrelationId\":\"%s\","                                                                                                                \
            "\"Version\":\"%s\""                                                                                                                       \
            "}",                                                                                                                                       \
            timestamp ? timestamp : "", (componentName) ? (componentName) : "N/A", (objectName) ? (objectName) : "N/A", objectResult_str,              \
            microseconds_str, distroName ? distroName : "unknown", correlationId ? correlationId : "", OSCONFIG_VERSION);                              \
        if (NULL != telemetry_json)                                                                                                                    \
        {                                                                                                                                              \
            OSConfigTelemetryAppendJSON(telemetry_json);                                                                                               \
        }                                                                                                                                              \
        FREE_MEMORY(distroName);                                                                                                                       \
        FREE_MEMORY(telemetry_json);                                                                                                                   \
    }

#else // BUILD_TELEMETRY

#define OSConfigTimeStampSave()                                                                                                                        \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
    } while (0)
#define OSConfigGetElapsedTime(elapsed_us_var)                                                                                                         \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (elapsed_us_var) = 0;                                                                                                                          \
    } while (0)
#define OSConfigProcessTelemetryFile()                                                                                                                 \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
    } while (0)
#define OSConfigTelemetryStatusTrace(callingFunctionName, status)                                                                                      \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (void)(callingFunctionName);                                                                                                                   \
        (void)(status);                                                                                                                                \
    } while (0)
#define OSConfigTelemetryStatusTraceImpl(callingFunctionName, status, line)                                                                            \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (void)(callingFunctionName);                                                                                                                   \
        (void)(status);                                                                                                                                \
        (void)(line);                                                                                                                                  \
    } while (0)
#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds)                                                                              \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (void)(baselineName);                                                                                                                          \
        (void)(mode);                                                                                                                                  \
        (void)(durationSeconds);                                                                                                                       \
    } while (0)
#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds)                                                           \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (void)(componentName);                                                                                                                         \
        (void)(objectName);                                                                                                                            \
        (void)(objectResult);                                                                                                                          \
        (void)(microseconds);                                                                                                                          \
    } while (0)

#endif // BUILD_TELEMETRY

#endif // TELEMETRY_H
