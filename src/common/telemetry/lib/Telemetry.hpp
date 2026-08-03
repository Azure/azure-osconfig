// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include <Logging.h>
#include <chrono>
#include <string>

namespace Telemetry
{

class TelemetryManagerImpl;

class TelemetryManager
{
public:
    static constexpr std::chrono::seconds CONFIG_DEFAULT_TEARDOWN_TIME{5};
    static constexpr const char* TELEMETRY_NAME = "OSConfigTelemetry";
    static constexpr const char* TELEMETRY_VERSION = "1.0.0";
    static constexpr const char* TELEMETRY_CACHE_FILE_NAME = "/var/lib/osconfig/telemetry/cache.db";
    static constexpr const int TELEMETRY_CACHE_FILE_SIZE = 10 * 1024 * 1024;
    static constexpr const int TELEMETRY_RAM_QUEUE_SIZE = 2 * 1024 * 1024;

    explicit TelemetryManager(std::string cacheFilePath, bool enableDebug = false, std::chrono::seconds teardownTime = CONFIG_DEFAULT_TEARDOWN_TIME,
        OsConfigLogHandle logHandle = nullptr);

    ~TelemetryManager() noexcept;

    TelemetryManager() = delete;
    TelemetryManager(const TelemetryManager&) = delete;
    TelemetryManager& operator=(const TelemetryManager&) = delete;
    TelemetryManager(TelemetryManager&&) = delete;
    TelemetryManager& operator=(TelemetryManager&&) = delete;

    // Parse JSON file line by line and process events
    bool ProcessJsonFile(const std::string& filePath);

private:
    TelemetryManagerImpl* m_impl;
};

} // namespace Telemetry

#endif // TELEMETRY_HPP
