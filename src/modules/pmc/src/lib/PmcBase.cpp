// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <set>

#include <CommonUtils.h>
#include <Mmi.h>
#include <PmcBase.h>

static const std::string g_componentName = "PackageManager";
static const std::string g_reportedObjectName = "state";
static const std::string g_desiredObjectName = "desiredState";
static const std::string g_packages = "packages";
static const std::string g_sources = "sources";
static const std::string g_gpgKeys = "gpgKeys";
static const std::string g_executionState = "executionState";
static const std::string g_executionSubstate = "executionSubstate";
static const std::string g_executionSubstateDetails = "executionSubstateDetails";
static const std::string g_packagesFingerprint = "packagesFingerprint";
static const std::string g_sourcesFingerprint = "sourcesFingerprint";
static const std::string g_sourcesFilenames = "sourcesFilenames";

constexpr const char* g_commandAptUpdate = "apt-get update";
constexpr const char* g_commandExecuteUpdate = "apt-get install $value -y --allow-downgrades --auto-remove";
constexpr const char* g_commandGetInstalledPackageVersion = "apt-cache policy $value | grep Installed";
constexpr const char* g_commandDownloadGpgKey = "curl -sSL $url | gpg --dearmor --yes -o $destination";

constexpr const char* g_regexPackages = "(?:[a-zA-Z\\d\\-]+(?:=[a-zA-Z\\d\\.\\+\\-\\~\\:]+|\\-| )*)+";
constexpr const char* g_regexSources = "^(deb|deb-src)(?:\\s+\\[(.*)\\])?\\s+(https?:\\/\\/\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$";
constexpr const char* g_regexSignedByOption = "^.*signed-by=(\\S*).*$";

constexpr const char* g_sourcesFolderPath = "/etc/apt/sources.list.d/";
constexpr const char* g_keysFolderPath = "/usr/share/keyrings/";

