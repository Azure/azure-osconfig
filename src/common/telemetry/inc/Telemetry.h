// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the telemetry logger (C interface)
typedef struct TelemetryLogger* OSConfigTelemetryHandle;

// Opens a new telemetry logger instance
// Returns handle to the logger instance, or NULL on failure
OSConfigTelemetryHandle OSConfigTelemetryOpen(void);

// Closes the telemetry logger instance
// handle: Handle to the logger instance to close
// Returns 0 on success, non-zero on failure
int OSConfigTelemetryClose(OSConfigTelemetryHandle* handle);

// Logs an event with key-value pairs
// handle: Handle to the logger instance
// eventName: Name of the event to log
// keyValuePairs: Array of key-value pair strings (must be even number of elements)
// pairCount: Number of key-value pairs (keyValuePairs array size / 2)
// Returns 0 on success, non-zero on failure
int OSConfigTelemetryLogEvent(OSConfigTelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount);

// Set the binary directory path for telemetry executable
// handle: Handle to the logger instance
// directory: Path to the directory containing the telemetry binary
// Returns 0 on success, non-zero on failure
int OSConfigTelemetrySetBinaryDirectory(OSConfigTelemetryHandle handle, const char* directory);

// Get the directory of the current module
// Returns pointer to the directory string, or NULL on failure
const char* OSConfigTelemetryGetModuleDirectory();

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_H
