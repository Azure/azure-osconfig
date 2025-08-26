// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <set>
#include <unordered_map>

#include <keys.h>
#include <LogManager.hpp>
#include <parson.h>

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

        // Baseline completion event parameter set
        static const std::set<std::string> BASELINE_COMPLETE_REQUIRED_PARAMS;
        static const std::set<std::string> BASELINE_COMPLETE_OPTIONAL_PARAMS;

        // Baseline completion event
        // @param targetName: Target system name (e.g., "Debian GNU/Linux 12 (bookworm)")
        // @param baselineName: Name of the baseline (e.g., "SecurityBaseline")
        // @param mode: Execution mode ("audit-only" or "automatic remediation")
        // @param durationSeconds: Time taken to complete baseline in seconds
        void EventWrite_BaselineComplete(const std::string& targetName, const std::string& baselineName,
            const std::string& mode, int durationSeconds);

        // Parse JSON file line by line and process events
        bool ProcessJsonFile(const std::string& filePath);

        // Shutdown telemetry system
        void Shutdown();

        // Destructor
        // ~TelemetryManager();

    private:
        // Private constructor for singleton
        TelemetryManager();

        // Event parameter validation map
        static const std::unordered_map<std::string, std::pair<std::set<std::string>, std::set<std::string>>> EVENT_PARAMETER_SETS;

        // Generic event logging (private)
        void EventWrite(Microsoft::Applications::Events::EventProperties event);

        // Validate JSON parameters against event parameter set
        bool ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys) const;

        // Private members
        MAT::ILogger* m_logger;
        bool m_initialized;
        mutable std::mutex m_mutex;

        // Initialize configuration
        void SetupConfiguration(bool enableDebug, int teardownTime);

        // Process a single JSON line
        void ProcessJsonLine(const std::string& jsonLine);
    };

} // namespace Telemetry

#endif // TELEMETRY_H
