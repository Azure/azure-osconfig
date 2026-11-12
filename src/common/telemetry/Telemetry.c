// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Telemetry.h>

#include <CommonUtils.h>
#include <IpcInterface.h>
#include <Utils.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <version.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"
#define TELEMETRY_NOTFOUND_STRING "N/A"
#define TELEMETRY_CORRELATIONID_FILE "/var/lib/GuestConfig/Configuration/activityId.txt"

static TelemetryWorker* g_worker = NULL;
static OsConfigLogHandle g_log = NULL;
static char* g_distroName = NULL;
static bool g_initialized = false;

static char* GetModuleDirectory(void)
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

void OSConfigTelemetryCrashDetected(const char* crashInfo)
{
    const char* names[] = {"CrashInfo"};
    const char* values[] = {crashInfo};

    if (BeginEvent())
    {
        SubmitEvent("CrashDetected", names, values, ARRAY_SIZE(names));
    }
}
