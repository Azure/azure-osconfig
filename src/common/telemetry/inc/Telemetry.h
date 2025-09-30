// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

#include <Logging.h>
#include <CommonUtils.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"
#define TELEMETRY_TIMEOUT_SECONDS 10

// Helper function to generate random filename
static char* generateRandomFilename(void)
{
    char temp_template[] = "/tmp/telemetry_XXXXXX";
    int fd = mkstemp(temp_template);
    if (fd == -1)
    {
        return NULL;
    }

    close(fd);
    if (unlink(temp_template) != 0)
    {
        perror("Failed to delete temporary file");
    }

    // Append .json to the unique name
    char* result = (char*)malloc(strlen(temp_template) + 6); // +5 for ".json" +1 for '\0'
    if (result)
    {
        strcpy(result, temp_template);
        strcat(result, ".json");
    }

    return result;
}

// Helper function to get module directory
static char* getModuleDirectory(void)
{
    Dl_info dl_info;

    // Get information about the current function's address
    if (dladdr((void*)getModuleDirectory, &dl_info) != 0)
    {
        if (dl_info.dli_fname != NULL)
        {
            const char* fullPath = dl_info.dli_fname;

            // Find the last slash to get the directory path
            const char* lastSlash = strrchr(fullPath, '/');
            if (lastSlash != NULL)
            {
                size_t dirLen = lastSlash - fullPath;
                char* result = (char*)malloc(dirLen + 1);
                if (result)
                {
                    strncpy(result, fullPath, dirLen);
                    result[dirLen] = '\0';
                    return result;
                }
            }
        }
    }

    return NULL; // Return NULL on failure
}

// Buffer sizes for string conversion of numeric values
// Based on maximum possible digits for each type plus sign and null terminator
#define MAX_INT_STRING_LENGTH 16    // Accommodates 32-bit int values
#define MAX_LONG_STRING_LENGTH 32   // Accommodates 64-bit long values

// Static file handle for telemetry logging with thread safety
static FILE* g_telemetryFile = NULL;
static char* g_fileName = NULL;
static char* g_moduleDirectory = NULL;
static int g_telemetryFileInitialized = 0;

// Cleanup telemetry file - automatically called at program exit
static void OSConfigProcessTelemetryFile(void) __attribute__((unused));
static void OSConfigProcessTelemetryFile(void) {
    if (g_telemetryFile && g_fileName && g_moduleDirectory) {
        char* command = FormatAllocateString("%s/%s -v %s %d", g_moduleDirectory, TELEMETRY_BINARY_NAME, g_fileName, TELEMETRY_TIMEOUT_SECONDS);
        if (NULL != command)
        {
            ExecuteCommand(NULL, command, false, false, 0, TELEMETRY_TIMEOUT_SECONDS, NULL, NULL, NULL);
        }

        FREE_MEMORY(command);
    }

    g_telemetryFile = NULL;
    g_telemetryFileInitialized = 0;

    FREE_MEMORY(g_fileName);
    FREE_MEMORY(g_moduleDirectory);
}

// Initialize telemetry file - call once at program start (thread-safe)
static void OSConfigTelemetryInit(void) __attribute__((unused));
static void OSConfigTelemetryInit(void) {
    if (!g_telemetryFileInitialized) {
        static int initLock = 0;
        if (__sync_bool_compare_and_swap(&initLock, 0, 1)) {
            if (!g_telemetryFile) {
                g_moduleDirectory = getModuleDirectory();
                g_fileName = generateRandomFilename();
                g_telemetryFile = fopen(g_fileName, "a");
                if (g_telemetryFile) {
                    g_telemetryFileInitialized = 1;
                }
            }
        } else {
            while (!g_telemetryFileInitialized) {
                usleep(1000);
            }
        }
    }
}

// Helper function to append raw JSON string to telemetry file
static void OSConfigTelemetryAppendJSON(const char* jsonString) __attribute__((unused));
static void OSConfigTelemetryAppendJSON(const char* jsonString) {
    if (!g_telemetryFileInitialized) {
        OSConfigTelemetryInit();
    }
    if (g_telemetryFile) {
        fprintf(g_telemetryFile, "%s\n", jsonString);
        fflush(g_telemetryFile);
    }
}

#define OSConfigTelemetryStatusTrace(callingFunctionName, status) do { \
    char status_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(status_str, sizeof(status_str), "%d", (status)); \
    char line_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(line_str, sizeof(line_str), "%d", __LINE__); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    const char* timestamp = GetFormattedTime(); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventName\":\"StatusTrace\",\"Timestamp\":\"%s\",\"Filename\":\"%s\",\"LineNumber\":\"%s\",\"FunctionName\":\"%s\",\"CallingFunctionName\":\"%s\",\"ResultCode\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        timestamp ? timestamp : "", __FILE__, line_str, __func__, \
        (callingFunctionName) ? (callingFunctionName) : "-", \
        status_str, \
        distroName ? distroName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#define OSConfigTelemetryBaselineRun(baselineName, mode, durationSeconds) do { \
    char durationSeconds_str[MAX_LONG_STRING_LENGTH] = {0}; \
    snprintf(durationSeconds_str, sizeof(durationSeconds_str), "%.2f", (double)(durationSeconds)); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    const char* timestamp = GetFormattedTime(); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventName\":\"BaselineRun\",\"Timestamp\":\"%s\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"DurationSeconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        timestamp ? timestamp : "", (baselineName) ? (baselineName) : "N/A", \
        (mode) ? (mode) : "N/A", \
        durationSeconds_str, \
        distroName ? distroName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#define OSConfigTelemetryRuleComplete(componentName, objectName, objectResult, microseconds) do { \
    char objectResult_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(objectResult_str, sizeof(objectResult_str), "%d", (objectResult)); \
    char microseconds_str[MAX_LONG_STRING_LENGTH] = {0}; \
    snprintf(microseconds_str, sizeof(microseconds_str), "%ld", (long)(microseconds)); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    const char* timestamp = GetFormattedTime(); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventName\":\"RuleComplete\",\"Timestamp\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s\",\"Microseconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        timestamp ? timestamp : "", (componentName) ? (componentName) : "N/A", \
        (objectName) ? (objectName) : "N/A", \
        objectResult_str, \
        microseconds_str, \
        distroName ? distroName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#endif // TELEMETRY_H
