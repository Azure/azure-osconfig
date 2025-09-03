// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <parson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <Logging.h>
#include "TelemetryJson.h"

#if BUILD_TELEMETRY

// Static instance of the telemetry logger
static struct TelemetryLogger* g_telemetryLogger = NULL;

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
    char* result = malloc(strlen(temp_template) + 6); // +5 for ".json" +1 for '\0'
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
                char* result = malloc(dirLen + 1);
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
        char* path = malloc(pathLen);
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

// C interface implementations
int OSConfigTelemetryOpen(OsConfigLogHandle log)
{
    // If already open, return success
    if (g_telemetryLogger != NULL && g_telemetryLogger->isOpen)
    {
        return 0;
    }

    // Clean up any existing instance
    if (g_telemetryLogger != NULL)
    {
        OSConfigTelemetryClose();
    }

    g_telemetryLogger = malloc(sizeof(struct TelemetryLogger));
    if (!g_telemetryLogger)
    {
        OsConfigLogError(log, "Failed to allocate memory for TelemetryLogger");
        return -1;
    }

    // Initialize the logger
    g_telemetryLogger->logFile = NULL;
    g_telemetryLogger->filename = NULL;
    g_telemetryLogger->binaryDirectory = NULL;
    g_telemetryLogger->isOpen = 0;
    g_telemetryLogger->log = log;

    // Generate filename
    g_telemetryLogger->filename = generateRandomFilename();
    if (!g_telemetryLogger->filename)
    {
        free(g_telemetryLogger);
        g_telemetryLogger = NULL;
        OsConfigLogError(log, "Failed to generate random filename for telemetry log");
        return -1;
    }

    // Open file
    g_telemetryLogger->logFile = fopen(g_telemetryLogger->filename, "a");
    if (!g_telemetryLogger->logFile)
    {
        free(g_telemetryLogger->filename);
        free(g_telemetryLogger);
        g_telemetryLogger = NULL;
        OsConfigLogError(log, "Failed to open telemetry log file: %s", strerror(errno));
        return -1;
    }

    g_telemetryLogger->isOpen = 1;
    return 0;
}

int OSConfigTelemetryClose(void)
{
    if (g_telemetryLogger == NULL)
    {
        return -1;
    }

    if (!g_telemetryLogger->isOpen)
    {
        OsConfigLogError(g_telemetryLogger->log, "Telemetry logger is not open");
        return -1;
    }

    if (g_telemetryLogger->logFile)
    {
        fclose(g_telemetryLogger->logFile);
        g_telemetryLogger->logFile = NULL;
    }

    g_telemetryLogger->isOpen = 0;

    // Set binary directory if not already set
    if (!g_telemetryLogger->binaryDirectory)
    {
        g_telemetryLogger->binaryDirectory = getModuleDirectory();
    }

    // Run telemetry proxy if binary directory is available
    if (g_telemetryLogger->binaryDirectory && g_telemetryLogger->filename)
    {
        runTelemetryProxy(g_telemetryLogger->filename, g_telemetryLogger->binaryDirectory);
    }

    // Clean up
    if (g_telemetryLogger->filename)
    {
        free(g_telemetryLogger->filename);
    }
    if (g_telemetryLogger->binaryDirectory)
    {
        free(g_telemetryLogger->binaryDirectory);
    }
    free(g_telemetryLogger);
    g_telemetryLogger = NULL;

    return 0;
}

