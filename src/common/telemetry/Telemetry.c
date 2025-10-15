// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.h"

#include "CommonUtils.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE* g_telemetryFile = NULL;
static char* g_fileName = NULL;
static char* g_moduleDirectory = NULL;
static int g_telemetryFileInitialized = 0;

static char* GenerateRandomFilename(void)
{
    char tempTemplate[] = "/tmp/telemetry_XXXXXX";
    const char* suffix = ".json";
    char* result = NULL;
    int fd = -1;

    if (-1 != (fd = mkstemp(tempTemplate)))
    {
        close(fd);

        if (0 == unlink(tempTemplate))
        {
            size_t baseLength = strlen(tempTemplate);
            size_t suffixLength = strlen(suffix);
            result = (char*)malloc(baseLength + suffixLength + 1);

            if (NULL != result)
            {
                memcpy(result, tempTemplate, baseLength);
                memcpy(result + baseLength, suffix, suffixLength + 1);
            }
        }
    }

    return result;
}

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

FILE* OSConfigTelemetryGetFile(void)
{
    return g_telemetryFile;
}

const char* OSConfigTelemetryGetFileName(void)
{
    return g_fileName;
}

const char* OSConfigTelemetryGetModuleDirectory(void)
{
    return g_moduleDirectory;
}

int OSConfigTelemetryIsInitialized(void)
{
    return g_telemetryFileInitialized;
}

void OSConfigTelemetryInit(void)
{
    if (g_telemetryFileInitialized || (NULL != g_telemetryFile))
    {
        return;
    }

    if (NULL == g_moduleDirectory)
    {
        g_moduleDirectory = ResolveModuleDirectory();
    }

    if (NULL == g_fileName)
    {
        g_fileName = GenerateRandomFilename();
    }

    if (NULL == g_fileName)
    {
        return;
    }

    g_telemetryFile = fopen(g_fileName, "a");

    if (NULL != g_telemetryFile)
    {
        g_telemetryFileInitialized = 1;
    }
}

void OSConfigTelemetryCleanup(void)
{
    if (NULL != g_telemetryFile)
    {
        fclose(g_telemetryFile);
        g_telemetryFile = NULL;
    }

    g_telemetryFileInitialized = 0;

    FREE_MEMORY(g_fileName);
    FREE_MEMORY(g_moduleDirectory);
}

void OSConfigTelemetryAppendJSON(const char* jsonString)
{
    if (NULL == jsonString)
    {
        return;
    }

    if (!g_telemetryFileInitialized)
    {
        OSConfigTelemetryInit();
    }

    if (NULL != g_telemetryFile)
    {
        fprintf(g_telemetryFile, "%s\n", jsonString);
        fflush(g_telemetryFile);
    }
}
