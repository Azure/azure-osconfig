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
OSConfigTelemetryHandle OSConfigTelemetryOpen(void)
{
    struct TelemetryLogger* logger = malloc(sizeof(struct TelemetryLogger));
    if (!logger)
    {
        return NULL;
    }

    // Initialize the logger
    logger->logFile = NULL;
    logger->filename = NULL;
    logger->binaryDirectory = NULL;
    logger->isOpen = 0;

    // Generate filename
    logger->filename = generateRandomFilename();
    if (!logger->filename)
    {
        free(logger);
        return NULL;
    }

    // Open file
    logger->logFile = fopen(logger->filename, "a");
    if (!logger->logFile)
    {
        free(logger->filename);
        free(logger);
        return NULL;
    }

    logger->isOpen = 1;
    return (OSConfigTelemetryHandle)logger;
}

int OSConfigTelemetryClose(OSConfigTelemetryHandle* handle)
{
    if (handle == NULL || *handle == NULL)
    {
        return -1;
    }

    struct TelemetryLogger* logger = (struct TelemetryLogger*)*handle;

    if (!logger->isOpen)
    {
        return -1;
    }

    if (logger->logFile)
    {
        fclose(logger->logFile);
        logger->logFile = NULL;
    }

    logger->isOpen = 0;

    // Set binary directory if not already set
    if (!logger->binaryDirectory)
    {
        logger->binaryDirectory = getModuleDirectory();
    }

    // Run telemetry proxy if binary directory is available
    if (logger->binaryDirectory && logger->filename)
    {
        runTelemetryProxy(logger->filename, logger->binaryDirectory);
    }

    // Clean up
    if (logger->filename)
    {
        free(logger->filename);
    }
    if (logger->binaryDirectory)
    {
        free(logger->binaryDirectory);
    }
    free(logger);
    *handle = NULL;

    return 0;
}

int OSConfigTelemetryLogEvent(OSConfigTelemetryHandle handle, const char* eventName,
                              const char** keyValuePairs, int pairCount)
{
    if (handle == NULL || eventName == NULL)
    {
        return -1;
    }

    struct TelemetryLogger* logger = (struct TelemetryLogger*)handle;

    if (!logger->isOpen || !logger->logFile)
    {
        return -1;
    }

    // Create JSON value and get root object
    JSON_Value* rootValue = json_value_init_object();
    if (rootValue == NULL)
    {
        return -1;
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);
    if (rootObject == NULL)
    {
        json_value_free(rootValue);
        return -1;
    }

    // Add timestamp
    const char* timestamp = GetFormattedTime();
    if (!timestamp || json_object_set_string(rootObject, "Timestamp", timestamp) != JSONSuccess)
    {
        json_value_free(rootValue);
        return -1;
    }

    // Add event name
    if (json_object_set_string(rootObject, "EventName", eventName) != JSONSuccess)
    {
        json_value_free(rootValue);
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
        return -1;
    }

    // Write to file
    fprintf(logger->logFile, "%s\n", jsonString);
    fflush(logger->logFile);

    // Free the serialized string
    json_free_serialized_string(jsonString);

    return 0;
}

int OSConfigTelemetrySetBinaryDirectory(OSConfigTelemetryHandle handle, const char* directory)
{
    if (handle == NULL || directory == NULL)
    {
        return -1;
    }

    struct TelemetryLogger* logger = (struct TelemetryLogger*)handle;

    // Free existing directory if set
    if (logger->binaryDirectory)
    {
        free(logger->binaryDirectory);
    }

    // Copy the new directory
    size_t dirLen = strlen(directory) + 1;
    logger->binaryDirectory = malloc(dirLen);
    if (!logger->binaryDirectory)
    {
        return -1;
    }
    strcpy(logger->binaryDirectory, directory);

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
OSConfigTelemetryHandle OSConfigTelemetryOpen(void)
{
    return NULL;
}

int OSConfigTelemetryClose(OSConfigTelemetryHandle* handle)
{
    (void)handle; // Suppress unused parameter warning
    return -1;
}

int OSConfigTelemetryLogEvent(OSConfigTelemetryHandle handle, const char* eventName,
                              const char** keyValuePairs, int pairCount)
{
    (void)handle; // Suppress unused parameter warnings
    (void)eventName;
    (void)keyValuePairs;
    (void)pairCount;
    return -1;
}

int OSConfigTelemetrySetBinaryDirectory(OSConfigTelemetryHandle handle, const char* directory)
{
    (void)handle; // Suppress unused parameter warnings
    (void)directory;
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
