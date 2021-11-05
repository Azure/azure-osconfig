// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <map>
#include <memory>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <Networking.h>
#include <ScopeGuard.h>

void __attribute__((constructor)) InitModule()
{
    NetworkingLog::OpenLog();
    OsConfigLogInfo(NetworkingLog::Get(), "Networking module loaded");
}
void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(NetworkingLog::Get(), "Networking module unloaded");
    NetworkingLog::CloseLog();
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
                OsConfigLogError(NetworkingLog::Get(), "MmiGetInfo(%s, %.*s, %d) invalid arguments",
                    clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0));
            }

            status = EINVAL;
        }
        else
        {
            constexpr const char networkingInfo[] =
                R""""({
                "Name": "Networking",
                "Description": "Provides functionality to remotely query device networking",
                "Manufacturer": "Microsoft",
                "VersionMajor": 1,
                "VersionMinor": 0,
                "VersionInfo": "Iron",
                "Components": ["Networking"],
                "Lifetime": 1,
                "UserAccount": 0})"""";

            std::size_t infoLength = sizeof(networkingInfo) - 1;
            *payloadSizeBytes = infoLength;
            *payload = new char[infoLength];
            if (nullptr == *payload)
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGetInfo failed to allocate %d bytes for payload", (int)infoLength);
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, networkingInfo, infoLength);
            }
        }

        ScopeGuard sg{[&]()
        {
            if ((MMI_OK == status) && (nullptr != payload) && (nullptr != payloadSizeBytes))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(NetworkingLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
                else
                {
                    OsConfigLogInfo(NetworkingLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
                }
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(NetworkingLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
                else
                {
                    OsConfigLogError(NetworkingLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
            }
        }};

        return status;
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(NetworkingLog::Get(), "MmiGetInfo exception occurred");
        return ENODATA;
    }
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    try
    {
        int status = EINVAL;
        MMI_HANDLE handle = nullptr;

        ScopeGuard sg{[&]()
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(NetworkingLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
            else
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
        }};

        if (nullptr == clientName)
        {
            OsConfigLogError(NetworkingLog::Get(), "MmiOpen called without a clientName.");
        }
        else
        {
            NetworkingObject* networking = new (std::nothrow) NetworkingObject(maxPayloadSizeBytes);
            if (!networking)
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiOpen memory allocation failed");
            }
            else
            {
                status = MMI_OK;
            }

            if (nullptr != networking)
            {
                handle = reinterpret_cast<MMI_HANDLE>(networking);
            }

            return handle;
        }

        return handle;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(NetworkingLog::Get(), "MmiOpen exception occurred");
        return nullptr;
    }
}

void MmiClose(MMI_HANDLE clientSession)
{
    try
    {
        if (clientSession != nullptr)
        {
            NetworkingObject* networking = reinterpret_cast<NetworkingObject*>(clientSession);
            delete networking;
        }
        else
        {
            OsConfigLogError(NetworkingLog::Get(), "MmiClose invalid MMI_HANDLE. handle: %p", clientSession);
        }
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(NetworkingLog::Get(), "MmiClose exception occurred");
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return ENOSYS;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    try
    {
        int status = EINVAL;

        if (nullptr == clientSession)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) clientSession %p null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), clientSession);
            }
        }
        else if ((nullptr == componentName) || (0 != std::strcmp(componentName, NETWORKING)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) componentName %s is invalid, %s is expected",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), componentName, NETWORKING);
            }
        }
        else if ((nullptr == objectName) || (0 != std::strcmp(objectName, NETWORK_CONFIGURATION)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) objectName %s is invalid, %s is expected",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), objectName, NETWORK_CONFIGURATION);
            }
        }
        else if (nullptr == payload)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) payload %.*s is null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0), *payload);
            }
        }
        else if (nullptr == payloadSizeBytes)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) payloadSizeBytes %d is null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0));
            }
        }
        else
        {
            NetworkingObject* networking = reinterpret_cast<NetworkingObject*>(clientSession);

            if (nullptr != networking)
            {
                status = networking->Get(clientSession, componentName, objectName, payload, payloadSizeBytes);
            }
        }

        ScopeGuard sg{[&]()
        {
            if ((MMI_OK == status) && (nullptr != payload) && (nullptr != payloadSizeBytes))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d",
                        clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogError(NetworkingLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
            }
        }};

        return status;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(NetworkingLog::Get(), "MmiGet exception occurred");
        return ENODATA;
    }
}

void MmiFree(MMI_JSON_STRING payload)
{
    try
    {
        if (!payload)
        {
            return;
        }
        delete[] payload;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(NetworkingLog::Get(), "MmiFree exception occurred");
    }
}