// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include "ParameterSets.hpp"

#include <Keys.h>
#include <LogManager.hpp>
#include <Logging.h>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Telemetry
{

class TelemetryManager
{
public:
    static const int CONFIG_DEFAULT_TEARDOWN_TIME = 5; // seconds
    static constexpr const char* TELEMETRY_VERSION = "1.0.0";

    explicit TelemetryManager(bool enableDebug = false, int teardownTime = CONFIG_DEFAULT_TEARDOWN_TIME, OsConfigLogHandle logHandle = nullptr);

    ~TelemetryManager() noexcept;

    TelemetryManager() = delete;
    TelemetryManager(const TelemetryManager&) = delete;
    TelemetryManager& operator=(const TelemetryManager&) = delete;
    TelemetryManager(TelemetryManager&&) = delete;
    TelemetryManager& operator=(TelemetryManager&&) = delete;

    // Parse JSON file line by line and process events
    bool ProcessJsonFile(const std::string& filePath);

private:
    OsConfigLogHandle m_log;
    MAT::ILogConfiguration m_logConfig;
    std::unique_ptr<MAT::ILogManager> m_logManager;
    MAT::ILogger* m_logger;

    void EventWrite(Microsoft::Applications::Events::EventProperties event);
    bool ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys);
    bool ProcessJsonLine(const std::string& jsonLine);
};

} // namespace Telemetry

#endif // TELEMETRY_HPP
