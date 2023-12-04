// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <regex>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


#include "CommonUtils.h"
#include "Mmi.h"
#include "Ztsi.h"

static const char g_configurationPropertyEnabled[] = "enabled";
static const char g_configurationPropertyMaxScheduledAttestationsPerDay[] = "maxScheduledAttestationsPerDay";
static const char g_configurationPropertyMaxManualAttestationsPerDay[] = "maxManualAttestationsPerDay";

static const bool g_defaultEnabled = false;
static const int g_defaultMaxScheduledAttestationsPerDay = 10;
static const int g_defaultMaxManualAttestationsPerDay = 10;

static const int g_totalAttestationsAllowedPerDay = 100;

// Block for a maximum of (20 milliseconds x 5 retries) 100ms
static const unsigned int g_lockWait = 20;
static const unsigned int g_lockWaitMaxRetries = 5;

const std::string Ztsi::m_componentName = "Ztsi";
const std::string Ztsi::m_desiredEnabled = "desiredEnabled";
const std::string Ztsi::m_desiredMaxScheduledAttestationsPerDay = "desiredMaxScheduledAttestationsPerDay";
const std::string Ztsi::m_desiredMaxManualAttestationsPerDay = "desiredMaxManualAttestationsPerDay";
const std::string Ztsi::m_reportedEnabled = "enabled";
const std::string Ztsi::m_reportedMaxScheduledAttestationsPerDay = "maxScheduledAttestationsPerDay";
const std::string Ztsi::m_reportedMaxManualAttestationsPerDay = "maxManualAttestationsPerDay";

int SerializeJsonObject(MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, rapidjson::Document& document)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    if ((maxPayloadSizeBytes > 0) && (buffer.GetSize() > maxPayloadSizeBytes))
    {
        OsConfigLogError(ZtsiLog::Get(), "Failed to serialize JSON object to buffer");
        status = E2BIG;
    }
    else
    {
        try
        {
            *payload = new (std::nothrow) char[buffer.GetSize()];
            if (nullptr == *payload)
            {
                OsConfigLogError(ZtsiLog::Get(), "Unable to allocate memory for payload");
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
                delete[] *payload;
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

bool IsValidClientName(const char* name)
{
    // "Azure OSConfig <model version>;<major>.<minor>.<patch>.<yyyymmdd><build>"
    const std::string productInfoTemplate = "^((Azure OSConfig )([0-9]+);(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.([0-9]{8})).*$";
    const std::string clientNamePrefix = "Azure OSConfig ";
    const std::string modelVersionDelimiter = ";";
    const std::string semanticVersionDelimeter = ".";

    // OSConfig model version 5 published on September 27, 2021
    const int referenceModelVersion = 5;
    const int referenceReleaseDay = 27;
    const int referenceReleaseMonth = 9;
    const int referenceReleaseYear = 2021;

    // String length of date string yyyymmmdd
    const int dateLength = 9;

    bool isValid = true;

    const std::string clientName = name;

    // Regex for validating the client name against the OSConfig product info
    std::regex pattern(productInfoTemplate);

    if (!clientName.empty() && std::regex_match(clientName, pattern))
    {
        std::string versionInfo = clientName.substr(clientNamePrefix.length());
        std::string modelVersion = versionInfo.substr(0, versionInfo.find(modelVersionDelimiter));

        int modelVersionNumber = std::stoi(modelVersion);
        if (modelVersionNumber < referenceModelVersion)
        {
            isValid = false;
        }

        // Get build date from versionInfo
        int position = 0;
        for (int i = 0; i < 3; i++)
        {
            position = versionInfo.find(semanticVersionDelimeter, position + 1);
        }

        std::string buildDate = versionInfo.substr(position + 1, position + dateLength);
        int year = std::stoi(buildDate.substr(0, 4));
        int month = std::stoi(buildDate.substr(4, 2));
        int day = std::stoi(buildDate.substr(6, 2));

        if ((month < 1) || (month > 12) || (day < 1) || (day > 31))
        {
            isValid = false;
        }

        char dateNow[dateLength] = { 0 };
        int monthNow = 0, dayNow = 0, yearNow = 0;
        time_t t = time(0);
        strftime(dateNow, dateLength, "%Y%m%d", localtime(&t));
        sscanf(dateNow, "%4d%2d%2d", &yearNow, &monthNow, &dayNow);

        // Check if the build date is in the future
        if ((yearNow < year) || ((yearNow == year) && ((monthNow < month) || ((monthNow == month) && (dayNow < day)))))
        {
            isValid = false;
        }

        // Check if the build date is from the past - before the reference release date
        if ((year < referenceReleaseYear) || ((year == referenceReleaseYear) && ((month < referenceReleaseMonth) || ((month == referenceReleaseMonth) && (day < referenceReleaseDay)))))
        {
            isValid = false;
        }
    }
    else
    {
        isValid = false;
    }

    return isValid;
}

OSCONFIG_LOG_HANDLE ZtsiLog::m_log = nullptr;

Ztsi::Ztsi(std::string filePath, unsigned int maxPayloadSizeBytes)
{
    m_agentConfigurationFile = filePath;
    m_agentConfigurationDir = filePath.substr(0, filePath.find_last_of("/"));
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_lastAvailableConfiguration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay, g_defaultMaxManualAttestationsPerDay};
    m_lastEnabledState = false;
}

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Ztsi",
    "Description": "Provides functionality to remotely configure the ZTSI Agent on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Ztsi"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

int Ztsi::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(ZtsiLog::Get(), "GetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(ZtsiLog::Get(), "GetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ZtsiLog::Get(), "GetInfo called with null payloadSizeBytes");
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
                OsConfigLogError(ZtsiLog::Get(), "GetInfo failed to allocate memory");
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
            OsConfigLogError(ZtsiLog::Get(), "GetInfo exception thrown: %s", e.what());
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
    }

    return status;
}

