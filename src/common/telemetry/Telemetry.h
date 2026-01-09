// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <CommonUtils.h>
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
#define TELEMETRY_TMP_FILE_NAME "/tmp/osconfig_telemetry.jsonl"
#define TELEMETRY_COMMAND_TIMEOUT_SECONDS 10
#define TELEMETRY_TEARDOWN_TIMEOUT_SECONDS 8 // Must be less than TELEMETRY_COMMAND_TIMEOUT_SECONDS
#define TELEMETRY_NOTFOUND_STRING "N/A"

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
char* GetModuleDirectory(void);
char* GetCachedDistroName(void);

void TelemetryInitialize(const OsConfigLogHandle log);
void TelemetryCleanup(const OsConfigLogHandle log);
void TelemetryAppendJson(const char* jsonString);
#else
static char* GetModuleDirectory(void)
{
    return NULL;
}

static char* GetCachedDistroName(void)
{
    return NULL;
}

static inline void TelemetryInitialize(const OsConfigLogHandle log)
{
    (void)log;
}

static inline void TelemetryCleanup(const OsConfigLogHandle log)
{
    (void)log;
}

static inline void TelemetryAppendJson(const char* jsonString)
{
    (void)jsonString;
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
static inline void OSConfigTimeStampSave(void)
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    char start_str[MAX_NUM_STRING_LENGTH] = {0};
    snprintf(start_str, sizeof(start_str), "%" PRId64, TsToUs(start));
    setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR, start_str, 1);
}

static inline void OSConfigGetElapsedTime(int64_t* elapsed_us_var)
{
    if (elapsed_us_var == NULL)
    {
        return;
    }

    struct timespec end;
    const char* start_str = getenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR);
    char* end_ptr;
    int64_t start_us = (start_str != NULL) ? strtoll(start_str, &end_ptr, 10) : 0;

    if (start_us == 0)
    {
        *elapsed_us_var = 0;
    }
    else
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
        *elapsed_us_var = TsToUs(end) - start_us;
    }
}

#define OSConfigTelemetryStatusTrace(callingFunctionName, status)                                                                                      \
    {                                                                                                                                                  \
        OSConfigTelemetryStatusTraceImpl((callingFunctionName), (status), __LINE__);                                                                   \
    }
