// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>

#include <AptInstall.h>
#include <ScopeGuard.h>
#include <Mmi.h>

void __attribute__((constructor)) InitModule()
{
    AptInstallLog::OpenLog();
    OsConfigLogInfo(AptInstallLog::Get(), "C++ AptInstall module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(AptInstallLog::Get(), "C++ AptInstall module unloaded");
    AptInstallLog::CloseLog();
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
                OsConfigLogInfo(AptInstallLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(AptInstallLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    // Get the static information from the AptInstall module
    status = AptInstall::GetInfo(clientName, payload, payloadSizeBytes);

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
            OsConfigLogInfo(AptInstallLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(AptInstallLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        try
        {
            // Create an instance of the AptInstall class to be returned as an MMI_HANDLE for each client session
            AptInstall* session = new (std::nothrow) AptInstall(maxPayloadSizeBytes);
            if (nullptr == session)
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiOpen failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(session);
            }
        }
        catch (std::exception& e)
        {
            OsConfigLogError(AptInstallLog::Get(), "MmiOpen exception thrown: %s", e.what());
            status = EINTR;
        }
    }
    else
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    AptInstall* session = reinterpret_cast<AptInstall*>(clientSession);
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
    AptInstall* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(AptInstallLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        // Cast the MMI_HANDLE to an instance of AptInstall class session and call Set()
        session = reinterpret_cast<AptInstall*>(clientSession);
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
    AptInstall* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(AptInstallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(AptInstallLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        // Cast the MMI_HANDLE to an instance of AptInstall class session and call Get()
        session = reinterpret_cast<AptInstall*>(clientSession);
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