int Ztsi::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == componentName)
    {
        OsConfigLogError(ZtsiLog::Get(), "Get called with null componentName");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(ZtsiLog::Get(), "Get called with null objectName");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ZtsiLog::Get(), "Get called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        unsigned int maxPayloadSizeBytes = GetMaxPayloadSizeBytes();
        rapidjson::Document document;
        if (0 == Ztsi::m_componentName.compare(componentName))
        {
            if (0 == Ztsi::m_reportedEnabled.compare(objectName))
            {
                Ztsi::EnabledState enabledState = GetEnabledState();
                document.SetInt(static_cast<int>(enabledState));
                status = SerializeJsonObject(payload, payloadSizeBytes, maxPayloadSizeBytes, document);
            }
            else if (0 == Ztsi::m_reportedMaxManualAttestationsPerDay.compare(objectName))
            {
                int maxManualAttestationsPerDay = GetMaxManualAttestationsPerDay();
                document.SetInt(maxManualAttestationsPerDay);
                status = SerializeJsonObject(payload, payloadSizeBytes, maxPayloadSizeBytes, document);
            }

            else if (0 == Ztsi::m_reportedMaxScheduledAttestationsPerDay.compare(objectName))
            {
                int maxScheduledAttestationsPerDay = GetMaxScheduledAttestationsPerDay();
                document.SetInt(maxScheduledAttestationsPerDay);
                status = SerializeJsonObject(payload, payloadSizeBytes, maxPayloadSizeBytes, document);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int Ztsi::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    rapidjson::Document document;

    if (nullptr == componentName)
    {
        OsConfigLogError(ZtsiLog::Get(), "Set called with null componentName");
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(ZtsiLog::Get(), "Set called with null objectName");
        status = EINVAL;
    }
    else if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(ZtsiLog::Get(), "Unabled to parse JSON payload");
        status = EINVAL;
    }
    else
    {
        if (0 == Ztsi::m_componentName.compare(componentName))
        {
            if (0 == Ztsi::m_desiredEnabled.compare(objectName))
            {
                if (document.IsBool())
                {
                    status = SetEnabled(document.GetBool());
                }
                else
                {
                    OsConfigLogError(ZtsiLog::Get(), "'%s' is not of type boolean", Ztsi::m_desiredEnabled.c_str());
                    status = EINVAL;
                }
            }
            else if (0 == Ztsi::m_desiredMaxScheduledAttestationsPerDay.compare(objectName))
            {
                if (document.IsInt())
                {
                    status = SetMaxScheduledAttestationsPerDay(document.GetInt());
                }
                else
                {
                    OsConfigLogError(ZtsiLog::Get(), "'%s' is not of type int", Ztsi::m_desiredMaxScheduledAttestationsPerDay.c_str());
                    status = EINVAL;
                }
            }
            else if (0 == Ztsi::m_desiredMaxManualAttestationsPerDay.compare(objectName))
            {
                if (document.IsInt())
                {
                    status = SetMaxManualAttestationsPerDay(document.GetInt());
                }
                else
                {
                    OsConfigLogError(ZtsiLog::Get(), "'%s' is not of type int", Ztsi::m_desiredMaxManualAttestationsPerDay.c_str());
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid objectName: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Invalid componentName: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

unsigned int Ztsi::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

Ztsi::EnabledState Ztsi::GetEnabledState()
{
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay , g_defaultMaxManualAttestationsPerDay};
    return (MMI_OK == ReadAgentConfiguration(configuration)) ? (configuration.enabled ? EnabledState::Enabled : EnabledState::Disabled) : EnabledState::Unknown;
}

int Ztsi::GetMaxScheduledAttestationsPerDay()
{
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay , g_defaultMaxManualAttestationsPerDay};
    return (MMI_OK == ReadAgentConfiguration(configuration)) ? configuration.maxScheduledAttestationsPerDay : g_defaultMaxScheduledAttestationsPerDay;
}

int Ztsi::GetMaxManualAttestationsPerDay()
{
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay , g_defaultMaxManualAttestationsPerDay};
    return (MMI_OK == ReadAgentConfiguration(configuration)) ? configuration.maxManualAttestationsPerDay : g_defaultMaxManualAttestationsPerDay;
}

int Ztsi::SetEnabled(bool enabled)
{
    int status = MMI_OK;
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay , g_defaultMaxManualAttestationsPerDay};
    m_lastEnabledState = enabled;

    status = ReadAgentConfiguration(configuration);
    if ((MMI_OK == status) || (EINVAL == status))
    {
        // Check if the state is already set to the desired state
        if (enabled != configuration.enabled)
        {
            configuration.enabled = enabled;
            status = IsValidConfiguration(configuration) ? WriteAgentConfiguration(configuration) : EINVAL;
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired enabled state
        configuration.enabled = enabled;
        status = IsValidConfiguration(configuration) ? CreateConfigurationFile(configuration) : EINVAL;
    }

    return status;
}

int Ztsi::SetMaxScheduledAttestationsPerDay(int maxScheduledAttestationsPerDay)
{
    int status = MMI_OK;
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay, g_defaultMaxManualAttestationsPerDay};
    status = ReadAgentConfiguration(configuration);
    if ((MMI_OK == status) || (EINVAL == status))
    {
        if (maxScheduledAttestationsPerDay != configuration.maxScheduledAttestationsPerDay)
        {
            configuration.enabled = m_lastEnabledState;
            configuration.maxScheduledAttestationsPerDay = maxScheduledAttestationsPerDay;
            status = IsValidConfiguration(configuration) ? WriteAgentConfiguration(configuration) : EINVAL;
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired maxScheduledAttestationsPerDay state
        configuration.enabled = m_lastEnabledState;
        configuration.maxScheduledAttestationsPerDay = maxScheduledAttestationsPerDay;
        status = IsValidConfiguration(configuration) ? CreateConfigurationFile(configuration) : EINVAL;
    }

    return status;
}

int Ztsi::SetMaxManualAttestationsPerDay(int maxManualAttestationsPerDay)
{
    int status = MMI_OK;
    AgentConfiguration configuration = {g_defaultEnabled, g_defaultMaxScheduledAttestationsPerDay, g_defaultMaxManualAttestationsPerDay};

    status = ReadAgentConfiguration(configuration);
    if ((MMI_OK == status) || (EINVAL == status))
    {
        if (maxManualAttestationsPerDay != configuration.maxManualAttestationsPerDay)
        {
            configuration.enabled = m_lastEnabledState;
            configuration.maxManualAttestationsPerDay = maxManualAttestationsPerDay;
            status = IsValidConfiguration(configuration) ? WriteAgentConfiguration(configuration) : EINVAL;
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired maxManualAttestationsPerDay state
        configuration.enabled = m_lastEnabledState;
        configuration.maxManualAttestationsPerDay = maxManualAttestationsPerDay;
        status = IsValidConfiguration(configuration) ? CreateConfigurationFile(configuration) : EINVAL;
    }

    return status;
}

bool Ztsi::IsValidConfiguration(const Ztsi::AgentConfiguration& configuration)
{
    bool isValid = true;

    if (configuration.maxManualAttestationsPerDay < 0 || configuration.maxScheduledAttestationsPerDay < 0)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ZtsiLog::Get(), "MaxManualAttestationsPerDay and MaxScheduledAttestationsPerDay must both be nonnegative");
        }

        isValid = false;
    }

    if (configuration.maxManualAttestationsPerDay + configuration.maxScheduledAttestationsPerDay > g_totalAttestationsAllowedPerDay)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ZtsiLog::Get(), "The total number of attestations per day (Scheduled + Manual) cannot exceed %s", std::to_string(g_totalAttestationsAllowedPerDay).c_str());
        }

        isValid = false;
    }

    return isValid;
}

