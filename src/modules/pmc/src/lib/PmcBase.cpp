// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <set>

#include <CommonUtils.h>
#include <Mmi.h>
#include <PmcBase.h>

static const std::string g_componentName = "PackageManagerConfiguration";
static const std::string g_reportedObjectName = "state";
static const std::string g_desiredObjectName = "desiredState";
static const std::string g_packages = "packages";
static const std::string g_executionState = "executionState";
static const std::string g_executionSubState = "executionSubState";
static const std::string g_executionSubStateDetails = "executionSubStateDetails";
static const std::string g_packagesFingerprint = "packagesFingerprint";
static const std::string g_requiredTools[] = {"apt-get", "apt-cache", "dpkg-query"};

constexpr const char* g_commandCheckToolPresence = "command -v $value";
constexpr const char* g_commandAptUpdate = "apt-get update";
constexpr const char* g_commandExecuteUpdate = "apt-get install $value -y --allow-downgrades --auto-remove";
constexpr const char* g_commandGetInstalledPackageVersion = "apt-cache policy $value | grep Installed";
constexpr const char* g_commandGetInstalledPackagesHash = "dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64";
constexpr const char* g_regexPackages = "(?:[a-zA-Z\\d\\-]+(?:=[a-zA-Z\\d\\.\\+\\-\\~\\:]+|\\-| )*)+";

