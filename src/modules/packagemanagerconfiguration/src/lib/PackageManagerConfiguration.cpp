// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>

#include "CommonUtils.h"
#include "Mmi.h"
#include "PackageManagerConfiguration.h"

static const std::string g_componentName = "PackageManagerConfiguration";
static const std::string g_reportedObjectName = "State";
static const std::string g_desiredObjectName = "DesiredState";
static const std::string g_packages = "Packages";
static const std::string g_sources = "Sources";
static const std::string g_executionState = "ExecutionState";
static const std::string g_packagesFingerprint = "PackagesFingerprint";
static const std::string g_sourcesFingerprint = "SourcesFingerprint";
static const std::string g_sourcesFilenames = "SourcesFilenames";
static const std::string g_commandGetInstalledPackagesHash = "dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64";
static const std::string g_commandAptUpdate = "sudo apt-get update";
static const std::string g_sourcesFolderPath = "/etc/apt/sources.list.d/";
static const std::string g_listExtension = ".list";

constexpr const char* g_commandExecuteUpdate = "sudo apt-get install $value -y --allow-downgrades --auto-remove";
constexpr const char* g_commandGetSourcesFingerprint = "find $value -type f -name '*.list' -exec cat {} \\; | sha256sum | head -c 64";
constexpr const char* g_commandGetInstalledPackageVersion = "apt-cache policy $value | grep Installed";
constexpr const char ret[] = R""""({
    "Name": "PackageManagerConfiguration Module",
    "Description": "Module designed to install DEB-packages using APT",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["PackageManagerConfiguration"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE PackageManagerConfigurationLog::m_log = nullptr;

PackageManagerConfigurationBase::PackageManagerConfigurationBase(unsigned int maxPayloadSizeBytes, std::string sourcesDir)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_sourcesConfigurationDir = sourcesDir;
    m_executionState = ExecutionState();
}

PackageManagerConfigurationBase::PackageManagerConfigurationBase(unsigned int maxPayloadSizeBytes)
    : PackageManagerConfigurationBase(maxPayloadSizeBytes, g_sourcesFolderPath)
{
}

PackageManagerConfiguration::PackageManagerConfiguration(unsigned int maxPayloadSizeBytes)
    : PackageManagerConfigurationBase(maxPayloadSizeBytes)
{
}

int PackageManagerConfigurationBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
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
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo failed to allocate memory");
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
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
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

int PackageManagerConfigurationBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingJsonPayload);

    int maxPayloadSizeBytes = static_cast<int>(GetMaxPayloadSizeBytes());
    if ((0 != maxPayloadSizeBytes) && (payloadSizeBytes > maxPayloadSizeBytes))
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "%s %s payload too large. Max payload expected %d, actual payload size %d", componentName, objectName, maxPayloadSizeBytes, payloadSizeBytes);
        m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingJsonPayload);
        return E2BIG;
    }

    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Unabled to parse JSON payload: %s", payload);
        m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingJsonPayload);
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
                    PackageManagerConfigurationBase::DesiredState desiredState;
                    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingDesiredState);

                    if (0 == DeserializeDesiredState(document, desiredState))
                    {
                        m_desiredPackages = GetPackagesNames(desiredState.packages);

                        if (!desiredState.sources.empty())
                        {
                            status = PackageManagerConfigurationBase::ConfigureSources(desiredState.sources);
                        }
                        
                        if (status == MMI_OK)
                        {
                            status = PackageManagerConfigurationBase::ExecuteUpdates(desiredState.packages);
                        }
                    }
                    else
                    {
                        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to deserialize %s", g_desiredObjectName.c_str());
                        m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingDesiredState);
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(PackageManagerConfigurationLog::Get(), "JSON payload is not a %s object", g_desiredObjectName.c_str());
                    m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingDesiredState);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid objectName: %s", objectName);
                m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingDesiredState);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid componentName: %s", componentName);
            m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingJsonPayload);
            status = EINVAL;
        }
    }

    return status;
}

int PackageManagerConfigurationBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid payloadSizeBytes");
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
                reportedState.executionState = m_executionState.GetReportedExecutionState();
                reportedState.packagesFingerprint = GetFingerprint();
                reportedState.packages = GetReportedPackages(m_desiredPackages);
                reportedState.sourcesFingerprint = GetSourcesFingerprint(m_sourcesConfigurationDir);
                reportedState.sourcesFilenames = GetSourcesFilenames();
                status = SerializeState(reportedState, payload, payloadSizeBytes, maxPayloadSizeBytes);
            }
            else
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

unsigned int PackageManagerConfigurationBase::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

int PackageManagerConfigurationBase::DeserializeDesiredState(rapidjson::Document& document, DesiredState& object)
{
    int status = 0;

    if (document.HasMember(g_sources.c_str()))
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingSources);
        if (document[g_sources.c_str()].IsObject())
        {
            for (auto &member : document[g_sources.c_str()].GetObject())
            {
                if (member.value.IsString())
                {
                    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingSources, member.name.GetString());
                    object.sources[member.name.GetString()] = member.value.GetString();
                }
                else
                {
                    OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid string in JSON object string map at key %s", member.name.GetString());
                    m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingSources, member.name.GetString());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "%s is not a map", g_sources.c_str());
            m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingSources);
            status = EINVAL;
        }
    }
   
    if (document.HasMember(g_packages.c_str()))
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingPackages);
        if (document[g_packages.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_packages.c_str()].Size(); ++i)
            {
                if (document[g_packages.c_str()][i].IsString())
                {
                    std::string package = document[g_packages.c_str()][i].GetString();
                    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::DeserializingPackages, package);
                    object.packages.push_back(package);
                }
                else
                {
                    OsConfigLogError(PackageManagerConfigurationLog::Get(), "Invalid string in JSON object string array at position %d", i);
                    m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingPackages, "index " + i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "%s is not an array", g_packages.c_str());
            m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingPackages);
            status = EINVAL;
        }
    }
    
    if (!document.HasMember(g_sources.c_str()) && !document.HasMember(g_packages.c_str()))
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "JSON object does not contain '%s', neither '%s'", g_sources.c_str(), g_packages.c_str());
        m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::DeserializingDesiredState);
        status = EINVAL;
    }

    return status;
}

int PackageManagerConfigurationBase::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Unable to allocate memory for payload");
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
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Could not allocate payload: %s", e.what());
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

int PackageManagerConfigurationBase::ExecuteUpdate(const std::string &value)
{
    std::string command = std::regex_replace(g_commandExecuteUpdate, std::regex("\\$value"), value);

    int status = RunCommand(command.c_str(), true, nullptr, 600);
    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "ExecuteUpdate failed with status %d and arguments '%s'", status, value.c_str());
    }
    return status;
}

int PackageManagerConfigurationBase::ExecuteUpdates(const std::vector<std::string> packages)
{
    int status = MMI_OK;
    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::UpdatingPackagesLists);

    status = RunCommand(g_commandAptUpdate.c_str(), true, nullptr, 0);
    if (status != MMI_OK)
    {
        status == ETIME ? m_executionState.SetExecutionState(StateComponent::TimedOut, SubStateComponent::UpdatingPackagesLists) 
                        : m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::UpdatingPackagesLists);
        return status;
    }

    for (std::string package : packages)
    {
        OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "Starting to update package(s): %s", package.c_str());
        m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::InstallingPackages, package);
        status = ExecuteUpdate(package);
        if (status != MMI_OK)
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to update package(s): %s", package.c_str());
            status == ETIME ? m_executionState.SetExecutionState(StateComponent::TimedOut, SubStateComponent::InstallingPackages, package) 
                            : m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::InstallingPackages, package);
            return status;
        }
        OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "Successfully updated package(s): %s", package.c_str());
    }

    m_executionState.SetExecutionState(StateComponent::Succeeded, SubStateComponent::None);
    return status;
}

int PackageManagerConfigurationBase::SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key(g_packagesFingerprint.c_str());
    writer.String(reportedState.packagesFingerprint.c_str());

    writer.Key(g_packages.c_str());
    writer.StartArray();
    for (auto& element : reportedState.packages)
    {
        writer.String(element.c_str());
    }
    writer.EndArray();

    writer.Key(g_executionState.c_str());
    writer.String(reportedState.executionState.c_str());

    writer.Key(g_sourcesFingerprint.c_str());
    writer.String(reportedState.sourcesFingerprint.c_str());

    writer.Key(g_sourcesFilenames.c_str());
    writer.StartArray();
    for (auto& element : reportedState.sourcesFilenames)
    {
        writer.String(element.c_str());
    }
    writer.EndArray();

    writer.EndObject();

    unsigned int bufferSize = buffer.GetSize();

    if ((0 != maxPayloadSizeBytes) && (bufferSize > maxPayloadSizeBytes))
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to serialize object %s. Max payload expected %d, actual payload size %d", g_reportedObjectName.c_str(), maxPayloadSizeBytes, bufferSize);
        status = E2BIG;
    }
    else
    {
        status = PackageManagerConfigurationBase::CopyJsonPayload(buffer, payload, payloadSizeBytes);
        if(0 != status)
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to serialize object %s", g_reportedObjectName.c_str());
        }
    }
    
    return status;
}

int PackageManagerConfiguration::RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds)
{
    char* buffer = nullptr;
    int status = ExecuteCommand(nullptr, command, replaceEol, true, 0, timeoutSeconds, &buffer, nullptr, PackageManagerConfigurationLog::Get());

    if (status == MMI_OK)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "RunCommand failed with status: %d and output '%s'", status, buffer);
    }

    if (buffer)
    {
        free(buffer);
    }
    return status;
}

std::string PackageManagerConfigurationBase::GetFingerprint()
{
    std::string hashString = "";
    RunCommand(g_commandGetInstalledPackagesHash.c_str(), true, &hashString, 0);
    return hashString;
}

std::vector<std::string> PackageManagerConfigurationBase::GetPackagesNames(std::vector<std::string> packages)
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

std::vector<std::string> PackageManagerConfigurationBase::Split(const std::string& str, const std::string& delimiter)
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

std::vector<std::string> PackageManagerConfigurationBase::GetReportedPackages(std::vector<std::string> packages)
{
    std::vector<std::string> result;
    int status;
    for (auto& packageName : packages)
    {
        std::string command = std::regex_replace(g_commandGetInstalledPackageVersion, std::regex("\\$value"), packageName);

        std::string rawVersion = "";
        status = RunCommand(command.c_str(), true, &rawVersion, 0);
        if (status != MMI_OK && IsFullLoggingEnabled())
        {
            OsConfigLogError(PackageManagerConfigurationLog::Get(), "Get the installed version of package %s failed with status %d", packageName.c_str(), status);
        }

        std::string version = !rawVersion.empty() ? Split(rawVersion, ":")[1] : "(failed)";
        std::string packageElement = packageName + "=" + Trim(version, " ");
        result.push_back(packageElement);
    }

  return result;
}

std::string PackageManagerConfigurationBase::TrimStart(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_first_not_of(trim);
    if (pos == std::string::npos)
    {
        return str;
    }
    return str.substr(pos);
}

std::string PackageManagerConfigurationBase::TrimEnd(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_last_not_of(trim);
    if (pos == std::string::npos)
    {
        return str;
    }
    return str.substr(0, pos + 1);
}

std::string PackageManagerConfigurationBase::Trim(const std::string& str, const std::string& trim)
{
    return TrimStart(TrimEnd(str, trim), trim);
}


std::string PackageManagerConfigurationBase::GetSourcesFingerprint(std::string sourcesDir)
{
    std::string hashString = "";
    std::string command = std::regex_replace(g_commandGetSourcesFingerprint, std::regex("\\$value"), sourcesDir);
    int status = RunCommand(command.c_str(), true, &hashString, 0);

    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Get the fingerprint of source files in directory %s failed with status %d", sourcesDir.c_str(), status);
    }

    return !hashString.empty() ? hashString : "(failed)";
}

std::vector<std::string> PackageManagerConfigurationBase::GetSourcesFilenames()
{
    std::vector<std::string> result;

    for (auto& file : std::filesystem::directory_iterator(m_sourcesConfigurationDir))
    {
        std::string fileName = file.path().filename();
        int listExtensionLength = g_listExtension.length();
        if (fileName.compare(fileName.length() - listExtensionLength, listExtensionLength, g_listExtension) == 0)
        {
            std::string fileNameWithoutExtension = fileName.substr(0, fileName.find_last_of("."));
            result.push_back(fileNameWithoutExtension);
        }
    }

    return result;
}

int PackageManagerConfigurationBase::ConfigureSources(const std::map<std::string, std::string> sources)
{
    int status = MMI_OK;
    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::ModifyingSources);

    for (auto& source : sources)
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::ModifyingSources, source.first);
        std::string sourceFileName = source.first + ".list";
        std::string sourcesFilePath = std::filesystem::path(m_sourcesConfigurationDir) / sourceFileName;
        OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "Starting to configure source(s) file: %s", sourcesFilePath.c_str());

        // Delete file when provided map value is empty
        if (source.second.empty()) 
        {
            if (FileExists(sourcesFilePath.c_str()))
            {
                status = remove(sourcesFilePath.c_str());
            }
            else
            {
                OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "Nothing to delete. Source(s) file: %s does not exist", sourcesFilePath.c_str());
            }

            if (status != MMI_OK)
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to delete source(s) file %s with status %d. Stopping configuration for further sources", sourcesFilePath.c_str(), status);
                m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::ModifyingSources, source.first);
                return errno;
            }
        }
        else
        {
            std::ofstream output(sourcesFilePath);
            
            if (output.fail())
            {
                OsConfigLogError(PackageManagerConfigurationLog::Get(), "Failed to create source(s) file %s. Stopping configuration for further sources", sourcesFilePath.c_str());
                m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::ModifyingSources, source.first);
                output.close();
                return errno;
            }
            
            output << source.second << std::endl;
            output.close();
        }
    }

    m_executionState.SetExecutionState(StateComponent::Running, SubStateComponent::UpdatingPackagesSources);
    status = RunCommand(g_commandAptUpdate.c_str(), true, nullptr, 0);

    if (status != MMI_OK)
    {
        OsConfigLogError(PackageManagerConfigurationLog::Get(), "Refresh sources failed with status %d", status);
        status == ETIME ? m_executionState.SetExecutionState(StateComponent::TimedOut, SubStateComponent::UpdatingPackagesSources)
                        : m_executionState.SetExecutionState(StateComponent::Failed, SubStateComponent::UpdatingPackagesSources);
    }
    else
    {
        OsConfigLogInfo(PackageManagerConfigurationLog::Get(), "Successfully configured sources");
        m_executionState.SetExecutionState(StateComponent::Succeeded, SubStateComponent::None);
    }
    
    return status;
}