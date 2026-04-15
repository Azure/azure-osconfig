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
#include <sys/stat.h>
#include <unistd.h>

static FILE* g_telemetryFile = NULL;
static char* g_executableDirectory = NULL;
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

#ifndef API_KEY
#define API_KEY "@OsConfigTelemetryApiKey@"
#endif

void TelemetryInitialize(const OsConfigLogHandle log)
{
    if (false == DirectoryExists(OSCONFIG_DIRECTORY_NAME))
    {
        if (0 != mkdir(OSCONFIG_DIRECTORY_NAME, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
        {
            OsConfigLogError(log, "TelemetryInitialize: Failed to create directory: '%s' (%d, %s)", OSCONFIG_DIRECTORY_NAME, errno, strerror(errno));
            return;
        }
        else
        {
            OsConfigLogInfo(log, "TelemetryInitialize: Created directory: %s", OSCONFIG_DIRECTORY_NAME);
        }
    }

    if (false == DirectoryExists(TELEMETRY_DIRECTORY_NAME))
    {
        if (0 != mkdir(TELEMETRY_DIRECTORY_NAME, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
        {
            OsConfigLogError(log, "TelemetryInitialize: Failed to create directory: '%s' (%d, %s)", TELEMETRY_DIRECTORY_NAME, errno, strerror(errno));
            return;
        }
        else
        {
            OsConfigLogInfo(log, "TelemetryInitialize: Created directory: %s", TELEMETRY_DIRECTORY_NAME);
        }
    }

    g_telemetryFile = fopen(TELEMETRY_TMP_FILE_NAME, "a");

    if (NULL != g_telemetryFile)
    {
        OsConfigLogInfo(log, "TelemetryInitialize: Opened file: %s", TELEMETRY_TMP_FILE_NAME);

        g_executableDirectory = GetModuleDirectory();

        if (NULL != g_executableDirectory)
        {
            OsConfigLogInfo(log, "TelemetryInitialize: Found module directory: %s", g_executableDirectory);
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

    OsConfigLogInfo(log, "TelemetryInitialize: on '%s' and using API_KEY '%s'", g_distroName, API_KEY);
}

void TelemetryCleanup(const OsConfigLogHandle log)
{
    int status = 0;
    char* fileName = NULL;
    char* command = NULL;

    OsConfigLogInfo(log, "TelemetryCleanup: using API_KEY '%s'", API_KEY);

    if (NULL != g_telemetryFile)
    {
        if (NULL != g_executableDirectory)
        {
            fileName = FormatAllocateString("%s/%s", g_executableDirectory, TELEMETRY_BINARY_NAME);

            if (false == FileExists(fileName))
            {
                OsConfigLogError(log, "TelemetryCleanup: '%s' does not exist, cannot invoke executable  to send telemetry", fileName);
            }
            else
            {
                SetFileAccess(fileName, 0, 0, 0700, log);
                command = FormatAllocateString("%s -f %s -t %d %s", fileName, TELEMETRY_TMP_FILE_NAME, TELEMETRY_TEARDOWN_TIMEOUT_SECONDS, VERBOSE_FLAG_IF_DEBUG);
                if (0 != (status = ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_COMMAND_TIMEOUT_SECONDS, NULL, NULL, log)))
                {
                    OsConfigLogError(log, "TelemetryCleanup: '%s' failed with %d (%s)", command, status, strerror(status));
                }
            }
        }

        fclose(g_telemetryFile);
        g_telemetryFile = NULL;
    }
    else
    {
        OsConfigLogError(log, "TelemetryCleanup: no telemetry file, nothing to send");
    }

    FREE_MEMORY(command);
    FREE_MEMORY(fileName);

    FREE_MEMORY(g_executableDirectory);
    FREE_MEMORY(g_distroName);
}

void TelemetryAppendPayloadToFile(const char* jsonString)
{
    if ((NULL != jsonString) && (NULL != g_telemetryFile))
    {
        fprintf(g_telemetryFile, "%s\n", jsonString);
        fflush(g_telemetryFile);
    }
}

#endif // BUILD_TELEMETRY
