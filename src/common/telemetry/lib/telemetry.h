#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <string>
#include <memory>
#include <mutex>
#include <vector>

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

        // Generic event logging
        void LogEvent(const std::string& eventName);

        // Parse JSON file line by line and process events
        bool ProcessJsonFile(const std::string& filePath);

        // Shutdown telemetry system
        void Shutdown();

        // Destructor
        ~TelemetryManager();

    private:
        // Private constructor for singleton
        TelemetryManager();

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