constexpr const char* g_listExtension = ".list";
constexpr const char g_moduleInfo[] = R""""({
    "Name": "PMC",
    "Description": "Module designed to install DEB-packages using APT",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["PackageManager"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

using StateComponent = ExecutionState::StateComponent;
using SubstateComponent = ExecutionState::SubstateComponent;

OSCONFIG_LOG_HANDLE PmcLog::m_log = nullptr;

PmcBase::PmcBase(unsigned int maxPayloadSizeBytes, const char* sourcesDirectory)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_sourcesConfigurationDirectory = sourcesDirectory;
    m_executionState = ExecutionState();
    m_lastReachedStateHash = 0;
}

PmcBase::PmcBase(unsigned int maxPayloadSizeBytes)
    : PmcBase(maxPayloadSizeBytes, g_sourcesFolderPath)
{
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
    if (!CanRunOnThisPlatform())
    {
        return ENODEV;
    }

    size_t payloadHash = HashString(payload);
    if (m_lastReachedStateHash == payloadHash)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(PmcLog::Get(), "Ignoring update, desired state equals current state.");
        }
        return MMI_OK;
    }

    int status = PMC_0K;
    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingJsonPayload);

    int maxPayloadSizeBytes = static_cast<int>(GetMaxPayloadSizeBytes());
    if ((0 != maxPayloadSizeBytes) && (payloadSizeBytes > maxPayloadSizeBytes))
    {
        OsConfigLogError(PmcLog::Get(), "%s %s payload too large. Max payload expected %d, actual payload size %d", componentName, objectName, maxPayloadSizeBytes, payloadSizeBytes);
        m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingJsonPayload);
        return E2BIG;
    }

    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(PmcLog::Get(), "Unabled to parse JSON payload: %s", payload);
        m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingJsonPayload);
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
                    DesiredState desiredState;
                    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingDesiredState);

                    status = DeserializeDesiredState(document, desiredState);
                    if (m_executionState.IsSuccessful())
                    {
                        status = ValidateAndGetPackagesNames(desiredState.Packages);
                        if (m_executionState.IsSuccessful())
                        {
                            status = DownloadGpgKeys(desiredState.GpgKeys);
                            if (m_executionState.IsSuccessful())
                            {
                                status = ConfigureSources(desiredState.Sources, desiredState.GpgKeys);
                                if (m_executionState.IsSuccessful())
                                {
                                    status = ExecuteUpdates(desiredState.Packages);
                                }
                            }
                        }
                    }
                    else
                    {
                        OsConfigLogError(PmcLog::Get(), "Failed to deserialize %s", g_desiredObjectName.c_str());
                        m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingDesiredState);
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(PmcLog::Get(), "JSON payload is not a %s object", g_desiredObjectName.c_str());
                    m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingDesiredState);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(PmcLog::Get(), "Invalid objectName: %s", objectName);
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingDesiredState);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "Invalid componentName: %s", componentName);
            m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingJsonPayload);
            status = EINVAL;
        }
    }

    // If anything goes wrong, reset the last reached state since the current state is undefined.
    m_lastReachedStateHash = m_executionState.IsSuccessful() ? payloadHash : 0;

    return m_executionState.IsSuccessful() ? MMI_OK : status;
}

int PmcBase::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    if (!CanRunOnThisPlatform())
    {
        return ENODEV;
    }

    int status = PMC_0K;

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
                reportedState.ExecutionState = m_executionState;
                reportedState.PackagesFingerprint = GetPackagesFingerprint();
                reportedState.Packages = GetReportedPackages(m_desiredPackages);
                reportedState.SourcesFingerprint = GetSourcesFingerprint(m_sourcesConfigurationDirectory);
                reportedState.SourcesFilenames = ListFiles(m_sourcesConfigurationDirectory, g_listExtension);
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

    return (status == PMC_0K) ? MMI_OK : status;
}

unsigned int PmcBase::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

int PmcBase::ValidateDocument(const rapidjson::Document& document)
{
    if (!document.HasMember(g_sources.c_str()) && !document.HasMember(g_packages.c_str()) && !document.HasMember(g_gpgKeys.c_str()))
    {
        OsConfigLogError(PmcLog::Get(), "JSON object does not contain any of ['%s', '%s', '%s']", g_sources.c_str(), g_packages.c_str(), g_gpgKeys.c_str());
        m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingDesiredState);
        return EINVAL;
    }

    return PMC_0K;
}

int PmcBase::DeserializeSources(const rapidjson::Document& document, DesiredState& object)
{
    int status = PMC_0K;

    if (document.HasMember(g_sources.c_str()))
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingSources);
        if (document[g_sources.c_str()].IsObject())
        {
            for (auto &member : document[g_sources.c_str()].GetObject())
            {
                if (member.value.IsString())
                {
                    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingSources, member.name.GetString());
                    object.Sources[member.name.GetString()] = member.value.GetString();
                }
                else if (member.value.IsNull())
                {
                    object.Sources[member.name.GetString()] = "";
                }
                else
                {
                    OsConfigLogError(PmcLog::Get(), "Invalid string in JSON object string map at key %s", member.name.GetString());
                    m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingSources, member.name.GetString());
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "%s is not a map", g_sources.c_str());
            m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingSources);
            status = EINVAL;
        }
    }

    return status;
}

int PmcBase::DeserializePackages(const rapidjson::Document& document, DesiredState& object)
{
    int status = PMC_0K;

    if (document.HasMember(g_packages.c_str()))
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingPackages);
        if (document[g_packages.c_str()].IsArray())
        {
            for (rapidjson::SizeType i = 0; i < document[g_packages.c_str()].Size(); ++i)
            {
                if (document[g_packages.c_str()][i].IsString())
                {
                    std::string package = document[g_packages.c_str()][i].GetString();
                    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingPackages, package);
                    object.Packages.push_back(package);
                }
                else
                {
                    OsConfigLogError(PmcLog::Get(), "Invalid string in JSON object string array at position %d", i);
                    m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingPackages, "index " + i);
                    status = EINVAL;
                }
            }
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "%s is not an array", g_packages.c_str());
            m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingPackages);
            status = EINVAL;
        }
    }

    return status;
}

int PmcBase::DeserializeDesiredState(const rapidjson::Document& document, DesiredState& object)
{
    int status = ValidateDocument(document);
    if (status == PMC_0K)
    {
        status = DeserializeGpgKeys(document, object);
    }

    if (status == PMC_0K)
    {
        status = DeserializeSources(document, object);
    }

    if (status == PMC_0K)
    {
        status = DeserializePackages(document, object);
    }

    return status;
}

int PmcBase::DeserializeGpgKeys(const rapidjson::Document& document, DesiredState& state)
{
    if (!document.HasMember(g_gpgKeys.c_str()))
    {
        return PMC_0K;
    }

    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingGpgKeys);
    auto& section = document[g_gpgKeys.c_str()];

    if (!section.IsObject())
    {
        OsConfigLogError(PmcLog::Get(), "%s is not a map", g_gpgKeys.c_str());
        m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingGpgKeys);
        return EINVAL;
    }

    for (auto& member : section.GetObject())
    {
        auto key = member.name.GetString();
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingGpgKeys, key);

        if (member.value.IsString())
        {
            state.GpgKeys[key] = member.value.GetString();
        }
        else if (member.value.IsNull())
        {
            state.GpgKeys[key] = std::string();
        }
        else
        {
            OsConfigLogError(PmcLog::Get(), "Invalid string in JSON object string map at key %s", key);
            m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingGpgKeys, key);
            return EINVAL;
        }
    }

    return PMC_0K;
}

int PmcBase::CopyJsonPayload(const rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = PMC_0K;

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
    if (status != PMC_0K && IsFullLoggingEnabled())
    {
        OsConfigLogError(PmcLog::Get(), "ExecuteUpdate failed with status %d and arguments '%s'", status, value.c_str());
    }
    return status;
}

int PmcBase::ExecuteUpdates(const std::vector<std::string>& packages)
{
    int status = PMC_0K;

    for (std::string package : packages)
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::InstallingPackages, package);
        status = ExecuteUpdate(package);
        if (status != PMC_0K)
        {
            OsConfigLogError(PmcLog::Get(), "Failed to update package(s): %s", package.c_str());
            status == ETIME ?
                m_executionState.SetExecutionState(StateComponent::TimedOut, SubstateComponent::InstallingPackages, package) :
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::InstallingPackages, package);

            return status;
        }
    }

    m_executionState.SetExecutionState(StateComponent::Succeeded, SubstateComponent::None);
    return status;
}

int PmcBase::SerializeState(const State& reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes)
{
    int status = PMC_0K;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key(g_packagesFingerprint.c_str());
    writer.String(reportedState.PackagesFingerprint.c_str());

    writer.Key(g_packages.c_str());
    writer.StartArray();
    for (auto& element : reportedState.Packages)
    {
        writer.String(element.c_str());
    }
    writer.EndArray();

    writer.Key(g_executionState.c_str());
    writer.Int(reportedState.ExecutionState.GetExecutionState());

    writer.Key(g_executionSubstate.c_str());
    writer.Int(reportedState.ExecutionState.GetExecutionSubstate());

    writer.Key(g_executionSubstateDetails.c_str());
    writer.String(reportedState.ExecutionState.GetExecutionSubstateDetails().c_str());

    writer.Key(g_sourcesFingerprint.c_str());
    writer.String(reportedState.SourcesFingerprint.c_str());

    writer.Key(g_sourcesFilenames.c_str());
    writer.StartArray();
    for (auto& element : reportedState.SourcesFilenames)
    {
        writer.String(element.c_str());
    }
    writer.EndArray();

    writer.EndObject();

    unsigned int bufferSize = buffer.GetSize();

    if ((0 != maxPayloadSizeBytes) && (bufferSize > maxPayloadSizeBytes))
    {
        OsConfigLogError(PmcLog::Get(), "Failed to serialize object %s. Max payload expected %d, actual payload size %d", g_reportedObjectName.c_str(), maxPayloadSizeBytes, bufferSize);
        status = E2BIG;
    }
    else
    {
        status = CopyJsonPayload(buffer, payload, payloadSizeBytes);
        if (PMC_0K != status)
        {
            OsConfigLogError(PmcLog::Get(), "Failed to serialize object %s", g_reportedObjectName.c_str());
        }
    }

    return status;
}

int PmcBase::ValidateAndGetPackagesNames(const std::vector<std::string>& packagesLines)
{
    // Clear previous packages names
    m_desiredPackages.clear();

    for (auto& packagesLine : packagesLines)
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DeserializingPackages, packagesLine);

        // Validate packages input
        std::regex pattern(g_regexPackages);
        if (!std::regex_match(packagesLine, pattern))
        {
            OsConfigLogError(PmcLog::Get(), "Invalid package(s) argument provided: %s", packagesLine.c_str());
            m_desiredPackages.clear();
            m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DeserializingPackages, packagesLine);
            return EINVAL;
        }

        std::vector<std::string> result = Split(packagesLine, " ");
        for (auto& element : result)
            {
                std::string packageName = Split(element, "=")[0];
                m_desiredPackages.push_back(TrimEnd(packageName, "-"));
            }
    }

    return 0;
}

std::vector<std::string> PmcBase::GetReportedPackages(const std::vector<std::string>& packages)
{
    std::vector<std::string> result;
    std::set<std::string> uniquePackages;

    int status = PMC_0K;
    for (auto& packageName : packages)
    {
        if (uniquePackages.insert(packageName).second)
        {
            std::string command = std::regex_replace(g_commandGetInstalledPackageVersion, std::regex("\\$value"), packageName);

            std::string rawVersion = "";
            status = RunCommand(command.c_str(), &rawVersion);
            if (status != PMC_0K && IsFullLoggingEnabled())
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

std::vector<std::string> PmcBase::ListFiles(const char* directory, const char* fileNameExtension)
{
    struct dirent* entry = nullptr;
    DIR* directoryStream = nullptr;
    int extensionLength = (nullptr != fileNameExtension) ? strlen(fileNameExtension) : 0;
    char* fileName = nullptr;
    int fileNameLength = 0;
    char* lastCharacters = nullptr;
    std::vector<std::string> result;

    directoryStream = opendir(directory);
    if (nullptr != directoryStream)
    {
        while ((entry = readdir(directoryStream)))
        {
            fileName = entry->d_name;
            if ((nullptr == fileName) || (0 == strcmp(fileName, ".")) || (0 == strcmp(fileName, "..")))
            {
                continue;
            }

            if (nullptr == fileNameExtension)
            {
                result.push_back(fileName);
            }
            else
            {
                fileNameLength = strlen(fileName);
                if (fileNameLength >= extensionLength)
                {
                    lastCharacters = &fileName[fileNameLength-extensionLength];
                    if (0 == strcmp(lastCharacters, fileNameExtension))
                    {
                        result.push_back(fileName);
                    }
                }
            }
        }
        closedir(directoryStream);
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(PmcLog::Get(), "Failed to open directory %s, cannot list files", directory);
    }

    return result;
}

int PmcBase::ConfigureSources(const std::map<std::string, std::string>& sources, const std::map<std::string, std::string>& gpgKeys)
{
    int status = PMC_0K;

    for (auto& source : sources)
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::ModifyingSources, source.first);
        std::string sourceFileName = source.first + ".list";
        std::string sourcesFilePath = m_sourcesConfigurationDirectory + sourceFileName;

        // Delete file when provided map value is empty
        if (source.second.empty())
        {
            if (FileExists(sourcesFilePath.c_str()))
            {
                status = remove(sourcesFilePath.c_str());
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PmcLog::Get(), "Nothing to delete. Source(s) file: %s does not exist", sourcesFilePath.c_str());
            }

            if (status != PMC_0K)
            {
                OsConfigLogError(PmcLog::Get(), "Failed to delete source(s) file %s with status %d. Stopping configuration for further sources", sourcesFilePath.c_str(), status);
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::ModifyingSources, source.first);
                return errno;
            }
        }
        else
        {
            std::ofstream output(sourcesFilePath);

            if (output.fail())
            {
                OsConfigLogError(PmcLog::Get(), "Failed to create source(s) file %s. Stopping configuration for further sources", sourcesFilePath.c_str());
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::ModifyingSources, source.first);
                output.close();
                return errno;
            }

            std::string packageSource = source.second;

            if (ValidateAndUpdatePackageSource(packageSource, gpgKeys))
            {
                output << packageSource << std::endl;
                output.close();
            }
            else
            {
                OsConfigLogError(PmcLog::Get(), "Invalid source format provided for %s. Stopping configuration for further sources", source.first.c_str());
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::ModifyingSources, source.first);
                output.close();
                return EINVAL;
            }
        }
    }

    m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::UpdatingPackageLists);
    status = RunCommand(g_commandAptUpdate, nullptr, true);

    if (status != PMC_0K)
    {
        OsConfigLogError(PmcLog::Get(), "Refresh package lists failed with status %d", status);
        status == ETIME ? m_executionState.SetExecutionState(StateComponent::TimedOut, SubstateComponent::UpdatingPackageLists)
            : m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::UpdatingPackageLists);
    }
    else
    {
        m_executionState.SetExecutionState(StateComponent::Succeeded, SubstateComponent::None);
    }

    return status;
}

bool PmcBase::ValidateAndUpdatePackageSource(std::string& packageSource, const std::map<std::string, std::string>& gpgKeys)
{
    std::smatch sourceMatches;

    if (std::regex_match(packageSource, sourceMatches, std::regex(g_regexSources)))
    {
        if (sourceMatches.size() >= 3)
        {
            std::smatch signedByMatches;
            std::string matchedString = sourceMatches[2].matched ? sourceMatches[2].str() : std::string();
            if (std::regex_match(matchedString, signedByMatches, std::regex(g_regexSignedByOption)))
            {
                if (signedByMatches.size() >= 2)
                {
                    if (signedByMatches[1].matched)
                    {
                        std::string gpgKeyFileId = signedByMatches[1].str();
                        const auto &key = gpgKeys.find(gpgKeyFileId);
                        if (key != gpgKeys.end())
                        {
                            const std::string signedByPrefix = "signed-by=";
                            std::string placeholder = signedByPrefix + gpgKeyFileId;
                            std::size_t index = packageSource.find(placeholder);

                            if (index != std::string::npos)
                            {
                                std::string gpgKeyConfiguration = signedByPrefix + GenerateGpgKeyPath(gpgKeyFileId);
                                packageSource.replace(index, placeholder.length(), gpgKeyConfiguration);
                            }
                        }
                    }
                }
            }
        }

        return true;
    }

    return false;
}

std::string PmcBase::GenerateGpgKeyPath(const std::string& gpgKeyId)
{
    return g_keysFolderPath + gpgKeyId + ".gpg";
}

int PmcBase::DownloadGpgKeys(const std::map<std::string, std::string>& gpgKeys)
{
    int status = PMC_0K;

    for (auto& key : gpgKeys)
    {
        m_executionState.SetExecutionState(StateComponent::Running, SubstateComponent::DownloadingGpgKeys, key.first);
        std::string keyFilePath = GenerateGpgKeyPath(key.first);
        const std::string& sourceUrl = key.second;

        // Delete file when provided map value is empty
        if (sourceUrl.empty())
        {
            if (FileExists(keyFilePath.c_str()))
            {
                if (remove(keyFilePath.c_str()))
                {
                    status = errno;
                    OsConfigLogError(PmcLog::Get(), "Failed to delete key file %s", keyFilePath.c_str());
                    m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::ModifyingSources, key.first);
                }
            }
            else if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PmcLog::Get(), "Nothing to delete. Key file %s does not exist", keyFilePath.c_str());
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(PmcLog::Get(), "Downloading GPG key from %s to %s", sourceUrl.c_str(), keyFilePath.c_str());
            }

            std::string command = std::regex_replace(g_commandDownloadGpgKey, std::regex("\\$url"), sourceUrl);
            command = std::regex_replace(command, std::regex("\\$destination"), keyFilePath);
            status = RunCommand(command.c_str(), nullptr);

            if (status != PMC_0K)
            {
                OsConfigLogError(PmcLog::Get(), "Failed to download key from %s to %s", sourceUrl.c_str(), keyFilePath.c_str());
                m_executionState.SetExecutionState(StateComponent::Failed, SubstateComponent::DownloadingGpgKeys, key.first );
            }
        }
    }

    return status;
}