constexpr const char g_moduleInfo[] = R""""({
    "Name": "PMC",
    "Description": "Module designed to install DEB-packages using APT",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["PackageManagerConfiguration"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

OSCONFIG_LOG_HANDLE PmcLog::m_log = nullptr;

PmcBase::PmcBase(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_executionState = ExecutionState();
    m_lastReachedStateHash = 0;
}

int PmcBase::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(PmcLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(PmcLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(PmcLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(PmcLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, g_moduleInfo, len);
                *payloadSizeBytes = len;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(PmcLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
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

int PmcBase::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    if(!CanRunOnThisPlatform())
    {
        return ENODEV;
    }

    size_t payloadHash = std::hash<std::string>{}(payload);
    if (m_lastReachedStateHash == payloadHash)
    {
        OsConfigLogInfo(PmcLog::Get(), "Ignoring update, desired state equals current state.");
        return MMI_OK;
    }

    int status = MMI_OK;
    m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::deserializingJsonPayload);

    int maxPayloadSizeBytes = static_cast<int>(GetMaxPayloadSizeBytes());
    if ((0 != maxPayloadSizeBytes) && (payloadSizeBytes > maxPayloadSizeBytes))
    {
        OsConfigLogError(PmcLog::Get(), "%s %s payload too large. Max payload expected %d, actual payload size %d", componentName, objectName, maxPayloadSizeBytes, payloadSizeBytes);
        m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingJsonPayload);
        return E2BIG;
    }

    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(PmcLog::Get(), "Unabled to parse JSON payload: %s", payload);
        m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingJsonPayload);
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
                    PmcBase::DesiredState desiredState;
                    m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::deserializingDesiredState);

                    if (0 == DeserializeDesiredState(document, desiredState))
                    {
                        status = ValidateAndGetPackagesNames(desiredState.packages);
                        if (status != MMI_OK)
                        {
                            return status;
                        }

                        if (status == MMI_OK)
                        {
                            status = PmcBase::ExecuteUpdates(desiredState.packages, true);
                        }
                    }
                    else
                    {
                        OsConfigLogError(PmcLog::Get(), "Failed to deserialize %s", g_desiredObjectName.c_str());
                        m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingDesiredState);
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(PmcLog::Get(), "JSON payload is not a %s object", g_desiredObjectName.c_str());
                    m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingDesiredState);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(PmcLog::Get(), "Invalid objectName: %s", objectName);
                m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingDesiredState);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "Invalid componentName: %s", componentName);
            m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingJsonPayload);
            status = EINVAL;
        }
    }

    // If anything goes wrong, reset the last reached state since the current state is undefined.
    m_lastReachedStateHash = m_executionState.IsSuccessful() ? payloadHash : 0;

    return status;
}

int PmcBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
     if(!CanRunOnThisPlatform())
    {
        return ENODEV;
    }

    int status = MMI_OK;

    if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(PmcLog::Get(), "Invalid payloadSizeBytes");
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
                OsConfigLogError(PmcLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

unsigned int PmcBase::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

bool PmcBase::CanRunOnThisPlatform()
{
    for (auto& tool : g_requiredTools)
    {
        std::string command = std::regex_replace(g_commandCheckToolPresence, std::regex("\\$value"), tool);
        if (RunCommand(command.c_str(), nullptr) != MMI_OK)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PmcLog::Get(), "Cannot run on this platform, could not find required tool %s", tool.c_str());
            }

            return false;
        }
    }

    return true;
}

int PmcBase::DeserializeDesiredState(rapidjson::Document& document, DesiredState& object)
{
    int status = 0;

    if (document.HasMember(g_packages.c_str()))
    {
        m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::deserializingPackages);
        if (document[g_packages.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_packages.c_str()].Size(); ++i)
            {
                if (document[g_packages.c_str()][i].IsString())
                {
                    std::string package = document[g_packages.c_str()][i].GetString();
                    m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::deserializingPackages, package);
                    object.packages.push_back(package);
                }
                else
                {
                    OsConfigLogError(PmcLog::Get(), "Invalid string in JSON object string array at position %d", i);
                    m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingPackages, "index " + i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "%s is not an array", g_packages.c_str());
            m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingPackages);
            status = EINVAL;
        }
    }

    if (!document.HasMember(g_packages.c_str()))
    {
        OsConfigLogError(PmcLog::Get(), "JSON object does not contain '%s'", g_packages.c_str());
        m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingDesiredState);
        status = EINVAL;
    }

    return status;
}

int PmcBase::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(PmcLog::Get(), "Unable to allocate memory for payload");
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
        OsConfigLogError(PmcLog::Get(), "Could not allocate payload: %s", e.what());
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

int PmcBase::ExecuteUpdate(const std::string &value)
{
    std::string command = std::regex_replace(g_commandExecuteUpdate, std::regex("\\$value"), value);

    int status = RunCommand(command.c_str(), nullptr, true);
    if (status != MMI_OK && IsFullLoggingEnabled())
    {
        OsConfigLogError(PmcLog::Get(), "ExecuteUpdate failed with status %d and arguments '%s'", status, value.c_str());
    }
    return status;
}

int PmcBase::ExecuteUpdates(const std::vector<std::string> packages, bool updatePackageLists)
{
    int status = MMI_OK;

    if (updatePackageLists)
    {
        m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::updatingPackagesLists);

        status = RunCommand(g_commandAptUpdate, nullptr);
        if (status != MMI_OK)
        {
            status == ETIME ? m_executionState.SetExecutionState(ExecutionState::StateComponent::timedOut, ExecutionState::SubStateComponent::updatingPackagesLists)
                            : m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::updatingPackagesLists);
            return status;
        }
    }

    for (std::string package : packages)
    {
        OsConfigLogInfo(PmcLog::Get(), "Starting to update package(s): %s", package.c_str());
        m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::installingPackages, package);
        status = ExecuteUpdate(package);
        if (status != MMI_OK)
        {
            OsConfigLogError(PmcLog::Get(), "Failed to update package(s): %s", package.c_str());
            status == ETIME ? m_executionState.SetExecutionState(ExecutionState::StateComponent::timedOut, ExecutionState::SubStateComponent::installingPackages, package)
                            : m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::installingPackages, package);
            return status;
        }
        OsConfigLogInfo(PmcLog::Get(), "Successfully updated package(s): %s", package.c_str());
    }

    m_executionState.SetExecutionState(ExecutionState::StateComponent::succeeded, ExecutionState::SubStateComponent::none);
    return status;
}

int PmcBase::SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
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
    writer.Int(reportedState.executionState.GetExecutionState());

    writer.Key(g_executionSubState.c_str());
    writer.Int(reportedState.executionState.GetExecutionSubState());

    writer.Key(g_executionSubStateDetails.c_str());
    writer.String(reportedState.executionState.GetExecutionSubStateDetails().c_str());

    writer.EndObject();

    unsigned int bufferSize = buffer.GetSize();

    if ((0 != maxPayloadSizeBytes) && (bufferSize > maxPayloadSizeBytes))
    {
        OsConfigLogError(PmcLog::Get(), "Failed to serialize object %s. Max payload expected %d, actual payload size %d", g_reportedObjectName.c_str(), maxPayloadSizeBytes, bufferSize);
        status = E2BIG;
    }
    else
    {
        status = PmcBase::CopyJsonPayload(buffer, payload, payloadSizeBytes);
        if (0 != status)
        {
            OsConfigLogError(PmcLog::Get(), "Failed to serialize object %s", g_reportedObjectName.c_str());
        }
    }

    return status;
}

std::string PmcBase::GetFingerprint()
{
    std::string hashString = "";
    RunCommand(g_commandGetInstalledPackagesHash, &hashString);
    return hashString;
}

int PmcBase::ValidateAndGetPackagesNames(std::vector<std::string> packagesLines)
{
    // Clear previous packages names
    m_desiredPackages.clear();

    for (auto& packagesLine : packagesLines)
    {
        m_executionState.SetExecutionState(ExecutionState::StateComponent::running, ExecutionState::SubStateComponent::deserializingPackages, packagesLine);

        // Validate packages input
        std::regex pattern(g_regexPackages);
        if (!std::regex_match(packagesLine, pattern))
        {
            OsConfigLogError(PmcLog::Get(), "Invalid package(s) argument provided: %s", packagesLine.c_str());
            m_desiredPackages.clear();
            m_executionState.SetExecutionState(ExecutionState::StateComponent::failed, ExecutionState::SubStateComponent::deserializingPackages, packagesLine);
            return EINVAL;
        }

        std::vector<std::string> result = Split(packagesLine, " ");
        for (auto& element : result)
            {
                std::string packageName = Split(element, "=")[0];
                m_desiredPackages.push_back(TrimEnd(packageName, "-"));
            }
    }

    return MMI_OK;
}

std::vector<std::string> PmcBase::GetReportedPackages(std::vector<std::string> packages)
{
    std::vector<std::string> result;
    std::set<std::string> uniquePackages;

    int status;
    for (auto& packageName : packages)
    {
        if (uniquePackages.insert(packageName).second)
        {
            std::string command = std::regex_replace(g_commandGetInstalledPackageVersion, std::regex("\\$value"), packageName);

            std::string rawVersion = "";
            status = RunCommand(command.c_str(), &rawVersion);
            if (status != MMI_OK && IsFullLoggingEnabled())
            {
                OsConfigLogError(PmcLog::Get(), "Get the installed version of package %s failed with status %d", packageName.c_str(), status);
            }

            std::string version;
            if (!rawVersion.empty())
            {
                size_t pos = rawVersion.find_first_of(':') + 1;
                version = rawVersion.substr(pos);
            }
            else
            {
                version = "(failed)";
            }

            std::string packageElement = packageName + "=" + Trim(version, " ");
            result.push_back(packageElement);
        }
    }

  return result;
}

std::vector<std::string> PmcBase::Split(const std::string& str, const std::string& delimiter)
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

std::string PmcBase::TrimStart(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_first_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(pos);
}

std::string PmcBase::TrimEnd(const std::string& str, const std::string& trim)
{
    size_t pos = str.find_last_not_of(trim);
    if (pos == std::string::npos)
    {
        return "";
    }
    return str.substr(0, pos + 1);
}

std::string PmcBase::Trim(const std::string& str, const std::string& trim)
{
    return TrimStart(TrimEnd(str, trim), trim);
}
