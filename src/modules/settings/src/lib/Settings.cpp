// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <exception>
#include <ScopeGuard.h>
#include <Mmi.h>
#include <CommonUtils.h>
#include <ConfigFileUtils.h>
#include <Settings.h>

using namespace std;

static const char g_healthTelemetryConfigValue[] = "Permission";
static const char g_healthTelemetryNone[] = "None";
static const char g_healthTelemetryRequired[] = "Required";
static const char g_healthTelemetryOptional[] = "Optional";

static const char g_doPercentageDownloadThrottle[] = "/DOPercentageDownloadThrottle";
static const char g_doCacheHostSource[] = "/DOCacheHostSource";
static const char g_doCacheHost[] = "/DOCacheHost";
static const char g_doCacheHostFallback[] = "/DOCacheHostFallback";

OSCONFIG_LOG_HANDLE SettingsLog::m_logSettings = nullptr;

Settings::Settings(unsigned int maxSizeInBytes)
{
    maxPayloadSizeInBytes = maxSizeInBytes;
}

unsigned int Settings::GetMaxPayloadSizeBytes() const
{
    return maxPayloadSizeInBytes;
}

int Settings::SetDeviceHealthTelemetryConfiguration(std::string payload, const char* fileName, bool &configurationChanged)
{
    char* valueToWrite = NULL;
    CONFIG_FILE_HANDLE config = NULL;

    if (payload == "0")
    {
        valueToWrite = (char*)g_healthTelemetryNone;
    }
    else if (payload == "1")
    {
        valueToWrite = (char*)g_healthTelemetryRequired;
    }
    else if (payload == "2")
    {
        valueToWrite = (char*)g_healthTelemetryOptional;
    }
    else
    {
        OsConfigLogError(SettingsLog::Get(), "Argument payload %s is invalid", payload.c_str());
        return EINVAL;
    }

    if (!FileExists(fileName))
    {
        OsConfigLogError(SettingsLog::Get(), "Argument fileName %s not found", fileName);
        return ENOENT;
    }

    config = OpenConfigFile(fileName, ConfigFileFormatToml);

    int result = MMI_OK;
    if (NULL != config)
    {
        char* valueToRead = ReadConfigString(config, g_healthTelemetryConfigValue);
        if ((NULL == valueToRead) || (0 != strcmp(valueToRead, valueToWrite)))
        {
            configurationChanged = true;
            result = WriteConfigString(config, g_healthTelemetryConfigValue, valueToWrite);
            FreeConfigString(valueToRead);
            CloseConfigFile(config);
        }
    }

    return result;
}

int Settings::SetDeliveryOptimizationPolicies(Settings::DeliveryOptimization deliveryoptimization, const char* fileName, bool &configurationChanged)
{
    int percentageDownloadThrottle = deliveryoptimization.percentageDownloadThrottle;
    int cacheHostSource = deliveryoptimization.cacheHostSource;
    const char* cacheHost = deliveryoptimization.cacheHost.c_str();
    int cacheHostFallback = deliveryoptimization.cacheHostFallback;

    if ((percentageDownloadThrottle < 0) || (percentageDownloadThrottle > 100))
    {
        OsConfigLogError(SettingsLog::Get(), "Policy percentageDownloadThrottle %d is invalid", percentageDownloadThrottle);
        return EINVAL;
    }

    if ((cacheHostSource < 0) || (cacheHostSource > 3))
    {
        OsConfigLogError(SettingsLog::Get(), "Policy cacheHostSource %d is invalid", cacheHostSource);
        return EINVAL;
    }

    if (!FileExists(fileName))
    {
        OsConfigLogError(SettingsLog::Get(), "Argument fileName %s not found", fileName);
        return ENOENT;
    }

    CONFIG_FILE_HANDLE config = NULL;
    config = OpenConfigFile(fileName, ConfigFileFormatJson);
    int status = MMI_OK;

    if (NULL != config)
    {
        // PercentageDownloadThrottle
        int percentageDownloadThrottleExisting = ReadConfigInteger(config, g_doPercentageDownloadThrottle);
        if (percentageDownloadThrottleExisting != percentageDownloadThrottle)
        {
            int result = WriteConfigInteger(config, g_doPercentageDownloadThrottle, percentageDownloadThrottle);
            if (WRITE_CONFIG_SUCCESS != result)
            {
                OsConfigLogError(SettingsLog::Get(), "Write operation failed for percentageDownloadThrottle %d", percentageDownloadThrottle);
                status = EPERM;
            }
            else
            {
                configurationChanged = true;
            }
        }

        // CacheHostSource
        int cacheHostSourceExisting = ReadConfigInteger(config, g_doCacheHostSource);
        if (cacheHostSourceExisting != cacheHostSource)
        {
            int result = WriteConfigInteger(config, g_doCacheHostSource, cacheHostSource);
            if (WRITE_CONFIG_SUCCESS != result)
            {
                OsConfigLogError(SettingsLog::Get(), "Policy cacheHostSource write operation failed for cacheHostSource %d", cacheHostSource);
                status = EPERM;
            }
            else
            {
                configurationChanged = true;
            }
        }

        // CacheHost
        char* cacheHostExisting = ReadConfigString(config, g_doCacheHost);
        if ((NULL == cacheHostExisting) || (0 != strcmp(cacheHostExisting, cacheHost)))
        {
            int result = WriteConfigString(config, g_doCacheHost, cacheHost);
            if (WRITE_CONFIG_SUCCESS != result)
            {
                OsConfigLogError(SettingsLog::Get(), "Write operation failed for cacheHost %s", cacheHost);
                status = EPERM;
            }
            else
            {
                configurationChanged = true;
            }

            ScopeGuard sg {[&]()
            {
                FreeConfigString(cacheHostExisting);
                cacheHostExisting = NULL;
            }};
        }

        // CacheHostFallback
        int cacheHostFallbackExisting = ReadConfigInteger(config, g_doCacheHostFallback);
        if (cacheHostFallbackExisting != cacheHostFallback)
        {
            int result = WriteConfigInteger(config, g_doCacheHostFallback, cacheHostFallback);
            if (WRITE_CONFIG_SUCCESS != result)
            {
                OsConfigLogError(SettingsLog::Get(), "Write operation failed for cacheHostFallback %d", cacheHostFallback);
                status = EPERM;
            }
            else
            {
                configurationChanged = true;
            }
        }

        CloseConfigFile(config);
    }

    return status;
}