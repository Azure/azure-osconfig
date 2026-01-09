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

static FILE* g_tmpFile = NULL;
static char* g_moduleDirectory = NULL;
static char* g_distroName = NULL;

char* GetModuleDirectory(void)
{
    Dl_info dlInfo = {0};
    char* directory = NULL;

    if (0 != dladdr((void*)&GetModuleDirectory, &dlInfo))
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

char* GetCachedDistroName(void)
{
    return g_distroName;
}

void TelemetryInitialize(const OsConfigLogHandle log)
{
    g_tmpFile = fopen(TELEMETRY_TMP_FILE_NAME, "a");

    if (NULL != g_tmpFile)
    {
        OsConfigLogInfo(log, "TelemetryInitialize: Opened file: %s", TELEMETRY_TMP_FILE_NAME);

        g_moduleDirectory = GetModuleDirectory();

        if (NULL != g_moduleDirectory)
        {
            OsConfigLogInfo(log, "TelemetryInitialize: Found module directory: %s", g_moduleDirectory);
        }
        else
        {
            OsConfigLogError(log, "TelemetryInitialize: Failed to resolve module directory");
        }
    }
    else
    {
        OsConfigLogError(log, "TelemetryInitialize: Failed to open file %s", TELEMETRY_TMP_FILE_NAME);
    }

    g_distroName = GetOsPrettyName(log);
}

void TelemetryCleanup(const OsConfigLogHandle log)
{
    if (NULL != g_tmpFile)
    {
        char* fileName = NULL;
        char* command = NULL;

        if (NULL != g_moduleDirectory)
        {
            fileName = FormatAllocateString("%s/%s", g_moduleDirectory, TELEMETRY_BINARY_NAME);

            if (NULL != fileName)
            {
                if (0 == SetFileAccess(fileName, 0, 0, 0700, log))
                {
                    command = FormatAllocateString("%s -f %s -t %d %s", fileName, TELEMETRY_TMP_FILE_NAME, TELEMETRY_COMMAND_TIMEOUT_SECONDS, VERBOSE_FLAG_IF_DEBUG);

                    if (NULL != command)
                    {
                        ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_COMMAND_TIMEOUT_SECONDS, NULL, NULL, log);
                        FREE_MEMORY(command);
                    }
                }

                FREE_MEMORY(fileName);
            }

            FREE_MEMORY(g_moduleDirectory);
        }

        fclose(g_tmpFile);
        g_tmpFile = NULL;
    }

    FREE_MEMORY(g_distroName);
}

void TelemetryAppendJson(const char* jsonString)
{
    if ((NULL != jsonString) && (NULL != g_tmpFile))
    {
        fprintf(g_tmpFile, "%s\n", jsonString);
        fflush(g_tmpFile);
    }
}

#endif // BUILD_TELEMETRY
