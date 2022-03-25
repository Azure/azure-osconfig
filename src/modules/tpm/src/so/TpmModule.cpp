// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <map>
#include <memory>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <Tpm.h>
#include <ScopeGuard.h>

void __attribute__((constructor)) InitModule()
{
    TpmLog::OpenLog();
    OsConfigLogInfo(TpmLog::Get(), "Tpm module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(TpmLog::Get(), "Tpm module unloaded");
    TpmLog::CloseLog();
}

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Tpm",
    "Description": "Provides functionality to remotely query the TPM on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Tpm"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

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
                OsConfigLogError(TpmLog::Get(), "MmiGetInfo(%s, %.*s, %d) invalid arguments",
                    clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0));
            }

            status = EINVAL;
        }
        else
        {
            std::size_t infoLength = sizeof(g_moduleInfo) - 1;
            *payloadSizeBytes = infoLength;
            *payload = new char[infoLength];
            if (nullptr == *payload)
            {
                OsConfigLogError(TpmLog::Get(), "MmiGetInfo failed to allocate %d bytes for payload", (int)infoLength);
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, g_moduleInfo, infoLength);
            }
        }

        ScopeGuard sg{[&]()
        {
            if ((MMI_OK == status) && (nullptr != payload) && (nullptr != payloadSizeBytes))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(TpmLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
                else
                {
                    OsConfigLogInfo(TpmLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
                }
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
                else
                {
                    OsConfigLogError(TpmLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
                }
            }
        }};

        return status;
    }
    catch (const std::exception &e)
    {
        OsConfigLogError(TpmLog::Get(), "MmiGetInfo exception occurred");
        return ENODATA;
    }
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    try
    {
        int status = MMI_OK;
        MMI_HANDLE handle = nullptr;

        if (nullptr == clientName)
        {
            OsConfigLogError(TpmLog::Get(), "MmiOpen(%s, %u) invalid arguments", clientName, maxPayloadSizeBytes);
            status = EINVAL;
        }
        else
        {
            Tpm* tpm = new (std::nothrow) Tpm(maxPayloadSizeBytes);
            if (!tpm)
            {
                OsConfigLogError(TpmLog::Get(), "MmiOpen Tpm construction failed");
                status = ENODATA;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(tpm);
            }
        }

        ScopeGuard sg{[&]()
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(TpmLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
            else
            {
                OsConfigLogError(TpmLog::Get(), "MmiOpen(%s) returned: %p, status: %d", clientName, handle, status);
            }
        }};

        return handle;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(TpmLog::Get(), "MmiOpen exception occurred");
        return nullptr;
    }
}

void MmiClose(MMI_HANDLE clientSession)
{
    try
    {
        if (clientSession != nullptr)
        {
            Tpm* tpm = reinterpret_cast<Tpm*>(clientSession);
            delete tpm;
        }
        else
        {
            OsConfigLogError(TpmLog::Get(), "MmiClose invalid MMI_HANDLE: %p", clientSession);
        }
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(TpmLog::Get(), "MmiClose exception occurred");
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
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) clientSession %p null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), clientSession);
            }
        }
        else if ((nullptr == componentName) || (0 != std::strcmp(componentName, TPM)))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) componentName %s is invalid, %s is expected",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), componentName, TPM);
            }
        }
        else if ((nullptr == objectName) ||
                 ((0 != std::strcmp(objectName, TPM_STATUS)) &&
                 (0 != std::strcmp(objectName, TPM_VERSION)) &&
                 (0 != std::strcmp(objectName, TPM_MANUFACTURER))))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) objectName %s is invalid, %s, %s, or %s is expected",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), objectName, TPM_STATUS, TPM_VERSION, TPM_MANUFACTURER);
            }
        }
        else if (nullptr == payload)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) payload %.*s is null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0), *payload);
            }
        }
        else if (nullptr == payloadSizeBytes)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) payloadSizeBytes %d is null",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), (payloadSizeBytes ? *payloadSizeBytes : 0));
            }
        }
        else
        {
            Tpm* tpm = reinterpret_cast<Tpm*>(clientSession);

            if (nullptr != tpm)
            {
                status = tpm->Get(objectName, payload, payloadSizeBytes);
            }
        }

        ScopeGuard sg{[&]()
        {
            if ((MMI_OK == status) && (nullptr != payload) && (nullptr != payloadSizeBytes))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d",
                        clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
                }
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d",
                    clientSession, componentName, objectName, (payloadSizeBytes ? *payloadSizeBytes : 0), *payload, (payloadSizeBytes ? *payloadSizeBytes : 0), status);
            }
        }};

        return status;
    }
    catch(const std::exception& e)
    {
        OsConfigLogError(TpmLog::Get(), "MmiGet exception occurred");
        return ENODATA;
    }
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (payload)
    {
        delete[] payload;
    }
}