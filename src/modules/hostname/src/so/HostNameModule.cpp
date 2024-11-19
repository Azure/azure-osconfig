// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <memory>
#include <Logging.h>
#include <CommonUtils.h>
#include <HostName.h>
#include <Mmi.h>
#include <ScopeGuard.h>
#include <functional>
#include <vector>
#include <map>
#include <string>
#include <rapidjson/document.h>

#define ERROR_MEMORY_ALLOCATION "%s memory allocation failed"
#define ERROR_GENERAL_EXCEPTION "%s threw an exception"
#define ERROR_INVALID_CLIENT_SESSION "%s called with an invalid client session"
#define ERROR_INVALID_ARGUMENTS "%s called with an invalid argument"

constexpr const char* g_moduleName = "HostName";
constexpr const char g_moduleInfo[] = R""""({
    "Name": "HostName",
    "Description": "Provides functionality to observe and configure network hostname and hosts",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["HostName"],
    "Lifetime": 2,
    "UserAccount": 0})"""";

void __attribute__((constructor)) InitModule()
{
    HostNameLog::OpenLog();
    OsConfigLogInfo(HostNameLog::Get(), "%s module loaded", g_moduleName);
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(HostNameLog::Get(), "%s module unloaded", g_moduleName);
    HostNameLog::CloseLog();
}

int MmiGetInfoInternal(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    ScopeGuard sg{[&]()
    {
        if (status == MMI_OK)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(HostNameLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(HostNameLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(HostNameLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(HostNameLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    if (!clientName || !payload || !payloadSizeBytes)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_ARGUMENTS, "MmiGetInfo");
        status = EINVAL;
    }
    else
    {
        std::size_t len = sizeof(g_moduleInfo) - 1;
        *payloadSizeBytes = len;
        *payload = new char[len];
        if (!payload)
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_MEMORY_ALLOCATION, "MmiGetInfo");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_moduleInfo, len);
        }
    }
    return status;
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    try
    {
        return MmiGetInfoInternal(clientName, payload, payloadSizeBytes);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiGetInfo");
        return EFAULT;
    }
}

MMI_HANDLE MmiOpenInternal(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = nullptr;
    int status = MMI_OK;
    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(HostNameLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
        }
        else
        {
            OsConfigLogError(HostNameLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
        }
    }};

    if (!clientName)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_ARGUMENTS, "MmiOpen");
        status = EINVAL;
    }
    else
    {
        HostName *hostName = new (std::nothrow) HostName(maxPayloadSizeBytes);
        if (!hostName)
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_MEMORY_ALLOCATION, "MmiOpen");
            status = ENOMEM;
        }
        else
        {
            handle = reinterpret_cast<MMI_HANDLE>(hostName);
        }
    }
    return handle;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    try
    {
        return MmiOpenInternal(clientName, maxPayloadSizeBytes);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiOpen");
        return nullptr;
    }
}

void MmiCloseInternal(MMI_HANDLE clientSession)
{
    OsConfigLogInfo(HostNameLog::Get(), "MmiClose(%p)", clientSession);
    if (clientSession)
    {
        HostName *hostName = reinterpret_cast<HostName *>(clientSession);
        if (!hostName)
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_CLIENT_SESSION, "MmiClose");
        }
        else
        {
            delete hostName;
            return;
        }
    }
    OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_ARGUMENTS, "MmiClose");
}

void MmiClose(MMI_HANDLE clientSession)
{
    try
    {
        return MmiCloseInternal(clientSession);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiClose");
    }
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    try
    {
        int status = MMI_OK;
        ScopeGuard sg{[&]()
        {
            if (IsFullLoggingEnabled())
            {
                if (MMI_OK == status)
                {
                    OsConfigLogInfo(HostNameLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
                }
                else
                {
                    OsConfigLogError(HostNameLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
                }
            }
            else
            {
                if (MMI_OK != status)
                {
                    OsConfigLogError(HostNameLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
                }
            }
        }};

        HostName *hostName = reinterpret_cast<HostName *>(clientSession);
        if (!hostName)
        {
            OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_CLIENT_SESSION, "MmiSet");
            status = EINVAL;
        }
        else
        {
            status = hostName->Set(clientSession, componentName, objectName, payload, payloadSizeBytes);
        }
        return status;
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiSet");
        return EFAULT;
    }
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    try
    {
        int status = MMI_OK;
        ScopeGuard sg{[&]()
        {
            if (IsFullLoggingEnabled())
            {
                if (MMI_OK == status)
                {
                    OsConfigLogInfo(HostNameLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
                else
                {
                    OsConfigLogError(HostNameLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
            }
        }};

        HostName *hostName = reinterpret_cast<HostName *>(clientSession);
        if (!hostName)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(HostNameLog::Get(), ERROR_INVALID_CLIENT_SESSION, "MmiGet");
            }
            status = EINVAL;
        }
        else
        {
            status = hostName->Get(clientSession, componentName, objectName, payload, payloadSizeBytes);
        }
        return status;
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiGet");
        return EFAULT;
    }
}

void MmiFree(MMI_JSON_STRING payload)
{
    try
    {
        return ::HostNameFree(payload);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(HostNameLog::Get(), ERROR_GENERAL_EXCEPTION, "MmiFree");
    }
}