#define OSConfigTelemetryStatusTraceImpl(callingFunctionName, status, line)                                                                            \
    {                                                                                                                                                  \
        char* telemetry_json = NULL;                                                                                                                   \
        const char* _correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);                                                                  \
        const char* _ruleCodename = getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR);                                                                    \
        const char* _scenarioName = getenv(TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR);                                                                    \
        const char* _timestamp = GetFormattedTime();                                                                                                   \
        int64_t _elapsed_us = 0;                                                                                                                       \
        const char* _distroName = GetCachedDistroName();                                                                                               \
        const char* _resultString = strerror(status);                                                                                                  \
        OSConfigGetElapsedTime(&_elapsed_us);                                                                                                          \
        telemetry_json = FormatAllocateString(                                                                                                         \
            "{"                                                                                                                                        \
            "\"EventName\":\"StatusTrace\","                                                                                                           \
            "\"Timestamp\":\"%s\","                                                                                                                    \
            "\"FileName\":\"%s\","                                                                                                                     \
            "\"LineNumber\":\"%d\","                                                                                                                   \
            "\"FunctionName\":\"%s\","                                                                                                                 \
            "\"RuleCodename\":\"%s\","                                                                                                                 \
            "\"CallingFunctionName\":\"%s\","                                                                                                          \
            "\"ResultCode\":\"%d\","                                                                                                                   \
            "\"ResultString\":\"%s\","                                                                                                                 \
            "\"ScenarioName\":\"%s\","                                                                                                                 \
            "\"Microseconds\":\"%" PRId64                                                                                                              \
            "\","                                                                                                                                      \
            "\"DistroName\":\"%s\","                                                                                                                   \
            "\"CorrelationId\":\"%s\","                                                                                                                \
            "\"Version\":\"%s\""                                                                                                                       \
            "}",                                                                                                                                       \
            _timestamp ? _timestamp : TELEMETRY_NOTFOUND_STRING, __FILE__, line, __func__, _ruleCodename ? _ruleCodename : TELEMETRY_NOTFOUND_STRING,  \
            (callingFunctionName) ? (callingFunctionName) : TELEMETRY_NOTFOUND_STRING, status, _resultString, _scenarioName, _elapsed_us,              \
            _distroName ? _distroName : TELEMETRY_NOTFOUND_STRING, _correlationId ? _correlationId : TELEMETRY_NOTFOUND_STRING, OSCONFIG_VERSION);     \
        if (NULL != telemetry_json)                                                                                                                    \
        {                                                                                                                                              \
            TelemetryAppendJson(telemetry_json);                                                                                                       \
            FREE_MEMORY(telemetry_json);                                                                                                               \
        }                                                                                                                                              \
    }

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds)                                                                              \
    {                                                                                                                                                  \
        char* telemetry_json = NULL;                                                                                                                   \
        const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);                                                                   \
        const char* timestamp = GetFormattedTime();                                                                                                    \
        const char* distroName = GetCachedDistroName();                                                                                                \
        telemetry_json = FormatAllocateString(                                                                                                         \
            "{"                                                                                                                                        \
            "\"EventName\":\"BaselineRun\","                                                                                                           \
            "\"Timestamp\":\"%s\","                                                                                                                    \
            "\"BaselineName\":\"%s\","                                                                                                                 \
            "\"Mode\":\"%s\","                                                                                                                         \
            "\"DurationSeconds\":\"%.2f\","                                                                                                            \
            "\"DistroName\":\"%s\","                                                                                                                   \
            "\"CorrelationId\":\"%s\","                                                                                                                \
            "\"Version\":\"%s\""                                                                                                                       \
            "}",                                                                                                                                       \
            timestamp ? timestamp : TELEMETRY_NOTFOUND_STRING, (baselineName) ? (baselineName) : TELEMETRY_NOTFOUND_STRING,                            \
            (mode) ? (mode) : TELEMETRY_NOTFOUND_STRING, durationSeconds, distroName ? distroName : TELEMETRY_NOTFOUND_STRING,                         \
            correlationId ? correlationId : TELEMETRY_NOTFOUND_STRING, OSCONFIG_VERSION);                                                              \
        if (NULL != telemetry_json)                                                                                                                    \
        {                                                                                                                                              \
            TelemetryAppendJson(telemetry_json);                                                                                                       \
            FREE_MEMORY(telemetry_json);                                                                                                               \
        }                                                                                                                                              \
    }

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds)                                                           \
    {                                                                                                                                                  \
        char* telemetry_json = NULL;                                                                                                                   \
        const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);                                                                   \
        const char* timestamp = GetFormattedTime();                                                                                                    \
        const char* distroName = GetCachedDistroName();                                                                                                \
        telemetry_json = FormatAllocateString(                                                                                                         \
            "{"                                                                                                                                        \
            "\"EventName\":\"RuleComplete\","                                                                                                          \
            "\"Timestamp\":\"%s\","                                                                                                                    \
            "\"ComponentName\":\"%s\","                                                                                                                \
            "\"ObjectName\":\"%s\","                                                                                                                   \
            "\"ObjectResult\":\"%d\","                                                                                                                 \
            "\"Microseconds\":\"%ld\","                                                                                                                \
            "\"DistroName\":\"%s\","                                                                                                                   \
            "\"CorrelationId\":\"%s\","                                                                                                                \
            "\"Version\":\"%s\""                                                                                                                       \
            "}",                                                                                                                                       \
            timestamp ? timestamp : TELEMETRY_NOTFOUND_STRING, (componentName) ? (componentName) : TELEMETRY_NOTFOUND_STRING,                          \
            (objectName) ? (objectName) : TELEMETRY_NOTFOUND_STRING, objectResult, microseconds, distroName ? distroName : TELEMETRY_NOTFOUND_STRING,  \
            correlationId ? correlationId : TELEMETRY_NOTFOUND_STRING, OSCONFIG_VERSION);                                                              \
        if (NULL != telemetry_json)                                                                                                                    \
        {                                                                                                                                              \
            TelemetryAppendJson(telemetry_json);                                                                                                       \
            FREE_MEMORY(telemetry_json);                                                                                                               \
        }                                                                                                                                              \
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
#define OSConfigGetElapsedTime(elapsed_us_var)                                                                                                         \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        (elapsed_us_var) = 0;                                                                                                                          \
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
