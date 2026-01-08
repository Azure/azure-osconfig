// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.h"

#ifdef BUILD_TELEMETRY

#include "CommonUtils.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE* g_telemetryFile = NULL;
static char* g_moduleDirectory = NULL;
static OsConfigLogHandle g_telemetryLog = NULL;
static char* g_cachedOsName = NULL;

static char* ResolveModuleDirectory(void)
{
    Dl_info dlInfo = {0};
    char* directory = NULL;

    if (0 != dladdr((void*)&ResolveModuleDirectory, &dlInfo))
    {
        if (NULL != dlInfo.dli_fname)
        {
            const char* modulePath = dlInfo.dli_fname;
            const char* lastSlash = strrchr(modulePath, '/');

            if (NULL != lastSlash)
            {
                size_t length = (size_t)(lastSlash - modulePath);
                directory = (char*)malloc(length + 1);

                if (NULL != directory)
                {
                    memcpy(directory, modulePath, length);
                    directory[length] = '\0';
                }
            }
        }
    }

    return directory;
}

const char* OSConfigTelemetryGetModuleDirectory(void)
{
    return g_moduleDirectory;
}

void OSConfigTelemetryInit(void)
{
    if (NULL != g_telemetryFile)
    {
        return;
    }

    if (NULL == g_moduleDirectory)
    {
        g_moduleDirectory = ResolveModuleDirectory();
    }

    if (NULL == g_moduleDirectory)
    {
        OsConfigLogError(g_telemetryLog, "Failed to resolve module directory for telemetry");
        return;
    }

    g_telemetryFile = fopen(TELEMETRY_TMP_FILE_NAME, "a");

    if (NULL != g_telemetryFile)
    {
        OsConfigLogInfo(g_telemetryLog, "Telemetry json used: %s", TELEMETRY_TMP_FILE_NAME);
    }
}

void OSConfigTelemetryCleanup(void)
{
    if (NULL != g_telemetryFile)
    {
        fclose(g_telemetryFile);
        g_telemetryFile = NULL;
    }

    FREE_MEMORY(g_moduleDirectory);
    FREE_MEMORY(g_cachedOsName);
}

void OSConfigTelemetrySetLogger(const OsConfigLogHandle log)
{
    g_telemetryLog = log;
}

void OSConfigTelemetryAppendJSON(const char* jsonString)
{
    if (NULL == jsonString)
    {
        return;
    }

    if (NULL == g_telemetryFile)
    {
        OSConfigTelemetryInit();
    }

    if (NULL != g_telemetryFile)
    {
        fprintf(g_telemetryFile, "%s\n", jsonString);
        fflush(g_telemetryFile);
    }
}

const char* OSConfigTelemetryGetCachedOsName(void)
{
    if (NULL == g_cachedOsName)
    {
        g_cachedOsName = GetOsPrettyName(g_telemetryLog);
    }
    return g_cachedOsName;
}

#endif // BUILD_TELEMETRY
