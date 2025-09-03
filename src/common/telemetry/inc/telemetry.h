// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the telemetry JSON logger (C interface)
typedef struct TelemetryLogger* TelemetryHandle;

/**
 * @brief Opens a new telemetry JSON logger instance
 *
 * Creates a new logger that writes JSON formatted entries to a random file in /tmp.
 * The filename will be of the format: /tmp/telemetry_XXXXXX.json where XXXXXX is random.
 *
 * @return TelemetryHandle Handle to the logger instance, or NULL on failure
 */
TelemetryHandle Telemetry_Open(void);

/**
 * @brief Closes the telemetry JSON logger instance
 *
 * @param handle Handle to the logger instance to close
 * @return int 0 on success, non-zero on failure
 */
int Telemetry_Close(TelemetryHandle* handle);

/**
 * @brief Logs an event with key-value pairs in JSON format
 *
 * @param handle Handle to the logger instance
 * @param eventName Name of the event to log
 * @param keyValuePairs Array of key-value pair strings (must be even number of elements)
 * @param pairCount Number of key-value pairs (keyValuePairs array size / 2)
 * @return int 0 on success, non-zero on failure
 */
int Telemetry_LogEvent(TelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount);

/**
 * @brief Set the binary directory path for telemetry executable
 *
 * @param handle Handle to the logger instance
 * @param directory Path to the directory containing the telemetry binary
 * @return int 0 on success, non-zero on failure
 */
int Telemetry_SetBinaryDirectory(TelemetryHandle handle, const char* directory);

/**
 * @brief Get the filepath of the log file
 *
 * @param handle Handle to the logger instance
 * @return const char* Pointer to the filepath string, or NULL on failure
 */
const char* Telemetry_GetFilepath(TelemetryHandle handle);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_H
