// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <CommonUtils.h>
#include <Mmi.h>
#include <ScopeGuard.h>
#include <Settings.h>

void __attribute__((constructor)) InitModule()
{
    SettingsLog::OpenLog();
    OsConfigLogInfo(SettingsLog::Get(), "Settings module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(SettingsLog::Get(), "Settings module unloaded");
    SettingsLog::CloseLog();
}

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Settings",
    "Description": "Provides functionality to configure other settings on the device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 0,
    "VersionMinor": 1,
    "VersionInfo": "Iron",
    "Components": ["Settings"],
    "Lifetime": 0,
    "UserAccount": 0})"""";

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(SettingsLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(SettingsLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            OsConfigLogError(SettingsLog::Get(), "MmiGetInfo(%s, %p, %p) returned %d", clientName, payload, payloadSizeBytes, status);
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(SettingsLog::Get(), "Invalid clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(SettingsLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SettingsLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(SettingsLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_moduleInfo, len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    MMI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]() {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(SettingsLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(SettingsLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        Settings* settings = new (std::nothrow) Settings(maxPayloadSizeBytes);
        if (nullptr != settings)
        {
            handle = reinterpret_cast<MMI_HANDLE>(settings);
        }
        else
        {
            OsConfigLogError(SettingsLog::Get(), "MmiOpen Settings construction failed");
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogError(SettingsLog::Get(), "MmiOpen(%s, %u) clientName %s is null", clientName, maxPayloadSizeBytes, clientName);
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    Settings* settings = reinterpret_cast<Settings*>(clientSession);
    if (settings != nullptr)
    {
        delete settings;
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MMI_OK;
    Settings* settings = reinterpret_cast<Settings*>(clientSession);

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(SettingsLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == settings)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else if (nullptr == componentName)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with null componentName");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with null objectName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with null payload");
        status = EINVAL;
    }
    else if (payloadSizeBytes < 0)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with negative payloadSizeBytes");
        status = EINVAL;
    }
    else if ((payloadSizeBytes > 0) && (payloadSizeBytes > static_cast<int>(settings->GetMaxPayloadSizeBytes())))
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet called with invalid payloadSizeBytes");
        status = E2BIG;
    }
    else
    {
        rapidjson::Document document;
        if (!document.Parse(payload, payloadSizeBytes).HasParseError())
        {
            if (0 == g_componentName.compare(componentName))
            {
                if (0 == g_deviceHealthTelemetry.compare(objectName))
                {
                    bool configurationChanged = false;
                    std::string payloadString = std::string(payload, payloadSizeBytes);
                    status = settings->SetDeviceHealthTelemetryConfiguration(payloadString, g_healthTelemetryConfigFile, configurationChanged);

                    if ((MMI_OK == status && configurationChanged))
                    {
                        status = ExecuteCommand(nullptr, "systemctl kill -s SIGHUP azure-device-telemetryd.service", false, true, 0, 0, nullptr, nullptr, nullptr);
                        status = (0 == status) ? ExecuteCommand(nullptr, "systemctl kill -s SIGHUP azure-device-errorreporting-uploaderd.service", false, true, 0, 0, nullptr, nullptr, nullptr) : status;
                    }
                }
                else if (0 == g_deliveryOptimization.compare(objectName))
                {
                    bool configurationChanged = false;
                    Settings::DeliveryOptimization deliveryOptimization;

                    if (document.HasMember(g_percentageDownloadThrottle.c_str()))
                    {
                        deliveryOptimization.percentageDownloadThrottle = document[g_percentageDownloadThrottle.c_str()].GetInt();
                    }

                    if (document.HasMember(g_cacheHostSource.c_str()))
                    {
                        deliveryOptimization.cacheHostSource = document[g_cacheHostSource.c_str()].GetInt();
                    }

                    if (document.HasMember(g_cacheHost.c_str()))
                    {
                        deliveryOptimization.cacheHost = document[g_cacheHost.c_str()].GetString();
                    }

                    if (document.HasMember(g_cacheHostFallback.c_str()))
                    {
                        deliveryOptimization.cacheHostFallback = document[g_cacheHostFallback.c_str()].GetInt();
                    }

                    status = settings->SetDeliveryOptimizationPolicies(deliveryOptimization, g_doConfigFile, configurationChanged);

                    if ((status == MMI_OK) && configurationChanged)
                    {
                        status = ExecuteCommand(nullptr, "systemctl kill -s SIGHUP deliveryoptimization-agent", false, true, 0, 0, nullptr, nullptr, nullptr);
                    }
                }
                else
                {
                    OsConfigLogError(SettingsLog::Get(), "MmiSet called with invalid objectName: %s", objectName);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet called with invalid componentName: %s", componentName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SettingsLog::Get(), "Unable to parse JSON payload");
            status = EINVAL;
        }
    }

    return status;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return ENOSYS;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (payload)
    {
        delete[] payload;
    }
}