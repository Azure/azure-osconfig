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
#include <unistd.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"

// Helper function to generate random filename
// TODO: Incorporate into this header
// static char* generateRandomFilename(void)
// {
//     char temp_template[] = "/tmp/telemetry_XXXXXX";
//     int fd = mkstemp(temp_template);
//     if (fd == -1)
//     {
//         return NULL;
//     }

//     close(fd);
//     if (unlink(temp_template) != 0)
//     {
//         perror("Failed to delete temporary file");
//     }

//     // Append .json to the unique name
//     char* result = malloc(strlen(temp_template) + 6); // +5 for ".json" +1 for '\0'
//     if (result)
//     {
//         strcpy(result, temp_template);
//         strcat(result, ".json");
//     }

//     return result;
// }

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

// Helper function to run telemetry proxy
static void runTelemetryProxy(const char* telemetryJSONFile, const char* binaryDirectory)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        if (!binaryDirectory)
        {
            exit(ENOENT);
        }

        // Create new session and detach from parent's process group
        setsid();

        // Execute telemetry application
        // Usage: telemetry [OPTIONS] <json_file_path> [teardown_time_seconds]
        size_t pathLen = strlen(binaryDirectory) + strlen(TELEMETRY_BINARY_NAME) + 2;
        char* path = (char*)malloc(pathLen);
        if (path)
        {
            snprintf(path, pathLen, "%s/%s", binaryDirectory, TELEMETRY_BINARY_NAME);
            execl(path, TELEMETRY_BINARY_NAME, "-v", telemetryJSONFile, "5", (char*)NULL);
            free(path);
        }
        exit(errno);
    }
    // Parent process continues immediately without waiting
}

// Buffer sizes for string conversion of numeric values
// Based on maximum possible digits for each type plus sign and null terminator
#define MAX_INT_STRING_LENGTH 16    // Accommodates 32-bit int values
#define MAX_LONG_STRING_LENGTH 32   // Accommodates 64-bit long values

// Static file handle for telemetry logging with thread safety
static FILE* g_telemetryFile = NULL;
static int g_telemetryFileInitialized = 0;

// Initialize telemetry file - call once at program start (thread-safe)
#define OSConfigTelemetryInit() do { \
    if (!g_telemetryFileInitialized) { \
        static int initLock = 0; \
        if (__sync_bool_compare_and_swap(&initLock, 0, 1)) { \
            if (!g_telemetryFile) { \
                g_telemetryFile = fopen("/tmp/osconfig_telemetry.json", "a"); \
                if (g_telemetryFile) { \
                    atexit(OSConfigTelemetryCleanup); \
                    g_telemetryFileInitialized = 1; \
                } \
            } \
        } else { \
            while (!g_telemetryFileInitialized) { \
                usleep(1000); \
            } \
        } \
    } \
} while(0)

// Cleanup telemetry file - automatically called at program exit
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void OSConfigTelemetryCleanup(void) {
    if (g_telemetryFile) {
        fclose(g_telemetryFile);
        runTelemetryProxy("/tmp/osconfig_telemetry.json", getModuleDirectory());
        g_telemetryFile = NULL;
        g_telemetryFileInitialized = 0;
    }
}
#pragma GCC diagnostic pop

// Helper macro to append raw JSON string to telemetry file
#define OSConfigTelemetryAppendJSON(jsonString) do { \
    if (!g_telemetryFileInitialized) { \
        OSConfigTelemetryInit(); \
    } \
    if (g_telemetryFile) { \
        fprintf(g_telemetryFile, "%s\n", (jsonString)); \
        fflush(g_telemetryFile); \
    } \
} while(0)

#define OSConfigTelemetryStatusTrace(callingFunctionName, status) do { \
    char status_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(status_str, sizeof(status_str), "%d", (status)); \
    char line_str[MAX_INT_STRING_LENGTH] = {0}; \
    snprintf(line_str, sizeof(line_str), "%d", __LINE__); \
    const char* distroName = GetOsName(NULL); \
    const char* correlationId = getenv("activityId"); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"StatusTrace\",\"Filename\":\"%s\",\"LineNumber\":\"%s\",\"FunctionName\":\"%s\",\"CallingFunctionName\":\"%s\",\"ResultCode\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        __FILE__, line_str, __func__, \
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
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"BaselineRun\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"DurationSeconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        (baselineName) ? (baselineName) : "N/A", \
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
    const char* correlationId = getenv("activityId"); \
    char jsonBuffer[2048]; \
    snprintf(jsonBuffer, sizeof(jsonBuffer), \
        "{\"EventType\":\"RuleComplete\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s\",\"Microseconds\":\"%s\",\"DistroName\":\"%s\",\"CorrelationId\":\"%s\",\"Version\":\"%s\"}", \
        (componentName) ? (componentName) : "N/A", \
        (objectName) ? (objectName) : "N/A", \
        objectResult_str, \
        microseconds_str, \
        g_prettyName ? g_prettyName : "unknown", \
        correlationId ? correlationId : "", \
        OSCONFIG_VERSION); \
    OSConfigTelemetryAppendJSON(jsonBuffer); \
} while(0)

#endif // TELEMETRY_H
