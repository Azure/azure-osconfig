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
    if (!dladdr((void*)&GetModuleDirectory, &info) || !info.dli_fname) return NULL;
    char* path = realpath(info.dli_fname, NULL);
    if (!path) return NULL;
    char* slash = strrchr(path, '/');
    if (!slash) { free(path); return NULL; }
    // Preserve "/" for a module directly under the root directory.
    slash[slash == path ? 1 : 0] = '\0';
    return path;
}

char* GetCachedDistroName(void)
{
    return g_distroName;
}

int TelemetryInitialize(OsConfigLogHandle log)
{
    if (g_initialized)
    {
        OsConfigLogInfo(log, "TelemetryInitialize: Invocation already initialized; budget unchanged");
        return EALREADY;
    }
    g_initialized = true;
    g_log = log;
    int64_t start = 0, now = 0;
    int status = TelemetryMonotonicTime(&start);
    char* directory = NULL;
    char* path = NULL;
    if (status) goto finished;
    directory = GetModuleDirectory();
    if (!directory) { status = errno ? errno : ENOENT; goto finished; }
    path = FormatAllocateString("%s/%s", directory, TELEMETRY_BINARY_NAME);
    if (!path) { status = ENOMEM; goto finished; }
    status = SetFileAccess(path, 0, 0, 0700, log);
    if (status) goto finished;
    g_distroName = GetOsPrettyName(log);
    status = TelemetryMonotonicTime(&now);
    if (status) goto finished;
    int64_t elapsedMs = (now - start + 999999) / 1000000;
    if (elapsedMs >= TELEMETRY_WORK_BUDGET_MS) { status = ETIMEDOUT; goto finished; }
    status = TelemetryWorkerCreate(path, TELEMETRY_LIFETIME_MS,
        TELEMETRY_WORK_BUDGET_MS - (int)elapsedMs, TELEMETRY_OPERATION_MS, &g_worker, log);
finished:
    free(directory);
    free(path);
    if (status)
        OsConfigLogError(log, "TelemetryInitialize: Disabled for this invocation (status=%d)", status);
    return status;
}

void TelemetryCleanup(OsConfigLogHandle log)
{
    int status = TelemetryWorkerDestroy(&g_worker, log);
    if (status) OsConfigLogError(log, "TelemetryCleanup: Worker cleanup failed (status=%d)", status);
    if (g_worker) return; // Retain ownership if the exact child could not be reaped.
    free(g_distroName);
    g_distroName = NULL;
    g_log = NULL;
    g_initialized = false;
}

static const char* Value(const char* value)
{
    return value ? value : TELEMETRY_NOTFOUND_STRING;
}

static bool BeginEvent(int64_t* started)
{
    // Some library callers emit outside a telemetry invocation. Never use a
    // missing log handle or implicitly start a worker from those call sites.
    if (!g_initialized || !g_worker) return false;
    int status = TelemetryMonotonicTime(started);
    if (status)
    {
        OsConfigLogError(g_log, "Telemetry: Cannot time event preparation (status=%d)", status);
        return false;
    }
    return true;
}

static void Submit(const char* name, const char* const* names, const char* const* values,
    size_t count, int64_t started)
{
    const char* timestamp = GetFormattedTime();
    const char* commonNames[] = {"DistroName", "CorrelationId", "Version", "Timestamp"};
    const char* commonValues[] = {g_distroName, getenv(TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR),
        OSCONFIG_VERSION, timestamp};
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    for (size_t i = 0; i < ARRAY_SIZE(commonNames) + count; ++i)
    {
        properties[i].name = i < ARRAY_SIZE(commonNames) ? commonNames[i] : names[i - ARRAY_SIZE(commonNames)];
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = Value(i < ARRAY_SIZE(commonNames) ?
            commonValues[i] : values[i - ARRAY_SIZE(commonNames)]);
    }
    if (0 == TelemetryWorkerAccountPreparation(g_worker, started, g_log))
        TelemetryWorkerSend(g_worker, name, properties, ARRAY_SIZE(commonNames) + count, g_log);
}

void OSConfigTimeStampSave(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        if (g_initialized) OsConfigLogError(g_log, "Telemetry: Cannot read rule start clock (status=%d)", errno);
        return;
    }
    char value[32];
    snprintf(value, sizeof(value), "%" PRId64, TsToUs(now));
    if (setenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR, value, 1) && g_initialized)
        OsConfigLogError(g_log, "Telemetry: Cannot save rule start clock (status=%d)", errno);
}

void OSConfigGetElapsedTime(int64_t* microseconds)
{
    if (!microseconds) return;
    *microseconds = 0;
    const char* value = getenv(TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR);
    if (!value) return;
    char* end = NULL;
    errno = 0;
    int64_t start = strtoll(value, &end, 10);
    if (errno || end == value || *end || start <= 0)
    {
        if (g_initialized) OsConfigLogError(g_log, "Telemetry: Invalid rule start clock");
        return;
    }
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        if (g_initialized) OsConfigLogError(g_log, "Telemetry: Cannot read rule completion clock (status=%d)", errno);
        return;
    }
    int64_t elapsed = TsToUs(now) - start;
    if (elapsed >= 0) *microseconds = elapsed;
    else if (g_initialized) OsConfigLogError(g_log, "Telemetry: Rule start is in the future");
}

void TelemetryStatusTrace(const char* callingFunction, int status, const char* file,
    const char* function, int line)
{
    int64_t started = 0;
    if (!BeginEvent(&started)) return;
    int64_t elapsed = 0;
    OSConfigGetElapsedTime(&elapsed);
    char lineText[32], result[32], duration[32];
    snprintf(lineText, sizeof(lineText), "%d", line);
    snprintf(result, sizeof(result), "%d", status);
    snprintf(duration, sizeof(duration), "%" PRId64, elapsed);
    const char* names[] = {"FileName", "LineNumber", "ScenarioName", "FunctionName", "RuleCodename",
        "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    const char* values[] = {file, lineText, getenv(TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR), function,
        getenv(TELEMETRY_RULECODENAME_ENVIRONMENT_VAR), callingFunction, duration, result, strerror(status)};
    Submit("StatusTrace", names, values, ARRAY_SIZE(names), started);
}

void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds)
{
    int64_t started = 0;
    if (!BeginEvent(&started)) return;
    char duration[384];
    snprintf(duration, sizeof(duration), "%.2f", seconds);
    const char* names[] = {"BaselineName", "Mode", "DurationSeconds"};
    const char* values[] = {baseline, mode, duration};
    Submit("BaselineRun", names, values, ARRAY_SIZE(names), started);
}

void OSConfigTelemetryRuleComplete(const char* component, const char* object, int result, int64_t microseconds)
{
    int64_t started = 0;
    if (!BeginEvent(&started)) return;
    char resultText[32], duration[32];
    snprintf(resultText, sizeof(resultText), "%d", result);
    snprintf(duration, sizeof(duration), "%" PRId64, microseconds);
    const char* names[] = {"ComponentName", "ObjectName", "ObjectResult", "Microseconds"};
    const char* values[] = {component, object, resultText, duration};
    Submit("RuleComplete", names, values, ARRAY_SIZE(names), started);
}

void OSConfigTelemetryCrashDetected(const char* crashInfo)
{
    int64_t started = 0;
    if (!BeginEvent(&started)) return;
    const char* names[] = {"CrashInfo"};
    const char* values[] = {crashInfo};
    Submit("CrashDetected", names, values, ARRAY_SIZE(names), started);
}

#endif // BUILD_TELEMETRY
