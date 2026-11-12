// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Telemetry.h>

#include <CommonUtils.h>
#include <IpcInterface.h>
#include <Utils.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <version.h>

#define TELEMETRY_CORRELATIONID_FILE "/var/lib/GuestConfig/Configuration/activityId.txt"

static TelemetryWorker* g_worker = NULL;
static OsConfigLogHandle g_log = NULL;
static char* g_distroName = NULL;
static bool g_initialized = false;

char* GetModuleDirectory(void)
{
    Dl_info info = {0};
    char* path = NULL;
    char* slash = NULL;

    if ((0 != dladdr((void*)&GetModuleDirectory, &info)) && (NULL != info.dli_fname))
    {
        path = realpath(info.dli_fname, NULL);

        if (NULL != path)
        {
            slash = strrchr(path, '/');

            if (NULL == slash)
            {
                free(path);
                path = NULL;
            }
            else
            {
                // Preserve "/" for a module directly under the root directory.
                slash[(slash == path) ? 1 : 0] = '\0';
            }
        }
    }

    return path;
}

char* GetCachedDistroName(void)
{
    return g_distroName;
}

int TelemetryInitializeInternal(OsConfigLogHandle log)
{
    int status = 0;
    char* directory = NULL;
    char* path = NULL;
    bool available = false;
    const char* operation = "GetModuleDirectory";

    if (g_initialized)
    {
        return status;
    }

    g_initialized = true;
    g_log = log;

    if (NULL == (directory = GetModuleDirectory()))
    {
        status = (0 != errno) ? errno : ENOENT;
    }
    else if (NULL == (path = FormatAllocateString("%s/%s", directory, TELEMETRY_BINARY_NAME)))
    {
        operation = "FormatAllocateString";
        status = ENOMEM;
    }
    else
    {
        operation = "access(OSConfigTelemetry)";
        if (0 == access(path, F_OK))
        {
            available = true;
            operation = "SetFileAccess";
            status = SetFileAccess(path, 0, 0, 0700, log);
        }
        else if ((ENOENT != errno) && (ENOTDIR != errno))
        {
            status = errno;
        }
        // An absent optional executable silently disables this invocation.
    }

    if ((0 == status) && available)
    {
        g_distroName = GetOsPrettyName(log);
        operation = "TelemetryWorkerCreate";
        status = TelemetryWorkerCreate(path, &g_worker, log);
    }

    if (0 != status)
    {
        OsConfigLogInfo(log, "TelemetryInitializeInternal: %s failed with %d (%s)", operation, status, strerror(status));
    }

    free(directory);
    free(path);

    return status;
}

void TelemetryInitialize(const OsConfigLogHandle log)
{
    TelemetryInitializeInternal(log);
}

void TelemetryCleanup(OsConfigLogHandle log)
{
    int status = 0;

    if (0 != (status = TelemetryWorkerDestroy(&g_worker, log)))
    {
        OsConfigLogInfo(log, "TelemetryCleanup: TelemetryWorkerDestroy failed with %d (%s)", status, strerror(status));
    }

    // Retain ownership if the exact child could not be reaped.
    if (NULL == g_worker)
    {
        free(g_distroName);
        g_distroName = NULL;
        g_log = NULL;
        g_initialized = false;
    }
}

static const char* Value(const char* value)
{
    return (NULL != value) ? value : TELEMETRY_NOTFOUND_STRING;
}

static bool BeginEvent(void)
{
    // Some library callers emit outside a telemetry invocation. Never use a
    // missing log handle or implicitly start a worker from those call sites.
    return g_initialized && (NULL != g_worker);
}

static const char* GetCorrelationId(char** fileContent)
{
    const char* correlationId = getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR);

    if ((NULL == correlationId) || ('\0' == correlationId[0]))
    {
        *fileContent = LoadStringFromFile(TELEMETRY_CORRELATIONID_FILE, false, g_log);
        correlationId = *fileContent;

        if ((NULL == correlationId) || ('\0' == correlationId[0]))
        {
            OsConfigLogInfo(g_log, "GetCorrelationId: environment variable '%s' is unset or empty and could not load a nonempty value from '%s'; using '%s'",
                TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR, TELEMETRY_CORRELATIONID_FILE, TELEMETRY_NOTFOUND_STRING);
            correlationId = TELEMETRY_NOTFOUND_STRING;
        }
    }

    return correlationId;
}

static void SubmitEvent(const char* name, const char* const* names, const char* const* values, size_t count)
{
    const char* timestamp = GetFormattedTime();
    char* correlationIdFromFile = NULL;
    const char* commonNames[] = {"DistroName", "CorrelationId", "Version", "Timestamp"};
    const char* commonValues[] = {g_distroName, GetCorrelationId(&correlationIdFromFile), OSCONFIG_VERSION, timestamp};
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    size_t i = 0;

    for (i = 0; i < (ARRAY_SIZE(commonNames) + count); ++i)
    {
        properties[i].name = (i < ARRAY_SIZE(commonNames)) ? commonNames[i] : names[i - ARRAY_SIZE(commonNames)];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = Value((i < ARRAY_SIZE(commonNames)) ? commonValues[i] : values[i - ARRAY_SIZE(commonNames)]);
    }

    TelemetryWorkerSendEvent(g_worker, name, properties, ARRAY_SIZE(commonNames) + count, g_log);

    free(correlationIdFromFile);
}

