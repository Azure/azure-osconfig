// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRYJSON_HPP
#define TELEMETRYJSON_HPP

#define TELEMETRY_BINARY_NAME "telemetry"
#define TELEMETRY_SYSLOG_IDENTIFIER "osconfig-telemetry"

#ifdef __cplusplus

#include <string>
#include <memory>

// C++ interface
namespace TelemetryJson
{

class Logger
{
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

#include <telemetry.h>

#ifdef __cplusplus
}
#endif

#endif // TELEMETRYJSON_HPP