int OSConfigTelemetryLogEvent(const char* eventName, const char** keyValuePairs, int pairCount)
{
    if (g_telemetryLogger == NULL || eventName == NULL)
    {
        return -1;
    }

    if (!g_telemetryLogger->isOpen || !g_telemetryLogger->logFile)
    {
        OsConfigLogError(g_telemetryLogger->log, "Telemetry logger is not open");
        return -1;
    }

    // Create JSON value and get root object
    JSON_Value* rootValue = json_value_init_object();
    if (rootValue == NULL)
    {
        OsConfigLogError(g_telemetryLogger->log, "Failed to create JSON root object");
        return -1;
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);
    if (rootObject == NULL)
    {
        json_value_free(rootValue);
        OsConfigLogError(g_telemetryLogger->log, "Failed to get JSON root object");
        return -1;
    }

    // Add timestamp
    const char* timestamp = GetFormattedTime();
    if (!timestamp || json_object_set_string(rootObject, "Timestamp", timestamp) != JSONSuccess)
    {
        json_value_free(rootValue);
        OsConfigLogError(g_telemetryLogger->log, "Failed to set Timestamp in JSON object");
        return -1;
    }

    // Add event name
    if (json_object_set_string(rootObject, "EventName", eventName) != JSONSuccess)
    {
        json_value_free(rootValue);
        OsConfigLogError(g_telemetryLogger->log, "Failed to set EventName in JSON object");
        return -1;
    }

    // Add key-value pairs directly to root object if provided
    if (keyValuePairs != NULL && pairCount > 0)
    {
        for (int i = 0; i < pairCount; i++)
        {
            const char* key = keyValuePairs[i * 2];
            const char* value = keyValuePairs[i * 2 + 1];

            if (key != NULL && value != NULL)
            {
                // Attempt to deduce the value type and serialize appropriately
                JSON_Status result = JSONFailure;

                // Try to parse as boolean first (exact matches)
                if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
                {
                    int boolValue = (strcmp(value, "true") == 0) ? 1 : 0;
                    result = json_object_set_boolean(rootObject, key, boolValue);
                }
                // Try to parse as null
                else if (strcmp(value, "null") == 0)
                {
                    result = json_object_set_null(rootObject, key);
                }
                // Try to parse as integer
                else
                {
                    char* endPtr = NULL;
                    long longValue = strtol(value, &endPtr, 10);

                    // Check if entire string was consumed and no overflow occurred
                    if (endPtr != NULL && *endPtr == '\0' && endPtr != value)
                    {
                        // Check if it fits in int range
                        if (longValue >= INT_MIN && longValue <= INT_MAX)
                        {
                            result = json_object_set_number(rootObject, key, (double)longValue);
                        }
                        else
                        {
                            // Value is too large for int, treat as string
                            result = json_object_set_string(rootObject, key, value);
                        }
                    }
                    else
                    {
                        // Try to parse as double
                        double doubleValue = strtod(value, &endPtr);

                        // Check if entire string was consumed
                        if (endPtr != NULL && *endPtr == '\0' && endPtr != value)
                        {
                            result = json_object_set_number(rootObject, key, doubleValue);
                        }
                        else
                        {
                            // Not a number, treat as string
                            result = json_object_set_string(rootObject, key, value);
                        }
                    }
                }

                if (result != JSONSuccess)
                {
                    json_value_free(rootValue);
                    OsConfigLogError(g_telemetryLogger->log, "Failed to set key-value pair in JSON object");
                    return -1;
                }
            }
        }
    }

    // Serialize JSON to string
    char* jsonString = json_serialize_to_string(rootValue);
    json_value_free(rootValue);

    if (jsonString == NULL)
    {
        OsConfigLogError(g_telemetryLogger->log, "Failed to serialize JSON to string");
        return -1;
    }

    // Write to file
    fprintf(g_telemetryLogger->logFile, "%s\n", jsonString);
    fflush(g_telemetryLogger->logFile);

    // Free the serialized string
    json_free_serialized_string(jsonString);

    return 0;
}

int OSConfigTelemetrySetBinaryDirectory(const char* directory)
{
    if (g_telemetryLogger == NULL || directory == NULL)
    {
        return -1;
    }

    // Free existing directory if set
    if (g_telemetryLogger->binaryDirectory)
    {
        free(g_telemetryLogger->binaryDirectory);
    }

    // Copy the new directory
    size_t dirLen = strlen(directory) + 1;
    g_telemetryLogger->binaryDirectory = malloc(dirLen);
    if (!g_telemetryLogger->binaryDirectory)
    {
        OsConfigLogError(g_telemetryLogger->log, "Failed to allocate memory for binary directory");
        return -1;
    }
    strcpy(g_telemetryLogger->binaryDirectory, directory);

    return 0;
}

const char* OSConfigTelemetryGetModuleDirectory(void)
{
    static char* moduleDir = NULL;
    if (!moduleDir)
    {
        moduleDir = getModuleDirectory();
    }
    return moduleDir;
}

#else // BUILD_TELEMETRY

// Stub implementations when BUILD_TELEMETRY is not enabled
int OSConfigTelemetryOpen(OsConfigLogHandle log)
{
    (void)log; // Suppress unused parameter warning
    return -1;
}

int OSConfigTelemetryClose(void)
{
    return -1;
}

int OSConfigTelemetryLogEvent(const char* eventName, const char** keyValuePairs, int pairCount)
{
    (void)eventName; // Suppress unused parameter warnings
    (void)keyValuePairs;
    (void)pairCount;
    return -1;
}

int OSConfigTelemetrySetBinaryDirectory(const char* directory)
{
    (void)directory; // Suppress unused parameter warning
    return -1;
}

const char* OSConfigTelemetryGetFilepath(OSConfigTelemetryHandle handle)
{
    (void)handle; // Suppress unused parameter warning
    return NULL;
}

const char* OSConfigTelemetryGetModuleDirectory(void)
{
    return NULL;
}

#endif // BUILD_TELEMETRY
