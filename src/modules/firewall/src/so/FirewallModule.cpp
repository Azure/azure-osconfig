// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <memory>
#include <Logging.h>
#include <CommonUtils.h>
#include <Firewall.h>
#include <Mmi.h>
#include <ScopeGuard.h>
#include <vector>
#include <string>

constexpr const char g_firewallInfo[] = R""""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

void __attribute__((constructor)) InitModule()
{
    FirewallLog::OpenLog();
    OsConfigLogInfo(FirewallLog::Get(), "Firewall module loaded");
}
void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(FirewallLog::Get(), "Firewall module unloaded");
    FirewallLog::CloseLog();
}

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
                OsConfigLogInfo(FirewallLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(FirewallLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(FirewallLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    } };

    if (nullptr == clientName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            size_t len = strlen(g_firewallInfo);
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(FirewallLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, g_firewallInfo, len);
                *payloadSizeBytes = len;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
            status = EINTR;

            if (nullptr != *payload)
            {
                delete[] * payload;
                *payload = nullptr;
            }

            if (nullptr != payloadSizeBytes)
            {
                *payloadSizeBytes = 0;
            }
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

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(FirewallLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        FirewallObject* session = new (std::nothrow) FirewallObject(maxPayloadSizeBytes);
        if (nullptr == session)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiOpen failed to allocate memory");
            status = ENOMEM;
        }
        else
        {
            handle = reinterpret_cast<MMI_HANDLE>(session);
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    if (nullptr != clientSession)
    {
        FirewallObject* session = reinterpret_cast<FirewallObject*>(clientSession);
        delete session;
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
    FirewallObject* firewall = reinterpret_cast<FirewallObject*>(clientSession);

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(FirewallLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
        }
    } };

    if (nullptr == clientSession)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else if (nullptr == componentName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with null componentName");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with null objectName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with null payload");
        status = EINVAL;
    }
    else if (0 > payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with negative payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            firewall->Set(componentName, objectName, payload, payloadSizeBytes);
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiSet exception occurred: %s", e.what());
            status = EINTR;
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
    int status = MMI_OK;
    FirewallObject* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(FirewallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(FirewallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    } };

    if (nullptr == clientSession)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else if (nullptr == componentName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null componentName");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null objectName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            session = reinterpret_cast<FirewallObject*>(clientSession);
            session->Get(componentName, objectName, payload, payloadSizeBytes);
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet exception occurred: %s", e.what());
            status = EINTR;
        }
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (!payload)
    {
        return;
    }
    delete[] payload;
}