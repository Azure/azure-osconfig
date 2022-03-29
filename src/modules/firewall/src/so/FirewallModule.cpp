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

const char g_componentName[] = "Firewall";
const char g_firewallState[] = "firewallState";
const char g_firewallFingerprint[] = "firewallFingerprint";

using namespace std;

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

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Firewall",
    "Description": "Provides functionality to remotely manage firewall rules on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Firewall"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

int MmiGetInfoInternal(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    ScopeGuard sg{[&]()
    {
        if (status == MMI_OK)
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
    }};

    if (nullptr == clientName || nullptr == payload || nullptr == payloadSizeBytes)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo called with invalid arguments");
        status = EINVAL;
        return status;
    }

    std::size_t len = sizeof(g_moduleInfo) - 1;

    *payloadSizeBytes = len;
    *payload = new char[len];
    std::memcpy(*payload, g_moduleInfo, len);

    return status;
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    try
    {
        return MmiGetInfoInternal(clientName, payload, payloadSizeBytes);
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo exception occurred");
        return EFAULT;
    }
}

MMI_HANDLE MmiOpenInternal(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    MMI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(FirewallLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
        }
        else
        {
            OsConfigLogError(FirewallLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiOpen called without a clientName");
        status = EINVAL;
    }
    else
    {
        FirewallObject* firewall = new (nothrow) FirewallObject(maxPayloadSizeBytes);
        if (!firewall)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiOpen memory allocation failed");
            status = ENOMEM;
        }
        else
        {
            handle = reinterpret_cast<MMI_HANDLE>(firewall);
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
        OsConfigLogError(FirewallLog::Get(), "MmiOpen exception occurred");
        return nullptr;
    }
}

void MmiCloseInternal(MMI_HANDLE clientSession)
{
    if (clientSession != nullptr)
    {
        FirewallObject* firewall = reinterpret_cast<FirewallObject*>(clientSession);
        delete firewall;
        clientSession = nullptr;
    }
}

void MmiClose(MMI_HANDLE clientSession)
{
    try
    {
        return MmiCloseInternal(clientSession);
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiClose exception occurred");
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
        FirewallObject* firewall = reinterpret_cast<FirewallObject*>(clientSession);
        return firewall->Set(clientSession, componentName, objectName, payload, payloadSizeBytes);
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet exception occurred");
        return EFAULT;
    }
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;
    if ((clientSession == nullptr)
        || (componentName == nullptr)
        || (objectName == nullptr)
        || (payload == nullptr)
        || (payloadSizeBytes == nullptr))
    {
        status = EINVAL;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d, null argument", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
    }
    else if (strcmp(componentName, g_componentName) != 0)
    {
        status = EINVAL;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d, component name is invalid", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }

    }
    else if ((strcmp(objectName, g_firewallState) != 0) && (strcmp(objectName, g_firewallFingerprint) != 0))
    {
        status = EINVAL;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d, object name is invalid", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
    }
    else
    {
        try
        {
            FirewallObject* firewall = reinterpret_cast<FirewallObject*>(clientSession);
            status = firewall->Get(clientSession, componentName, objectName, payload, payloadSizeBytes);
        }
        catch(const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet exception occurred");
            status = EFAULT;
        }
    }

    return status;
}

void MmiFreeInternal(MMI_JSON_STRING payload)
{
    if (!payload)
    {
        return;
    }
    delete[] payload;
}

void MmiFree(MMI_JSON_STRING payload)
{
    try
    {
        return MmiFreeInternal(payload);
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiFree exception occurred");
    }
}