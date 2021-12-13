// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>

#include <Sample.h>
#include <ScopeGuard.h>
#include <Mmi.h>

void __attribute__((constructor)) InitModule()
{
    SampleLog::OpenLog();
    OsConfigLogInfo(SampleLog::Get(), "C++ Sample module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(SampleLog::Get(), "C++ Sample module unloaded");
    SampleLog::CloseLog();
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    constexpr const char ret[] = R""""({
        "Name": "C++ Sample",
        "Description": "A sample module written in C++",
        "Manufacturer": "Microsoft",
        "VersionMajor": 1,
        "VersionMinor": 0,
        "VersionInfo": "",
        "Components": ["SampleComponent"],
        "Lifetime": 1,
        "UserAccount": 0})"""";

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(SampleLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(SampleLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SampleLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            std::size_t len = ARRAY_SIZE(ret) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(SampleLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, ret, len);
                *payloadSizeBytes = len;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(SampleLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
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
            OsConfigLogInfo(SampleLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr != clientName)
    {
        try
        {
            // Create an instance of the Sample class to be returned as an MMI_HANDLE for each client session
            Sample* session = new (std::nothrow) Sample(maxPayloadSizeBytes);
            if (nullptr == session)
            {
                OsConfigLogError(SampleLog::Get(), "MmiOpen failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(session);
            }
        }
        catch (std::exception& e)
        {
            OsConfigLogError(SampleLog::Get(), "MmiOpen exception thrown: %s", e.what());
            status = EINTR;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    Sample* session = reinterpret_cast<Sample*>(clientSession);
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
    Sample* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(SampleLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(SampleLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(SampleLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<Sample*>(clientSession);
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
    Sample* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(SampleLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<Sample*>(clientSession);
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