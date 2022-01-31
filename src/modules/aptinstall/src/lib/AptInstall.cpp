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
static const std::string g_packages = "Packages";
// static const std::string g_sources = "Sources";
static const std::string g_executionState = "ExecutionState";
static const std::string g_packagesFingerprint = "PackagesFingerprint";
static const std::string g_commandGetInstalledPackagesHash = "dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64";
static const std::string g_commandAptUpdate = "sudo apt-get update";

constexpr const char* g_commandExecuteUpdate = "sudo apt-get install $value -y --allow-downgrades --auto-remove";
constexpr const char* g_commandGetInstalledPackageVersion = "apt-cache policy $value | grep Installed";
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

AptInstallBase::AptInstallBase(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

AptInstall::AptInstall(unsigned int maxPayloadSizeBytes)
    : AptInstallBase(maxPayloadSizeBytes)
{
}

int AptInstallBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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

int AptInstallBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
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
                    AptInstallBase::DesiredPackages desiredPackages;
                    if (0 == DeserializeDesiredPackages(document, desiredPackages))
                    {
                        m_desiredPackages = GetPackagesNames(desiredPackages.packages);
                        m_executionState = ExecutionState::Running;
                        status = AptInstallBase::ExecuteUpdates(desiredPackages.packages);
                        m_executionState = AptInstallBase::GetStateFromStatusCode(status);
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

int AptInstallBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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
                reportedState.executionState = m_executionState;
                reportedState.packagesFingerprint = GetFingerprint();
                reportedState.packages = GetReportedPackages(m_desiredPackages);
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

unsigned int AptInstallBase::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

int AptInstallBase::DeserializeDesiredPackages(rapidjson::Document& document, DesiredPackages& object)
{
    int status = 0;

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

int AptInstallBase::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
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

int AptInstallBase::ExecuteUpdate(const std::string &value)
{
    std::string command = std::regex_replace(g_commandExecuteUpdate, std::regex("\\$value"), value);

    int status = RunCommand(command.c_str(), true, nullptr, 600);
    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(AptInstallLog::Get(), "ExecuteUpdate failed with status %d and arguments '%s'", status, value.c_str());
    }
    return status;
}

int AptInstallBase::ExecuteUpdates(const std::vector<std::string> packages)
{
    int status = MMI_OK;

    status = RunCommand(g_commandAptUpdate.c_str(), true, nullptr, 0);
    if (status != MMI_OK)
    {
        return status;
    }

    for (std::string package : packages)
    {
        OsConfigLogInfo(AptInstallLog::Get(), "Starting to update package(s): %s", package.c_str());
        status = ExecuteUpdate(package);
        if (status != MMI_OK)
        {
            OsConfigLogError(AptInstallLog::Get(), "Failed to update package(s): %s", package.c_str());
            return status;
        }
        OsConfigLogInfo(AptInstallLog::Get(), "Successfully updated package(s): %s", package.c_str());
    }
    return status;
}

int AptInstallBase::SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key(g_packagesFingerprint.c_str());
    writer.String(reportedState.packagesFingerprint.c_str());

    writer.Key(g_packages.c_str());
    writer.StartObject();
    for (auto& pair : reportedState.packages)
    {
        writer.Key(pair.first.c_str());
        writer.String(pair.second.c_str());
    }
    writer.EndObject();

    writer.Key(g_executionState.c_str());
    writer.Int(static_cast<int>(reportedState.executionState));

    writer.EndObject();

    unsigned int bufferSize = buffer.GetSize();

    if ((0 != maxPayloadSizeBytes) && (bufferSize > maxPayloadSizeBytes))
    {
        OsConfigLogError(AptInstallLog::Get(), "Failed to serialize object %s. Max payload expected %d, actual payload size %d", g_reportedObjectName.c_str(), maxPayloadSizeBytes, bufferSize);
        status = E2BIG;
    }
    else
    {
        status = AptInstallBase::CopyJsonPayload(buffer, payload, payloadSizeBytes);
        if(0 != status)
        {
            OsConfigLogError(AptInstallLog::Get(), "Failed to serialize object %s", g_reportedObjectName.c_str());
        }
    }
    
    return status;
}

int AptInstall::RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds)
{
    char* buffer = nullptr;
    int status = ExecuteCommand(nullptr, command, replaceEol, true, 0, timeoutSeconds, &buffer, nullptr, AptInstallLog::Get());

    if (status == MMI_OK)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(AptInstallLog::Get(), "RunCommand failed with status: %d and output '%s'", status, buffer);
    }

    if (buffer)
    {
        free(buffer);
    }
    return status;
}

AptInstallBase::ExecutionState AptInstallBase::GetStateFromStatusCode(int status)
{
    ExecutionState state = ExecutionState::Unknown;
    switch (status)
    {
        case EXIT_SUCCESS:
            state = ExecutionState::Succeeded;
            break;
        case ETIME:
            state = ExecutionState::TimedOut;
            break;
        default:
            state = ExecutionState::Failed;
    }
    return state;
}

std::string AptInstallBase::GetFingerprint()
{
    std::string hashString = "";
    RunCommand(g_commandGetInstalledPackagesHash.c_str(), true, &hashString, 0);
    return hashString;
}

std::vector<std::string> AptInstallBase::GetPackagesNames(std::vector<std::string> packages)
{
    std::vector<std::string> packagesNames;
    for (auto& packagesLine : packages)
    {
        std::vector<std::string> result = Split(packagesLine, " ");
        for (auto& element : result)
            {
                std::string packageName = Split(element, "=")[0];
                packagesNames.push_back(TrimEnd(packageName, "-"));
            }
    }
    return packagesNames;
}

std::vector<std::string> AptInstallBase::Split(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delimiter, end)) != std::string::npos)
    {
        end = str.find(delimiter, start);
        result.push_back(str.substr(start, end - start));
    }
    return result;
}

std::map<std::string, std::string> AptInstallBase::GetReportedPackages(std::vector<std::string> packages)
{
    std::map<std::string, std::string> result;
    int status;
    for (auto& packageName : packages)
    {
        std::string command = std::regex_replace(g_commandGetInstalledPackageVersion, std::regex("\\$value"), packageName);

        std::string rawVersion = "";
        status = RunCommand(command.c_str(), true, &rawVersion, 0);
        if (status != MMI_OK && IsFullLoggingEnabled())
        {
            OsConfigLogError(AptInstallLog::Get(), "Get the installed version of package %s failed with status %d", packageName.c_str(), status);
        }

        std::string version = rawVersion != "" ? Split(rawVersion, ":")[1] : "(failed)";
        result[packageName] = Trim(version, " ");
    }

  return result;
}

std::string AptInstallBase::TrimStart(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_first_not_of(trim);
    if (pos == std::string::npos)
    {
        return str;
    }
    return str.substr(pos);
}

std::string AptInstallBase::TrimEnd(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_last_not_of(trim);
    if (pos == std::string::npos)
    {
        return str;
    }
    return str.substr(0, pos + 1);
}

std::string AptInstallBase::Trim(const std::string& str, const std::string& trim)
{
    return TrimStart(TrimEnd(str, trim), trim);
}