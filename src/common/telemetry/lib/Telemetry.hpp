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

        // Parse JSON file line by line and process events
        static bool ProcessJsonFile(const std::string& filePath);

        // Setup configuration
        static void SetupConfiguration(bool enableDebug, int teardownTime);

        // Shutdown telemetry system
        static void Shutdown();

        ~TelemetryManager() noexcept;

    private:
        // Private constructor to prevent instantiation
        TelemetryManager() = delete;

        // Delete copy constructor and assignment operator
        TelemetryManager(const TelemetryManager&) = delete;
        TelemetryManager& operator=(const TelemetryManager&) = delete;

        // Private static members and methods for internal implementation
        static OsConfigLogHandle m_log;
        static MAT::ILogger* m_logger;
        static bool m_initialized;

        // Initialize telemetry system (private)
        static bool Initialize(bool enableDebug = false, int teardownTime = CONFIG_DEFAULT_TEARDOWN_TIME);

        // Generic event logging (private)
        static void EventWrite(Microsoft::Applications::Events::EventProperties event);

        // Validate JSON parameters against event parameter set
        static bool ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys);

        // Process a single JSON line (private)
        static void ProcessJsonLine(const std::string& jsonLine);
    };

} // namespace Telemetry

#endif // TELEMETRY_HPP