std::FILE* Ztsi::OpenAndLockFile(const char* mode)
{
    std::FILE* file = nullptr;

    if (nullptr != (file = fopen(m_agentConfigurationFile.c_str(), mode)))
    {
        if (!LockFile(file, ZtsiLog::Get()))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "Failed to lock file %s", m_agentConfigurationFile.c_str());
            }
            fclose(file);
            file = nullptr;
        }
    }

    return file;
}

std::FILE* Ztsi::OpenAndLockFile(const char* mode, unsigned int milliseconds, int count)
{
    int i = 0;
    time_t seconds = milliseconds / 1000;
    long nanoseconds = (milliseconds % 1000) * 1000000;
    struct timespec lockTimeToSleep = {seconds, nanoseconds};
    std::FILE* file = nullptr;

    while ((nullptr == (file = OpenAndLockFile(mode))) && (i < count))
    {
        nanosleep(&lockTimeToSleep, nullptr);
        i++;
    }

    return file;
}

void Ztsi::CloseAndUnlockFile(std::FILE* file)
{
    if ((nullptr != file))
    {
        fflush(file);
        UnlockFile(file, ZtsiLog::Get());
        fclose(file);
    }
}

int Ztsi::ReadAgentConfiguration(AgentConfiguration& configuration)
{
    int status = MMI_OK;
    std::string configurationJson;
    std::FILE* file = nullptr;
    long fileSize = 0;
    size_t bytesRead = 0;
    char* buffer = nullptr;

    if (FileExists(m_agentConfigurationFile.c_str()))
    {
        if (nullptr != (file = OpenAndLockFile("r")))
        {
            fseek(file, 0, SEEK_END);
            fileSize = ftell(file);
            rewind(file);

            buffer = new (std::nothrow) char[fileSize + 1];
            if (nullptr != buffer)
            {
                bytesRead = fread(buffer, 1, fileSize, file);
                if ((0 < fileSize) && (bytesRead == static_cast<unsigned>(fileSize)))
                {
                    buffer[fileSize] = '\0';
                    configurationJson = buffer;

                    if (MMI_OK == (status = ParseAgentConfiguration(configurationJson, configuration)))
                    {
                        // Cache the last available agent configuration
                        m_lastAvailableConfiguration = configuration;
                    }
                }
                else
                {
                    OsConfigLogError(ZtsiLog::Get(), "Failed to read configuration file %s", m_agentConfigurationFile.c_str());
                    status = EIO;
                }

                delete[] buffer;
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Failed to allocate memory for configuration file %s", m_agentConfigurationFile.c_str());
                status = ENOMEM;
            }

            CloseAndUnlockFile(file);
        }
        else
        {
            // The file is temporarily unavailable (locked) by another process
            // Return the last available configuration, with success status
            configuration = m_lastAvailableConfiguration;
            status = MMI_OK;
        }
    }
    else
    {
        status = ENOENT;
    }

    return status;
}