void OSConfigTimeStampSave(void)
{
    struct timespec now = {0};
    char value[32] = {0};
    int status = 0;

    if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
    {
        status = errno;
        if (g_initialized)
        {
            OsConfigLogInfo(g_log, "OSConfigTimeStampSave: clock_gettime(CLOCK_MONOTONIC) failed with %d (%s)", status, strerror(status));
        }
    }
    else
    {
        snprintf(value, sizeof(value), "%" PRId64, TsToUs(now));

        if ((0 != setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR, value, 1)) && g_initialized)
        {
            status = errno;
            OsConfigLogInfo(g_log, "OSConfigTimeStampSave: setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR) failed with %d (%s)", status, strerror(status));
        }
    }
}

void OSConfigGetElapsedTime(int64_t* microseconds)
{
    const char* value = NULL;
    char* end = NULL;
    int64_t start = 0;
    struct timespec now = {0};
    int64_t elapsed = 0;
    int status = 0;

    if (NULL == microseconds)
    {
        if (g_initialized)
        {
            OsConfigLogInfo(g_log, "OSConfigGetElapsedTime: output validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        }

        return;
    }

    *microseconds = 0;
    value = getenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR);

    if (NULL != value)
    {
        errno = 0;
        start = strtoll(value, &end, 10);

        if ((0 != errno) || (end == value) || ('\0' != *end) || (start <= 0))
        {
            status = (0 != errno) ? errno : EINVAL;
            if (g_initialized)
            {
                OsConfigLogInfo(g_log, "OSConfigGetElapsedTime: strtoll/start timestamp validation failed with %d (%s)", status, strerror(status));
            }
        }
        else if (0 != clock_gettime(CLOCK_MONOTONIC, &now))
        {
            status = errno;
            if (g_initialized)
            {
                OsConfigLogInfo(g_log, "OSConfigGetElapsedTime: clock_gettime(CLOCK_MONOTONIC) failed with %d (%s)", status, strerror(status));
            }
        }
        else
        {
            elapsed = TsToUs(now) - start;

            if (elapsed >= 0)
            {
                *microseconds = elapsed;
            }
            else if (g_initialized)
            {
                OsConfigLogInfo(g_log, "OSConfigGetElapsedTime: elapsed-time validation failed with %d (%s); start timestamp is in the future", EINVAL, strerror(EINVAL));
            }
        }
    }
}

void OSConfigTelemetryStatusTraceInternal(const char* callingFunction, int status, const char* file, const char* function, int line)
{
    int64_t elapsed = 0;
    char lineText[32] = {0};
    char result[32] = {0};
    char duration[32] = {0};
    const char* names[] = {"FileName", "LineNumber", "ScenarioName", "FunctionName", "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    const char* values[ARRAY_SIZE(names)] = {0};

    if (BeginEvent())
    {
        OSConfigGetElapsedTime(&elapsed);
        snprintf(lineText, sizeof(lineText), "%d", line);
        snprintf(result, sizeof(result), "%d", status);
        snprintf(duration, sizeof(duration), "%" PRId64, elapsed);
        values[0] = file;
        values[1] = lineText;
        values[2] = getenv(TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR);
        values[3] = function;
        values[4] = getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR);
        values[5] = callingFunction;
        values[6] = duration;
        values[7] = result;
        values[8] = strerror(status);
        SubmitEvent("StatusTrace", names, values, ARRAY_SIZE(names));
    }
}

void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds)
{
    char duration[384] = {0};
    const char* names[] = {"BaselineName", "Mode", "DurationSeconds"};
    const char* values[] = {baseline, mode, duration};

    if (BeginEvent())
    {
        snprintf(duration, sizeof(duration), "%.2f", seconds);
        SubmitEvent("BaselineRun", names, values, ARRAY_SIZE(names));
    }
}

void OSConfigTelemetryRuleComplete(const char* component, const char* object, int result, int64_t microseconds)
{
    char resultText[32] = {0};
    char duration[32] = {0};
    const char* names[] = {"ComponentName", "ObjectName", "ObjectResult", "Microseconds"};
    const char* values[] = {component, object, resultText, duration};

    if (BeginEvent())
    {
        snprintf(resultText, sizeof(resultText), "%d", result);
        snprintf(duration, sizeof(duration), "%" PRId64, microseconds);
        SubmitEvent("RuleComplete", names, values, ARRAY_SIZE(names));
    }
}

void OSConfigTelemetryCrashDetected(const char* crashInfo)
{
    const char* names[] = {"CrashInfo"};
    const char* values[] = {crashInfo};

    if (BeginEvent())
    {
        SubmitEvent("CrashDetected", names, values, ARRAY_SIZE(names));
    }
}
