// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the telemetry logger (C interface)
typedef struct TelemetryLogger* OSConfigTelemetryHandle;

// @brief Opens a new telemetry logger instance
//
// @return OSConfigTelemetryHandle Handle to the logger instance, or NULL on failure
OSConfigTelemetryHandle OSConfigTelemetryOpen(void);

// @brief Closes the telemetry logger instance
//
// @param handle Handle to the logger instance to close
// @return int 0 on success, non-zero on failure
int OSConfigTelemetryClose(OSConfigTelemetryHandle* handle);

// @brief Logs an event with key-value pairs
//
// @param handle Handle to the logger instance
// @param eventName Name of the event to log
// @param keyValuePairs Array of key-value pair strings (must be even number of elements)
// @param pairCount Number of key-value pairs (keyValuePairs array size / 2)
// @return int 0 on success, non-zero on failure
int OSConfigTelemetryLogEvent(OSConfigTelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount);

// @brief Set the binary directory path for telemetry executable
//
// @param handle Handle to the logger instance
// @param directory Path to the directory containing the telemetry binary
// @return int 0 on success, non-zero on failure
int OSConfigTelemetrySetBinaryDirectory(OSConfigTelemetryHandle handle, const char* directory);

// @brief Get the filepath of the log file
//
// @param handle Handle to the logger instance
// @return const char* to the filepath string, or NULL on failure
const char* OSConfigTelemetryGetFilepath(OSConfigTelemetryHandle handle);

// @brief Get the directory of the current module
//
// @return char* to the directory string, or NULL on failure
const char* OSConfigTelemetryGetModuleDirectory();

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_H
