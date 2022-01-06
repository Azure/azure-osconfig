// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include <Logging.h>

const std::string SETTINGS = "Settings";
const std::string DEVICEHEALTHTELEMETRY = "DeviceHealthTelemetryConfiguration";
const std::string DELIVERYOPTIMIZATION = "DeliveryOptimizationPolicies";
const std::string PERCENTAGEDOWNLOADTHROTTLE = "PercentageDownloadThrottle";
const std::string CACHEHOSTSOURCE = "CacheHostSource";
const std::string CACHEHOST = "CacheHost";
const std::string CACHEHOSTFALLBACK = "CacheHostFallback";

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
        int SetDeliveryOptimizationPolicies(Settings::DeliveryOptimization deliveryoptimization, const char* fileName, bool &configurationChanged);

    private:
        unsigned int maxPayloadSizeInBytes;
};