int Ztsi::ParseAgentConfiguration(const std::string& configurationJson, Ztsi::AgentConfiguration& configuration)
{
    int status = MMI_OK;
    rapidjson::Document document;

    if (document.Parse(configurationJson.c_str()).HasParseError())
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to parse JSON %s", configurationJson.c_str());
        }
        status = EINVAL;
    }
    else
    {
        if (document.HasMember(g_configurationPropertyEnabled))
        {
            if (document[g_configurationPropertyEnabled].IsBool())
            {
                configuration.enabled = document[g_configurationPropertyEnabled].GetBool();
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configurationPropertyEnabled);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configurationPropertyEnabled, m_agentConfigurationFile.c_str());
            status = EINVAL;
        }

        if (document.HasMember(g_configurationPropertyMaxScheduledAttestationsPerDay))
        {
            if (document[g_configurationPropertyMaxScheduledAttestationsPerDay].IsInt())
            {
                configuration.maxScheduledAttestationsPerDay = document[g_configurationPropertyMaxScheduledAttestationsPerDay].GetInt();
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configurationPropertyMaxScheduledAttestationsPerDay);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configurationPropertyMaxScheduledAttestationsPerDay, m_agentConfigurationFile.c_str());
            status = EINVAL;
        }

        if (document.HasMember(g_configurationPropertyMaxManualAttestationsPerDay))
        {
            if (document[g_configurationPropertyMaxManualAttestationsPerDay].IsInt())
            {
                configuration.maxManualAttestationsPerDay = document[g_configurationPropertyMaxManualAttestationsPerDay].GetInt();
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configurationPropertyMaxManualAttestationsPerDay);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configurationPropertyMaxManualAttestationsPerDay, m_agentConfigurationFile.c_str());
            status = EINVAL;
        }
    }

    return status;
}

