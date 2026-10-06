// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.h"

#ifdef BUILD_TELEMETRY

#include "CommonUtils.h"
#include "TelemetryWorker.h"
#include "TelemetryDeadline.h"
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <version.h>

#define TELEMETRY_WORK_BUDGET_MS 500
#define TELEMETRY_OPERATION_MS 500
#define TELEMETRY_LIFETIME_MS 600000

static TelemetryWorker* g_worker = NULL;
static OsConfigLogHandle g_log = NULL;
static char* g_distroName = NULL;
static bool g_initialized = false;

char* GetModuleDirectory(void)
{
    Dl_info info = {0};
    char* path = NULL;
    char* slash = NULL;

    if ((dladdr((void*)&GetModuleDirectory, &info)) && (info.dli_fname))
    {
        path = realpath(info.dli_fname, NULL);

        if (path)
        {
            slash = strrchr(path, '/');

            if (!slash)
            {
                free(path);
                path = NULL;
            }
            else
            {
                // Preserve "/" for a module directly under the root directory.
                slash[slash == path ? 1 : 0] = '\0';
            }
        }
    }

    return path;
}

char* GetCachedDistroName(void)
{
    return g_distroName;
}

int TelemetryInitializeInternal(bool forceMinTls, OsConfigLogHandle log)
{
    int64_t start = 0;
    int64_t now = 0;
    int status = 0;
    char* directory = NULL;
    char* path = NULL;
    int64_t elapsedMs = 0;

    if (g_initialized)
    {
        OsConfigLogInfo(log, "TelemetryInitialize: Invocation already initialized; budget unchanged");
        status = EALREADY;
    }
    else
    {
        g_initialized = true;
        g_log = log;

        if (0 == (status = TelemetryMonotonicTime(&start)))
        {
            if (NULL == (directory = GetModuleDirectory()))
            {
                status = errno ? errno : ENOENT;
            }
            else if (NULL == (path = FormatAllocateString("%s/%s", directory, TELEMETRY_BINARY_NAME)))
            {
                status = ENOMEM;
            }
            else
            {
                status = SetFileAccess(path, 0, 0, 0700, log);
            }
        }

        if (0 == status)
        {
            g_distroName = GetOsPrettyName(log);
            status = TelemetryMonotonicTime(&now);
        }

        if (0 == status)
        {
            elapsedMs = (now - start + 999999) / 1000000;

            if (elapsedMs >= TELEMETRY_WORK_BUDGET_MS)
            {
                status = ETIMEDOUT;
            }
            else
            {
                status = TelemetryWorkerCreate(path, TELEMETRY_LIFETIME_MS,
                    TELEMETRY_WORK_BUDGET_MS - (int)elapsedMs, TELEMETRY_OPERATION_MS, &g_worker, forceMinTls, log);
            }
        }

        if (status)
        {
            OsConfigLogError(log, "TelemetryInitialize: Disabled for this invocation (status=%d)", status);
        }
    }

    free(directory);
    free(path);

    return status;
}

void TelemetryInitialize(const OsConfigLogHandle log)
{
    TelemetryInitializeInternal(false, log);
}

void TelemetryCleanup(OsConfigLogHandle log)
{
    int status = 0;

    if (0 != (status = TelemetryWorkerDestroy(&g_worker, log)))
    {
        OsConfigLogError(log, "TelemetryCleanup: Worker cleanup failed (status=%d)", status);
    }

    // Retain ownership if the exact child could not be reaped.
    if (!g_worker)
    {
        free(g_distroName);
        g_distroName = NULL;
        g_log = NULL;
        g_initialized = false;
    }
}

static const char* Value(const char* value)
{
    return value ? value : TELEMETRY_NOTFOUND_STRING;
}

static bool BeginEvent(int64_t* started)
{
    int status = 0;
    bool ready = false;

    // Some library callers emit outside a telemetry invocation. Never use a
    // missing log handle or implicitly start a worker from those call sites.
    if ((g_initialized) && (g_worker))
    {
        if (0 != (status = TelemetryMonotonicTime(started)))
        {
            OsConfigLogError(g_log, "Telemetry: Cannot time event preparation (status=%d)", status);
        }
        else
        {
            ready = true;
        }
    }

    return ready;
}

