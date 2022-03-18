// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>

#include <PackageManagerConfiguration.h>
#include <ScopeGuard.h>
#include <Mmi.h>

void __attribute__((constructor)) InitModule()
{
    PackageManagerConfigurationLog::OpenLog();
    OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "C++ PackageManagerConfiguration module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "C++ PackageManagerConfiguration module unloaded");
    PackageManagerConfigurationLog::CloseLog();
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
                OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    // Get the static information from the PackageManagerConfiguration module
    status = PackageManagerConfiguration::GetInfo(clientName, payload, payloadSizeBytes);

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
            OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        try
        {
            // Create an instance of the PackageManagerConfiguration class to be returned as an MMI_HANDLE for each client session
            PackageManagerConfiguration* session = new (std::nothrow) PackageManagerConfiguration(maxPayloadSizeBytes);
            if (nullptr == session)
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiOpen failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(session);
            }
        }
        catch (std::exception& e)
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiOpen exception thrown: %s", e.what());
            status = EINTR;
        }
    }
    else
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    PackageManagerConfiguration* session = reinterpret_cast<PackageManagerConfiguration*>(clientSession);
    if (nullptr != session)
    {
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
    PackageManagerConfiguration* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        // Cast the MMI_HANDLE to an instance of PackageManagerConfiguration class session and call Set()
        session = reinterpret_cast<PackageManagerConfiguration*>(clientSession);
        status = session->Set(componentName, objectName, payload, payloadSizeBytes);
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
    PackageManagerConfiguration* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        // Cast the MMI_HANDLE to an instance of PackageManagerConfiguration class session and call Get()
        session = reinterpret_cast<PackageManagerConfiguration*>(clientSession);
        status = session->Get(componentName, objectName, payload, payloadSizeBytes);
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}