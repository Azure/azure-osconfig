// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <Logging.h>

const std::string g_componentName = "Settings";
const std::string g_deviceHealthTelemetry = "deviceHealthTelemetryConfiguration";
const std::string g_deliveryOptimization = "deliveryOptimizationPolicies";

const std::string g_percentageDownloadThrottle = "percentageDownloadThrottle";
const std::string g_cacheHostSource = "cacheHostSource";
const std::string g_cacheHost = "cacheHost";
const std::string g_cacheHostFallback = "cacheHostFallback";

static const char g_healthTelemetryConfigFile[] = "/etc/azure-device-health-services/config.toml";
static const char g_doConfigFile[] = "/etc/deliveryoptimization-agent/admin-config.json";

#define SETTINGS_LOGFILE "/var/log/osconfig_settings.log"
#define SETTINGS_ROLLEDLOGFILE "/var/log/osconfig_settings.bak"

class SettingsLog
{
    public:
        static OSCONFIG_LOG_HANDLE Get()
        {
            return m_logSettings;
        }

        static void OpenLog()
        {
            m_logSettings = ::OpenLog(SETTINGS_LOGFILE, SETTINGS_ROLLEDLOGFILE);
        }

        static void CloseLog()
        {
            ::CloseLog(&m_logSettings);
        }

    private:
        static OSCONFIG_LOG_HANDLE m_logSettings;
};

class Settings
{
    public:
        struct DeliveryOptimization
        {
            int percentageDownloadThrottle;
            int cacheHostSource;
            std::string cacheHost;
            int cacheHostFallback;
        };
        Settings(unsigned int maxSizeInBytes = 0);
        virtual ~Settings() = default;
        int SetDeviceHealthTelemetryConfiguration(std::string payload, const char* fileName, bool &configurationChanged);
        int SetDeliveryOptimizationPolicies(Settings::DeliveryOptimization deliveryoptimization, const char* fileName, bool& configurationChanged);

        unsigned int GetMaxPayloadSizeBytes() const;

    private:
        unsigned int maxPayloadSizeInBytes;
};