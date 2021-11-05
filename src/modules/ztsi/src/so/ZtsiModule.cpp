// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>

#include <ScopeGuard.h>
#include <Mmi.h>
#include <Ztsi.h>

static const std::string g_ztsiConfigFile = "/etc/ztsi/config.json";

static const std::string g_componentName = "Ztsi";
static const std::string g_desiredServiceUrl = "DesiredServiceUrl";
static const std::string g_desiredEnabled = "DesiredEnabled";
static const std::string g_reportedServiceUrl = "ServiceUrl";
static const std::string g_reportedEnabled = "Enabled";

void __attribute__((constructor)) InitModule()
{
    ZtsiLog::OpenLog();
    OsConfigLogInfo(ZtsiLog::Get(), "Ztsi module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(ZtsiLog::Get(), "Ztsi module unloaded");
    ZtsiLog::CloseLog();
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    constexpr const char ret[] = R""""({
        "Name": "Ztsi",
        "Description": "Provides functionality to remotely configure the ZTSI Agent on device",
        "Manufacturer": "Microsoft",
        "VersionMajor": 1,
        "VersionMinor": 0,
        "VersionInfo": "Nickel",
        "Components": ["Ztsi"],
        "Lifetime": 1,
        "UserAccount": 0})"""";

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (!IsValidClientName(clientName))
    {
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
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
                OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo failed to allocate memory");
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
            OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
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
            OsConfigLogInfo(ZtsiLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }
    else if (!IsValidClientName(clientName))
    {
        status = EINVAL;
    }
    else
    {
        try
        {
            Ztsi* ztsi = new (std::nothrow) Ztsi(g_ztsiConfigFile, maxPayloadSizeBytes);
            if (nullptr == ztsi)
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiOpen failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(ztsi);
            }
        }
        catch (std::exception& e)
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiOpen exception thrown: %s", e.what());
            status = EINTR;
        }
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    Ztsi* ztsi = reinterpret_cast<Ztsi*>(clientSession);
    if (nullptr != ztsi)
    {
        delete ztsi;
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
    Ztsi* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
        return status;
    }

    rapidjson::Document document;
    if (document.Parse(payload).HasParseError())
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiSet unabled to parse JSON payload");
        status = EINVAL;
        return status;
    }

    session = reinterpret_cast<Ztsi*>(clientSession);

    if (0 == g_componentName.compare(componentName))
    {
        if (0 == g_desiredEnabled.compare(objectName))
        {
            if (document.IsBool())
            {
                status = session->SetEnabled(document.GetBool());
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet %s is not of type boolean", g_desiredEnabled.c_str());
                status = EINVAL;
            }
        }
        else if (0 == g_desiredServiceUrl.compare(objectName))
        {
            if (document.IsString())
            {
                status = session->SetServiceUrl(document.GetString());
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet %s is not of type string", g_desiredServiceUrl.c_str());
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiSet called with invalid objectName: %s", objectName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiSet called with invalid componentName: %s", componentName);
        status = EINVAL;
    }

    return status;
}

int SerializeJsonObject(MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, rapidjson::Document& document)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    if (buffer.GetSize() > maxPayloadSizeBytes)
    {
        OsConfigLogError(ZtsiLog::Get(), "SerializeJsonObject failed to serialize JSON object to buffer");
        status = E2BIG;
        return status;
    }

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(ZtsiLog::Get(), "SerializeJsonPayload unable to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::fill(*payload, *payload + buffer.GetSize(), 0);
            std::memcpy(*payload, buffer.GetString(), buffer.GetSize());
            *payloadSizeBytes = buffer.GetSize();
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(ZtsiLog::Get(), "Could not allocate payload: %s", e.what());
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

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
        return status;
    }

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGet called with null payloadSizeBytes");
        status = EINVAL;
        return status;
    }

    *payload = nullptr;
    *payloadSizeBytes = 0;

    Ztsi* session = reinterpret_cast<Ztsi*>(clientSession);

    unsigned int maxPayloadSizeBytes = session->GetMaxPayloadSizeBytes();
    rapidjson::Document document;
    if (0 == g_componentName.compare(componentName))
    {
        if (0 == g_reportedEnabled.compare(objectName))
        {
            Ztsi::EnabledState enabledState = session->GetEnabledState();

            document.SetInt(static_cast<int>(enabledState));
            status = SerializeJsonObject(payload, payloadSizeBytes, maxPayloadSizeBytes, document);
        }
        else if (0 == g_reportedServiceUrl.compare(objectName))
        {
            std::string serviceUrl = session->GetServiceUrl();

            document.SetString(serviceUrl.c_str(), document.GetAllocator());
            status = SerializeJsonObject(payload, payloadSizeBytes, maxPayloadSizeBytes, document);
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiGet called with invalid objectName: %s", objectName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGet called with invalid componentName: %s", componentName);
        status = EINVAL;
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