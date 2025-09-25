// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Logging.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opens the static telemetry logger instance
// Returns 0 on success, non-zero on failure
int OSConfigTelemetryOpen(OsConfigLogHandle log);

// Closes the static telemetry logger instance
// Returns 0 on success, non-zero on failure
int OSConfigTelemetryClose(void);

// eventName: Name of the event to log
// keyValuePairs: Array of key-value pair strings (must be even number of elements)
// pairCount: Number of key-value pairs (keyValuePairs array size / 2)
// Returns 0 on success, non-zero on failure
int OSConfigTelemetryLogEvent(const char* eventName, const char** keyValuePairs, int pairCount);

// Set the binary directory path for telemetry executable on the static instance
// directory: Path to the directory containing the telemetry binary
// Returns 0 on success, non-zero on failure
int OSConfigTelemetrySetBinaryDirectory(const char* directory);

// Get the directory of the current module
// Returns pointer to the directory string, or NULL on failure
const char* OSConfigTelemetryGetModuleDirectory();

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_H
