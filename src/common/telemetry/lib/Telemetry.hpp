// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include <memory>
#include <mutex>
#include <parson.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <Keys.h>
#include <LogManager.hpp>
#include <Logging.h>

#include "ParameterSets.hpp"

namespace Telemetry {

    class TelemetryManager {
    public:
        static const int CONFIG_DEFAULT_TEARDOWN_TIME = 5; // seconds

        // Singleton pattern - get the global instance
        static TelemetryManager& GetInstance();

        // Delete copy constructor and assignment operator to enforce singleton
        TelemetryManager(const TelemetryManager&) = delete;
        TelemetryManager& operator=(const TelemetryManager&) = delete;

        // Initialize telemetry system
        bool Initialize(bool enableDebug = false, int teardownTime = CONFIG_DEFAULT_TEARDOWN_TIME);

        // Check if telemetry is initialized
        bool IsInitialized() const;

        // Parse JSON file line by line and process events
        bool ProcessJsonFile(const std::string& filePath);

        // Shutdown telemetry system
        void Shutdown();

    private:
        // Private constructor for singleton
        TelemetryManager();

        // Generic event logging (private)
        void EventWrite(Microsoft::Applications::Events::EventProperties event);

        // Validate JSON parameters against event parameter set
        bool ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys) const;

        // Private members
        MAT::ILogger* m_logger;
        bool m_initialized;
        mutable std::mutex m_mutex;
        static OsConfigLogHandle m_logfileHandle;

        // Initialize configuration
        void SetupConfiguration(bool enableDebug, int teardownTime);

        // Process a single JSON line
        void ProcessJsonLine(const std::string& jsonLine);
    };

} // namespace Telemetry

#endif // TELEMETRY_HPP
