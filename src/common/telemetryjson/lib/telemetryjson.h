// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRYJSON_H
#define TELEMETRYJSON_H

#define TELEMETRY_BINARY_NAME "telemetry"

#ifdef __cplusplus

#include <string>
#include <memory>

// C++ interface
namespace TelemetryJson {

class Logger {
public:
    Logger();
    ~Logger();

    // Disable copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Enable move constructor and assignment operator
    Logger(Logger&& other) noexcept;
    Logger& operator=(Logger&& other) noexcept;

    /**
     * @brief Opens the logger for writing
     * @return true on success, false on failure
     */
    bool open();

    /**
     * @brief Closes the logger
     * @return true on success, false on failure
     */
    bool close();

    /**
     * @brief Logs an event with key-value pairs
     * @param eventName Name of the event to log
     * @param keyValuePairs Array of key-value pair strings (must be even number of elements)
     * @param pairCount Number of key-value pairs (keyValuePairs array size / 2)
     * @return true on success, false on failure
     */
    bool logEvent(const std::string& eventName, const char** keyValuePairs, int pairCount);

    /**
     * @brief Logs an event with key-value pairs using initializer list (string values only)
     * @param eventName Name of the event to log
     * @param keyValuePairs Initializer list of key-value pairs (string values)
     * @return true on success, false on failure
     */
    bool logEvent(const std::string& eventName,
                  std::initializer_list<std::pair<std::string, std::string>> keyValuePairs);

    /**
     * @brief Logs an event with no additional properties
     * @param eventName Name of the event to log
     * @return true on success, false on failure
     */
    bool logEvent(const std::string& eventName);

    /**
     * @brief Check if the logger is open
     * @return true if open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Get the filename being logged to
     * @return The filename, or empty string if not open
     */
    const std::string& getFilename() const;

    /**
     * @brief Set the binary directory path for telemetry executable
     * @param directory Path to the directory containing the telemetry binary
     */
    void setBinaryDirectory(const std::string& directory);

    /**
     * @brief Get the binary directory path
     * @return The binary directory path, or empty string if not set
     */
    const std::string& getBinaryDirectory() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace TelemetryJson

extern "C" {
#endif

// Opaque handle for the telemetry JSON logger (C interface)
typedef struct TelemetryJsonLogger* TelemetryJsonHandle;

/**
 * @brief Opens a new telemetry JSON logger instance
 *
 * Creates a new logger that writes JSON formatted entries to a random file in /tmp.
 * The filename will be of the format: /tmp/telemetry_XXXXXX.json where XXXXXX is random.
 *
 * @return TelemetryJsonHandle Handle to the logger instance, or NULL on failure
 */
TelemetryJsonHandle TelemetryJson_Open(void);

/**
 * @brief Closes the telemetry JSON logger instance
 *
 * @param handle Handle to the logger instance to close
 * @return int 0 on success, non-zero on failure
 */
int TelemetryJson_Close(TelemetryJsonHandle* handle);

/**
 * @brief Logs an event with key-value pairs in JSON format
 *
 * @param handle Handle to the logger instance
 * @param eventName Name of the event to log
 * @param keyValuePairs Array of key-value pair strings (must be even number of elements)
 * @param pairCount Number of key-value pairs (keyValuePairs array size / 2)
 * @return int 0 on success, non-zero on failure
 */
int TelemetryJson_LogEvent(TelemetryJsonHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount);

/**
 * @brief Set the binary directory path for telemetry executable
 *
 * @param handle Handle to the logger instance
 * @param directory Path to the directory containing the telemetry binary
 * @return int 0 on success, non-zero on failure
 */
int TelemetryJson_SetBinaryDirectory(TelemetryJsonHandle handle, const char* directory);

/**
 * @brief Get the filepath of the log file
 *
 * @param handle Handle to the logger instance
 * @return const char* Pointer to the filepath string, or NULL on failure
 */
const char* TelemetryJson_GetFilepath(TelemetryJsonHandle handle);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRYJSON_H
