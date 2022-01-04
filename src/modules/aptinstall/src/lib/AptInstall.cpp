// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "CommonUtils.h"
#include "Mmi.h"
#include "AptInstall.h"

static const std::string g_componentName = "AptInstallComponent";
static const std::string g_objectName = "AptInstallObject";

constexpr const char ret[] = R""""({
    "Name": "AptInstall Module,
    "Description": "Module designed to install DEB-packages using APT",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["AptInstallComponent"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE AptInstallLog::m_log = nullptr;

AptInstall::AptInstall(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int AptInstall::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
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
                OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo failed to allocate memory");
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
            OsConfigLogError(AptInstallLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
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

int AptInstall::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(AptInstallLog::Get(), "Unabled to parse JSON payload: %s", payload);
        status = EINVAL;
    }
    else
    {
        // Dispatch the request to the appropriate method for the given component and object
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_objectName.compare(objectName))
            {
                // Get the required data from the payload and dispatch the request to the client session
                if (document.IsString())
                {
                    // Apply the payload data
                    m_value = document.GetString();
                    status = MMI_OK;
                }
                else
                {
                    OsConfigLogError(AptInstallLog::Get(), "JSON payload is not a string");
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(AptInstallLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(AptInstallLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int AptInstall::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(AptInstallLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        unsigned int maxPayloadSizeBytes = GetMaxPayloadSizeBytes();
        rapidjson::Document document;

        // Dispatch the get request to the appropriate method for the given component and object
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_objectName.compare(objectName))
            {
                std::string value = m_value;
                document.SetString(value.c_str(), document.GetAllocator());

                // Serialize the JSON object to the payload buffer
                status = AptInstall::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else
            {
                OsConfigLogError(AptInstallLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(AptInstallLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

// A helper method for serializing a JSON document to a payload string
int AptInstall::SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    if ((0 != maxPayloadSizeBytes) && (buffer.GetSize() > maxPayloadSizeBytes))
    {
        OsConfigLogError(AptInstallLog::Get(), "Failed to serialize JSON object to buffer");
        status = E2BIG;
    }
    else
    {
        try
        {
            *payload = new (std::nothrow) char[buffer.GetSize()];
            if (nullptr == *payload)
            {
                OsConfigLogError(AptInstallLog::Get(), "Unable to allocate memory for payload");
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
            OsConfigLogError(AptInstallLog::Get(), "Could not allocate payload: %s", e.what());
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

unsigned int AptInstall::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}