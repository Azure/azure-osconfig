// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Firewall.h>
#include <ScopeGuard.h>
#include <Mmi.h>

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
    }};

    try
    {
        status = Firewall::GetInfo(clientName, payload, payloadSizeBytes);
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGetInfo exception occured: %s", e.what());
        status = EINTR;
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
        Firewall* session = new (std::nothrow) Firewall(maxPayloadSizeBytes);

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
        Firewall* session = reinterpret_cast<Firewall*>(clientSession);
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
    Firewall* session = reinterpret_cast<Firewall*>(clientSession);

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
    }};

    if (session)
    {
        try
        {
            status = session->Set(componentName, objectName, payload, payloadSizeBytes);
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiSet exception occurred: %s", e.what());
            status = EINTR;
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
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
    Firewall* session = reinterpret_cast<Firewall*>(clientSession);

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
    }};

    if (session)
    {
        try
        {
            session = reinterpret_cast<Firewall*>(clientSession);
            status = session->Get(componentName, objectName, payload, payloadSizeBytes);
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(FirewallLog::Get(), "MmiGet exception occurred: %s", e.what());
            status = EINTR;
        }
    }
    else
    {
        OsConfigLogError(FirewallLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
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