int Ztsi::WriteAgentConfiguration(const Ztsi::AgentConfiguration& configuration)
{
    int status = MMI_OK;
    std::FILE* file = nullptr;

    if (nullptr != (file = OpenAndLockFile("r+", g_lockWait, g_lockWaitMaxRetries)))
    {
        std::string configurationJson = BuildConfigurationJson(configuration);

        int rc = std::fwrite(configurationJson.c_str(), 1, configurationJson.length(), file);
        if ((0 > rc) || (EOF == rc))
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to write to file %s", m_agentConfigurationFile.c_str());
            status = errno ? errno : EINVAL;
        }
        else
        {
            ftruncate(fileno(file), rc);
            m_lastAvailableConfiguration = configuration;
        }

        CloseAndUnlockFile(file);
    }
    else
    {
        status = errno;
    }

    return status;
}

int Ztsi::CreateConfigurationFile(const AgentConfiguration& configuration)
{
    int status = MMI_OK;
    struct stat sb;

    // Create /etc/sim-agent-edge/ if it does not exist
    if (0 != stat(m_agentConfigurationDir.c_str(), &sb))
    {
        if (0 == mkdir(m_agentConfigurationDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR))
        {
            RestrictFileAccessToCurrentAccountOnly(m_agentConfigurationDir.c_str());
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to create directory %s", m_agentConfigurationDir.c_str());
            status = errno;
        }
    }

    // Create /etc/sim-agent-edge/agent.conf if it does not exist
    if (0 != stat(m_agentConfigurationFile.c_str(), &sb))
    {
        std::ofstream newFile(m_agentConfigurationFile, std::ios::out | std::ios::trunc);
        if (newFile.good())
        {
            RestrictFileAccessToCurrentAccountOnly(m_agentConfigurationFile.c_str());
            std::string configurationJson = BuildConfigurationJson(configuration);
            newFile << configurationJson;
            newFile.close();

            m_lastAvailableConfiguration = configuration;
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to create file %s", m_agentConfigurationFile.c_str());
            status = errno;
        }
    }

    return status;
}

std::string Ztsi::BuildConfigurationJson(const AgentConfiguration& configuration)
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key(g_configurationPropertyEnabled);
    writer.Bool(configuration.enabled);

    writer.Key(g_configurationPropertyMaxScheduledAttestationsPerDay);
    writer.Int(configuration.maxScheduledAttestationsPerDay);

    writer.Key(g_configurationPropertyMaxManualAttestationsPerDay);
    writer.Int(configuration.maxManualAttestationsPerDay);

    writer.EndObject();

    return buffer.GetString();
}
