// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "CommonUtils.h"
#include "Mmi.h"
#include "Sample.h"

static const std::string g_componentName = "SampleComponentName";
static const std::string g_desiredStringObjectName = "DesiredStringObjectName";
static const std::string g_reportedStringObjectName = "ReportedStringObjectName";

constexpr const char info[] = R""""({
    "Name": "C++ Sample",
    "Description": "A sample module written in C++",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["SampleComponent"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE SampleLog::m_log = nullptr;

Sample::Sample(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int Sample::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

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
            std::size_t len = ARRAY_SIZE(info) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(SampleLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, info, len);
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

int Sample::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(SampleLog::Get(), "Unabled to parse JSON payload: %s", payload);
        status = EINVAL;
    }
    else
    {
        // Dispatch the request to the appropriate method for the given component and object
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_desiredStringObjectName.compare(objectName))
            {
                // Parse the string from the payload
                if (document.IsString())
                {
                    // Apply the payload data
                    m_stringObject = document.GetString();
                    status = MMI_OK;
                }
                else
                {
                    OsConfigLogError(SampleLog::Get(), "JSON payload is not a string");
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int Sample::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SampleLog::Get(), "Invalid payloadSizeBytes");
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
            if (0 == g_reportedStringObjectName.compare(objectName))
            {
                std::string value = m_stringObject;
                document.SetString(value.c_str(), document.GetAllocator());

                // Serialize the JSON object to the payload buffer
                status = Sample::SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

// A helper method for serializing a JSON document to a payload string
int Sample::SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    if ((0 != maxPayloadSizeBytes) && (buffer.GetSize() > maxPayloadSizeBytes))
    {
        OsConfigLogError(SampleLog::Get(), "Failed to serialize JSON object to buffer");
        status = E2BIG;
    }
    else
    {
        try
        {
            *payload = new (std::nothrow) char[buffer.GetSize()];
            if (nullptr == *payload)
            {
                OsConfigLogError(SampleLog::Get(), "Unable to allocate memory for payload");
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
            OsConfigLogError(SampleLog::Get(), "Could not allocate payload: %s", e.what());
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

unsigned int Sample::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}