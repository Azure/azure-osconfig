// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <Sample.h>
#include <ScopeGuard.h>
#include <Mmi.h>

static const std::string g_componentName = "SampleComponent";
static const std::string g_objectName = "SampleObject";

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
        return status;
    }

    rapidjson::Document document;
    if (document.Parse(payload).HasParseError())
    {
        OsConfigLogError(SampleLog::Get(), "MmiSet unabled to parse JSON payload");
        status = EINVAL;
        return status;
    }

    session = reinterpret_cast<Sample*>(clientSession);

    // Dispatch the request to the appropriate handler for the given component and object
    if (0 == g_componentName.compare(componentName))
    {
        if (0 == g_objectName.compare(objectName))
        {
            // Get the required data from the payload and dispatch the request to the client session
            if (document.IsString())
            {
                status = session->SetValue(document.GetString());
            }
            else
            {
                OsConfigLogError(SampleLog::Get(), "MmiSet JSON payload is not a string");
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "MmiSet called with invalid objectName: %s", objectName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "MmiSet called with invalid componentName: %s", componentName);
        status = EINVAL;
    }

    return status;
}

// A helper method for serializing a JSON document to a payload string
int SerializeJsonPayload(rapidjson::Document& document, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    if (buffer.GetSize() > maxPayloadSizeBytes)
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
        return status;
    }

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(SampleLog::Get(), "MmiGet called with null payloadSizeBytes");
        status = EINVAL;
        return status;
    }

    *payload = nullptr;
    *payloadSizeBytes = 0;

    Sample* session = reinterpret_cast<Sample*>(clientSession);

    unsigned int maxPayloadSizeBytes = session->GetMaxPayloadSizeBytes();
    rapidjson::Document document;

    // Dispatch the get request to the appropriate handler for the given component and object
    if (0 == g_componentName.compare(componentName))
    {
        if (0 == g_objectName.compare(objectName))
        {
            std::string value = session->GetValue();
            document.SetString(value.c_str(), document.GetAllocator());

            // Serialize the JSON object to the payload buffer
            status = SerializeJsonPayload(document, payload, payloadSizeBytes, maxPayloadSizeBytes);
        }
        else
        {
            OsConfigLogError(SampleLog::Get(), "MmiGet called with invalid objectName: %s", objectName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(SampleLog::Get(), "MmiGet called with invalid componentName: %s", componentName);
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