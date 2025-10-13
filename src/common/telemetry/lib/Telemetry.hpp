// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <Keys.h>
#include <LogManager.hpp>
#include <Logging.h>

#include "ParameterSets.hpp"

namespace Telemetry
{

    class TelemetryManager
    {
    public:
        static const int CONFIG_DEFAULT_TEARDOWN_TIME = 5; // seconds
        static const char* INSTANCE_NAME = "OSConfigTelemetry";
        static const char* TELEMETRY_VERSION = "1.0.0";

        explicit TelemetryManager(bool enableDebug = false, int teardownTime = CONFIG_DEFAULT_TEARDOWN_TIME);

        ~TelemetryManager() noexcept;

        // Delete copy constructor and assignment operator
        TelemetryManager(const TelemetryManager&) = delete;
        TelemetryManager& operator=(const TelemetryManager&) = delete;

        // Delete move constructor and assignment operator
        TelemetryManager(TelemetryManager&&) = delete;
        TelemetryManager& operator=(TelemetryManager&&) = delete;

        // Parse JSON file line by line and process events
        bool ProcessJsonFile(const std::string& filePath);

    private:
        // Private instance members
        OsConfigLogHandle m_log;
        std::unique_ptr<ILogManager> m_logger;

        // Generic event logging (private)
        void EventWrite(Microsoft::Applications::Events::EventProperties event);

        // Validate JSON parameters against event parameter set
        bool ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys);

        // Process a single JSON line (private)
        void ProcessJsonLine(const std::string& jsonLine);
    };

} // namespace Telemetry

#endif // TELEMETRY_HPP
