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

using namespace rapidjson;
std::unique_ptr<Settings> settings;

unsigned int maxPayloadSizeBytes = 0;

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

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    try
    {
        int status = MMI_OK;

        if ((nullptr == clientName) || (nullptr == payload) || (nullptr == payloadSizeBytes))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiGetInfo(%s, %.*s, %d) invalid arguments",
                    clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0));
            }
            status = EINVAL;
        }
        else
        {
            constexpr const char ret[] =
                R""""({
                "Name": "Settings",
                "Description": "Provides functionality to configure other settings on the device",
                "Manufacturer": "Microsoft",
                "VersionMajor": 0,
                "VersionMinor": 1,
                "VersionInfo": "Iron",
                "Components": ["Settings"],
                "Lifetime": 0,
                "UserAccount": 0})"""";

            std::size_t len = sizeof(ret) - 1;

            *payloadSizeBytes = len;
            *payload = new char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(SettingsLog::Get(), "MmiGetInfo failed to allocate %d bytes for payload", (int)len);
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, ret, len);
            }
        }

        ScopeGuard sg{[&]() {
            if ((MMI_OK == status) && (nullptr != payload) && (nullptr != payloadSizeBytes))
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
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(SettingsLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
                else
                {
                    OsConfigLogError(SettingsLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
            }
        }};

        return status;
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiGetInfo exception occurred");
        return ENODATA;
    }
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSize)
{
    try
    {
        int status = MMI_OK;
        MMI_HANDLE handle = nullptr;

        if (nullptr == clientName)
        {
            OsConfigLogError(SettingsLog::Get(), "MmiOpen(%s, %u) clientName %s is null", clientName, maxPayloadSize, clientName);
            status = EINVAL;
        }
        else
        {
            maxPayloadSizeBytes = maxPayloadSize;
            settings.reset(new Settings(maxPayloadSizeBytes));
            if (nullptr == settings.get())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiOpen Settings construction failed");
                status = ENODATA;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(settings.get());
            }
        }

        ScopeGuard sg{[&]() {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(SettingsLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
            else
            {
                OsConfigLogError(SettingsLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
        }};

        return handle;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiOpen exception occurred");
        return nullptr;
    }
}

void MmiClose(MMI_HANDLE clientSession)
{
    try
    {
        if (clientSession == nullptr)
        {
            OsConfigLogError(SettingsLog::Get(), "MmiClose MMI_HANDLE %p is null", clientSession);
        }
        else if (clientSession == reinterpret_cast<MMI_HANDLE>(settings.get()))
        {
            settings.reset();
        }
        else
        {
            OsConfigLogError(SettingsLog::Get(), "MmiClose MMI_HANDLE %p not recognized", clientSession);
        }
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiClose exception occurred");
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    try
    {
        int status = EINVAL;

        if (nullptr == clientSession)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) clientSession %p is null",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, clientSession);
            }
        }
        else if ((nullptr == componentName) || (SETTINGS != componentName))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) componentName %s is invalid, %s is expected",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, componentName, SETTINGS.c_str());
            }
        }
        else if ((nullptr == objectName) || ((DEVICEHEALTHTELEMETRY != objectName) && (DELIVERYOPTIMIZATION != objectName)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) objectName %s is invalid, %s or %s is expected",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, objectName, DEVICEHEALTHTELEMETRY.c_str(), DELIVERYOPTIMIZATION.c_str());
            }
        }
        else if (nullptr == payload)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) payload %p is null",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, payload);
            }
        }
        else if ((maxPayloadSizeBytes > 0) && (payloadSizeBytes > static_cast<int>(maxPayloadSizeBytes)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) payloadSizeBytes %d exceeds maxPayloadSizeBytes %d",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, payloadSizeBytes, static_cast<int>(maxPayloadSizeBytes));
            }
            status = E2BIG;
        }
        else
        {
            status = MMI_OK;
            std::string str(objectName);
            Document document;
            document.Parse(payload);

            if (document.HasParseError())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) parse operation failed with error: %s (offset: %u)\n",
                    clientSession, componentName, objectName, payload, payloadSizeBytes,
                    GetParseError_En(document.GetParseError()),
                    (unsigned)document.GetErrorOffset());

                status = ENODATA;
            }
            else if (str == DEVICEHEALTHTELEMETRY)
            {
                bool configurationChanged = false;
                static const char g_healthTelemetryConfigFile[] = "/etc/azure-device-health-services/config.toml";
                std::string DHStr = std::string(payload, payloadSizeBytes);

                status = settings->SetDeviceHealthTelemetryConfiguration(DHStr, g_healthTelemetryConfigFile, configurationChanged);

                if (status == MMI_OK)
                {
                    if (configurationChanged)
                    {
                        status = ExecuteCommand(nullptr, "systemctl kill -s SIGHUP azure-device-telemetryd.service", false, true, 0, 0, nullptr, nullptr, nullptr);
                        status = (0 == status) ? ExecuteCommand(nullptr, "systemctl kill -s SIGHUP azure-device-errorreporting-uploaderd.service", false, true, 0, 0, nullptr, nullptr, nullptr) : status;
                    }
                }
            }
            else if (str == DELIVERYOPTIMIZATION)
            {
                bool configurationChanged = false;
                static const char g_doConfigFile[] = "/etc/deliveryoptimization-agent/admin-config.json";
                Settings::DeliveryOptimization deliveryOptimization;
                if (document.HasMember(PERCENTAGEDOWNLOADTHROTTLE.c_str()))
                {
                    deliveryOptimization.percentageDownloadThrottle = document[PERCENTAGEDOWNLOADTHROTTLE.c_str()].GetInt();
                }

                if (document.HasMember(CACHEHOSTSOURCE.c_str()))
                {
                    deliveryOptimization.cacheHostSource = document[CACHEHOSTSOURCE.c_str()].GetInt();
                }

                if (document.HasMember(CACHEHOST.c_str()))
                {
                    deliveryOptimization.cacheHost = document[CACHEHOST.c_str()].GetString();
                }

                if (document.HasMember(CACHEHOSTFALLBACK.c_str()))
                {
                    deliveryOptimization.cacheHostFallback = document[CACHEHOSTFALLBACK.c_str()].GetInt();
                }

                status = settings->SetDeliveryOptimizationPolicies(deliveryOptimization, g_doConfigFile, configurationChanged);

                if ((status == MMI_OK) && configurationChanged)
                {
                    status = ExecuteCommand(nullptr, "systemctl kill -s SIGHUP deliveryoptimization-agent", false, true, 0, 0, nullptr, nullptr, nullptr);
                }
            }
        }

        ScopeGuard sg{[&]() {
            if (MMI_OK == status)
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) returned %d",
                        clientSession, componentName, objectName, payload, payloadSizeBytes, status);
                }
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SettingsLog::Get(), "MmiSet(%p, %s, %s, %p, %d) returned %d",
                    clientSession, componentName, objectName, payload, payloadSizeBytes, status);
            }
        }};

        return status;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(SettingsLog::Get(), "MmiSet exception occurred");
        return ENODATA;
    }
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