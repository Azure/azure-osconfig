// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>

#include "CommonUtils.h"
#include "Mmi.h"
#include "AptInstall.h"

static const std::string g_componentName = "AptInstall";
static const std::string g_desiredObjectName = "DesiredPackages";
static const std::string g_reportedObjectName = "State";
static const std::string g_stateId = "StateId";
static const std::string g_packages = "Packages";
// static const std::string g_sources = "Sources";

constexpr const char* g_commandExecuteUpdate = "sudo apt-get install $value -y --allow-downgrades --auto-remove";
constexpr const char ret[] = R""""({
    "Name": "AptInstall Module",
    "Description": "Module designed to install DEB-packages using APT",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["AptInstall"],
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

    // Validate payload size.
    int maxPayloadSizeBytes = static_cast<int>(GetMaxPayloadSizeBytes());
    if ((0 != maxPayloadSizeBytes) && (payloadSizeBytes > maxPayloadSizeBytes))
    {
        OsConfigLogError(AptInstallLog::Get(), "%s %s payload too large. Max payload expected %d, actual payload size %d", componentName, objectName, maxPayloadSizeBytes, payloadSizeBytes);
        return E2BIG;
    }

    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(AptInstallLog::Get(), "Unabled to parse JSON payload: %s", payload);
        status = EINVAL;
    }
    else
    {
        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_desiredObjectName.compare(objectName))
            {
                if (document.IsObject())
                {
                    AptInstall::DesiredPackages desiredPackages;
                    if (0 == DeserializeDesiredPackages(document, desiredPackages))
                    {
                        m_stateId = desiredPackages.stateId;
                        status = AptInstall::ExecuteUpdates(desiredPackages.packages);
                    }
                    else
                    {
                        OsConfigLogError(AptInstallLog::Get(), "Failed to deserialize %s", g_desiredObjectName.c_str());
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(AptInstallLog::Get(), "JSON payload is not a %s object", g_desiredObjectName.c_str());
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

        if (0 == g_componentName.compare(componentName))
        {
            if (0 == g_reportedObjectName.compare(objectName))
            {
                State reportedState;
                reportedState.stateId = m_stateId;
                status = SerializeState(reportedState, payload, payloadSizeBytes, maxPayloadSizeBytes);
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

unsigned int AptInstall::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

int AptInstall::DeserializeDesiredPackages(rapidjson::Document &document, DesiredPackages &object)
{
    int status = 0;

    if (document.HasMember(g_stateId.c_str()))
    {
        if (document[g_stateId.c_str()].IsString())
        {
            object.stateId = document[g_stateId.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(AptInstallLog::Get(), "%s is not a string", g_stateId.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(AptInstallLog::Get(), "JSON object does not contain a string setting");
        status = EINVAL;
    }

    // Deserialize a map of strings to strings
    // if (document.HasMember(g_sources.c_str()))
    // {
    //     if (document[g_sources.c_str()].IsObject())
    //     {
    //         for (auto &member : document[g_sources.c_str()].GetObject())
    //         {
    //             if (member.value.IsString())
    //             {
    //                 object.packages[member.name.GetString()] = member.value.GetString();
    //             }
    //             else
    //             {
    //                 OsConfigLogError(AptInstallLog::Get(), "Invalid string in JSON object string map at key %s", member.name.GetString());
    //                 status = EINVAL;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         OsConfigLogError(AptInstallLog::Get(), "%s is not an object", g_sources.c_str());
    //         status = EINVAL;
    //     }
    // }
    // else
    // {
    //     OsConfigLogError(AptInstallLog::Get(), "JSON object does not contain a string map setting");
    //     status = EINVAL;
    // }

    if (document.HasMember(g_packages.c_str()))
    {
        if (document[g_packages.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_packages.c_str()].Size(); ++i)
            {
                if (document[g_packages.c_str()][i].IsString())
                {
                    object.packages.push_back(document[g_packages.c_str()][i].GetString());
                }
                else
                {
                    OsConfigLogError(AptInstallLog::Get(), "Invalid string in JSON object string array at position %d", i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(AptInstallLog::Get(), "%s is not an array", g_packages.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(AptInstallLog::Get(), "JSON object does not contain a string array setting");
        status = EINVAL;
    }

    return status;
}


int AptInstall::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

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
            delete[] *payload;
            *payload = nullptr;
        }

        if (nullptr != payloadSizeBytes)
        {
            *payloadSizeBytes = 0;
        }
    }

    return status;
}


int AptInstall::RunCommand(const char* command, bool replaceEol)
{
    char* buffer = nullptr;
    int status = ExecuteCommand(nullptr, command, replaceEol, true, 0, 0, &buffer, nullptr, AptInstallLog::Get());

    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(AptInstallLog::Get(), "RunCommand failed with status: %d and output '%s'", status, buffer);
    }

    if (buffer)
    {
        free(buffer);
    }
    return status;
}

int AptInstall::ExecuteUpdate(const std::string &value)
{
    std::string command = std::regex_replace(g_commandExecuteUpdate, std::regex("\\$value"), value);

    int status = RunCommand(command.c_str(), true);
    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(AptInstallLog::Get(), "ExecuteUpdate failed with status %d and arguments '%s'", status, value.c_str());
    }
    return status;
}

int AptInstall::ExecuteUpdates(const std::vector<std::string> packages)
{
    int status = MMI_OK;
    for (std::string package : packages)
    {
        OsConfigLogInfo(AptInstallLog::Get(), "Starting to update package(s): %s", package.c_str());
        status = ExecuteUpdate(package);
        if (status != MMI_OK)
        {
            return status;
        }
    }
    return status;
}

int AptInstall::SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key(g_stateId.c_str());
    writer.String(reportedState.stateId.c_str());
    writer.EndObject();

    unsigned int bufferSize = buffer.GetSize();

    if ((0 != maxPayloadSizeBytes) && (bufferSize > maxPayloadSizeBytes))
    {
        OsConfigLogError(AptInstallLog::Get(), "Failed to serialize object %s. Max payload expected %d, actual payload size %d", g_reportedObjectName.c_str(), maxPayloadSizeBytes, bufferSize);
        status = E2BIG;
    }
    else
    {
        status = AptInstall::CopyJsonPayload(buffer, payload, payloadSizeBytes);
        if(0 != status)
        {
            OsConfigLogError(AptInstallLog::Get(), "Failed to serialize object %s", g_reportedObjectName.c_str());
        }
    }
    
    return status;
}