static void Submit(const char* name, const char* const* names, const char* const* values,
    size_t count, int64_t started)
{
    const char* timestamp = GetFormattedTime();
    const char* commonNames[] = {"DistroName", "CorrelationId", "Version", "Timestamp"};
    const char* commonValues[] = {g_distroName, getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR),
        OSCONFIG_VERSION, timestamp};
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    size_t i = 0;

    for (i = 0; i < ARRAY_SIZE(commonNames) + count; ++i)
    {
        properties[i].name = i < ARRAY_SIZE(commonNames) ? commonNames[i] : names[i - ARRAY_SIZE(commonNames)];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = Value(i < ARRAY_SIZE(commonNames) ?
            commonValues[i] : values[i - ARRAY_SIZE(commonNames)]);
    }

    if (0 == TelemetryWorkerAccountPreparation(g_worker, started, g_log))
    {
        TelemetryWorkerSend(g_worker, name, properties, ARRAY_SIZE(commonNames) + count, g_log);
    }
}

void OSConfigTimeStampSave(void)
{
    struct timespec now = {0};
    char value[32] = {0};

    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        if (g_initialized)
        {
            OsConfigLogError(g_log, "Telemetry: Cannot read rule start clock (status=%d)", errno);
        }
    }
    else
    {
        snprintf(value, sizeof(value), "%" PRId64, TsToUs(now));

        if ((setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR, value, 1)) && (g_initialized))
        {
            OsConfigLogError(g_log, "Telemetry: Cannot save rule start clock (status=%d)", errno);
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

    if (!microseconds)
    {
        return;
    }

    *microseconds = 0;
    value = getenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR);

    if (value)
    {
        errno = 0;
        start = strtoll(value, &end, 10);

        if ((errno) || (end == value) || (*end) || (start <= 0))
        {
            if (g_initialized)
            {
                OsConfigLogError(g_log, "Telemetry: Invalid rule start clock");
            }
        }
        else if (clock_gettime(CLOCK_MONOTONIC, &now))
        {
            if (g_initialized)
            {
                OsConfigLogError(g_log, "Telemetry: Cannot read rule completion clock (status=%d)", errno);
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
                OsConfigLogError(g_log, "Telemetry: Rule start is in the future");
            }
        }
    }
}

void TelemetryStatusTrace(const char* callingFunction, int status, const char* file,
    const char* function, int line)
{
    int64_t started = 0;
    int64_t elapsed = 0;
    char lineText[32] = {0};
    char result[32] = {0};
    char duration[32] = {0};
    const char* names[] = {"FileName", "LineNumber", "ScenarioName", "FunctionName", "RuleCodename",
        "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    const char* values[ARRAY_SIZE(names)] = {0};

    if (BeginEvent(&started))
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
        Submit("StatusTrace", names, values, ARRAY_SIZE(names), started);
    }
}

void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds)
{
    int64_t started = 0;
    char duration[384] = {0};
    const char* names[] = {"BaselineName", "Mode", "DurationSeconds"};
    const char* values[] = {baseline, mode, duration};

    if (BeginEvent(&started))
    {
        snprintf(duration, sizeof(duration), "%.2f", seconds);
        Submit("BaselineRun", names, values, ARRAY_SIZE(names), started);
    }
}

void OSConfigTelemetryRuleComplete(const char* component, const char* object, int result, int64_t microseconds)
{
    int64_t started = 0;
    char resultText[32] = {0};
    char duration[32] = {0};
    const char* names[] = {"ComponentName", "ObjectName", "ObjectResult", "Microseconds"};
    const char* values[] = {component, object, resultText, duration};

    if (BeginEvent(&started))
    {
        snprintf(resultText, sizeof(resultText), "%d", result);
        snprintf(duration, sizeof(duration), "%" PRId64, microseconds);
        Submit("RuleComplete", names, values, ARRAY_SIZE(names), started);
    }
}

void OSConfigTelemetryCrashDetected(const char* crashInfo)
{
    int64_t started = 0;
    const char* names[] = {"CrashInfo"};
    const char* values[] = {crashInfo};

    if (BeginEvent(&started))
    {
        Submit("CrashDetected", names, values, ARRAY_SIZE(names), started);
    }
}

#endif // BUILD_TELEMETRY
