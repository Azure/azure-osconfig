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
#include "Ztsi.h"

static const char g_configurationPropertyEnabled[] = "enabled";
static const char g_configurationPropertyServiceUrl[] = "serviceUrl";

static const Ztsi::EnabledState g_defaultEnabledState = Ztsi::EnabledState::Unknown;
static const bool g_defaultEnabled = false;
static const std::string g_defaultServiceUrl = "";

static const std::string g_urlRegex = "((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]";

// Block for a maximum of (20 milliseconds x 5 retries) 100ms
static const unsigned int g_lockWaitMillis = 20;
static const unsigned int g_lockWaitMaxRetries = 5;

// Regex for validating client name 'Azure OSConfig <model version>;<major>.<minor>.<patch>.<yyyymmdd><build>'
static const std::string g_clientNameRegex = "^((Azure OSConfig )[1-9];(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.([0-9]{8})).*$";
static const std::string g_clientNamePrefix = "Azure OSConfig ";
static const std::string g_modelVersionDelimiter = ";";
static const std::string g_semanticVersionDelimeter = ".";

// DTDL version 5 published with ZTSI on September 27, 2021
static const int g_initialModelVersion = 5;
static const int g_initialReleaseDay = 27;
static const int g_initialReleaseMonth = 9;
static const int g_initialReleaseYear = 2021;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

bool IsValidClientName(const std::string& clientName)
{
    bool isValid = true;

    std::regex pattern(g_clientNameRegex);
    if (!clientName.empty() && std::regex_match(clientName, pattern))
    {
        std::string versionInfo = clientName.substr(g_clientNamePrefix.length());
        std::string modelVersion = versionInfo.substr(0, versionInfo.find(g_modelVersionDelimiter));

        int modelVersionNumber = std::stoi(modelVersion);
        if (modelVersionNumber < g_initialModelVersion)
        {
            isValid = false;
        }

        // Get build date from versionInfo
        int position = 0;
        for (int i = 0; i < 3; i++)
        {
            position = versionInfo.find(g_semanticVersionDelimeter, position + 1);
        }

        std::string buildDate = versionInfo.substr(position + 1, position + DATE_FORMAT_LENGTH);
        int year = std::stoi(buildDate.substr(0, 4));
        int month = std::stoi(buildDate.substr(4, 2));
        int day = std::stoi(buildDate.substr(6, 2));

        if ((month < 1) || (month > 12) || (day < 1) || (day > 31))
        {
            isValid = false;
        }

        char dateNow[DATE_FORMAT_LENGTH] = {0};
        int monthNow, dayNow, yearNow;
        time_t t = time(0);
        strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));
        sscanf(dateNow, SSCANF_DATE_FORMAT, &yearNow, &monthNow, &dayNow);

        // Check if the build date is in the future
        if ((yearNow < year) || ((yearNow == year) && ((monthNow < month) || ((monthNow == month) && (dayNow < day)))))
        {
            isValid = false;
        }

        // Check if the build date is before the initial release date
        if ((year < g_initialReleaseYear) || ((year == g_initialReleaseYear) && ((month < g_initialReleaseMonth) || ((month == g_initialReleaseMonth) && (day < g_initialReleaseDay)))))
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
    m_lastAvailableConfiguration = {g_defaultServiceUrl, g_defaultEnabled};
}

unsigned int Ztsi::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

Ztsi::EnabledState Ztsi::GetEnabledState()
{
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};
    return (0 == ReadAgentConfiguration(configuration)) ? (configuration.enabled ? EnabledState::Enabled : EnabledState::Disabled) : g_defaultEnabledState;
}

std::string Ztsi::GetServiceUrl()
{
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};
    return (0 == ReadAgentConfiguration(configuration)) ? configuration.serviceUrl : g_defaultServiceUrl;
}

int Ztsi::SetEnabled(bool enabled)
{
    int status = 0;
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};

    status = ReadAgentConfiguration(configuration);
    if ((0 == status) || (EINVAL == status))
    {
        // Check if the state is already set to the desired state
        if (enabled != configuration.enabled)
        {
            configuration.enabled = enabled;
            status = WriteAgentConfiguration(configuration);
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired enabled state
        configuration.enabled = enabled;
        configuration.serviceUrl = g_defaultServiceUrl;
        status = CreateConfigurationFile(configuration);
    }

    return status;
}

int Ztsi::SetServiceUrl(const std::string& serviceUrl)
{
    int status = 0;
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};

    status = ReadAgentConfiguration(configuration);
    if ((0 == status) || (EINVAL == status))
    {
        if (serviceUrl != configuration.serviceUrl)
        {
            configuration.serviceUrl = serviceUrl;
            status = WriteAgentConfiguration(configuration);
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired serviceUrl
        configuration.enabled = g_defaultEnabledState;
        configuration.serviceUrl = serviceUrl;
        status = CreateConfigurationFile(configuration);
    }

    return status;
}

bool Ztsi::IsValidConfiguration(const Ztsi::AgentConfiguration& configuration)
{
    bool isValid = true;

    if (configuration.serviceUrl.empty() && configuration.enabled)
    {
        OsConfigLogError(ZtsiLog::Get(), "ServiceUrl is empty and enabled is true");
        isValid = false;
    }

    std::regex urlPattern(g_urlRegex);
    if (!configuration.serviceUrl.empty() && !regex_match(configuration.serviceUrl, urlPattern))
    {
        OsConfigLogError(ZtsiLog::Get(), "Invalid serviceUrl '%s'", configuration.serviceUrl.c_str());
        isValid = false;
    }

    return isValid;
}

bool Ztsi::FileExists(const std::string& filePath)
{
    struct stat sb;
    return (0 == stat(filePath.c_str(), &sb) && S_ISREG(sb.st_mode));
}

std::FILE* Ztsi::LockFile(const char* mode)
{
    int fd = -1;
    std::FILE* fp = nullptr;

    if (FileExists(m_agentConfigurationFile) && (nullptr != (fp = fopen(m_agentConfigurationFile.c_str(), mode))))
    {
        if (0 == (fd = fileno(fp)))
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to get file descriptor for %s", m_agentConfigurationFile.c_str());
        }
        else if (0 != flock(fd, LOCK_EX | LOCK_NB))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "Failed to lock file %s", m_agentConfigurationFile.c_str());
            }
            fclose(fp);
            fp = nullptr;
        }
    }

    return fp;
}

std::FILE* Ztsi::LockFile(const char* mode, unsigned int milliseconds, int count)
{
    int i = 0;
    time_t seconds = milliseconds / 1000;
    long nanoseconds = (milliseconds % 1000) * 1000000;
    struct timespec lockTimeToSleep = {seconds, nanoseconds};
    std::FILE* fp = nullptr;

    while ((nullptr == (fp = LockFile(mode))) && (i < count))
    {
        nanosleep(&lockTimeToSleep, nullptr);
        i++;
    }

    return fp;
}

void Ztsi::UnlockFile(std::FILE* fp)
{
    if ((nullptr != fp))
    {
        fflush(fp);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
    }
}

int Ztsi::ReadAgentConfiguration(AgentConfiguration& configuration)
{
    int status = 0;
    std::string configurationJson;
    std::FILE* fp = nullptr;
    long fileSize = 0;
    size_t bytesRead = 0;
    char* buffer = nullptr;

    if (FileExists(m_agentConfigurationFile))
    {
        if (nullptr != (fp = LockFile("r")))
        {
            fseek(fp, 0, SEEK_END);
            fileSize = ftell (fp);
            rewind(fp);

            buffer = new (std::nothrow) char[fileSize + 1];
            if (nullptr != buffer)
            {
                bytesRead = fread(buffer, 1, fileSize, fp);
                if ((0 < fileSize) && (bytesRead == static_cast<unsigned>(fileSize)))
                {
                    buffer[fileSize] = '\0';
                    configurationJson = buffer;

                    if (0 == (status = ParseAgentConfiguration(configurationJson, configuration)))
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


            m_lastAvailableConfiguration = configuration;

            UnlockFile(fp);
        }
        else
        {
            // The file is temporarily unavailable (locked) by another process
            // Return the last available configuration, with success status
            configuration = m_lastAvailableConfiguration;
            status = 0;
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
    int status = 0;
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

        if (document.HasMember(g_configurationPropertyServiceUrl))
        {
            if (document[g_configurationPropertyServiceUrl].IsString())
            {
                configuration.serviceUrl = document[g_configurationPropertyServiceUrl].GetString();
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configurationPropertyServiceUrl);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configurationPropertyServiceUrl, m_agentConfigurationFile.c_str());
            status = EINVAL;
        }
    }

    return status;
}

int Ztsi::WriteAgentConfiguration(const Ztsi::AgentConfiguration& configuration)
{
    int status = 0;
    std::FILE* fp = nullptr;

    if (Ztsi::IsValidConfiguration(configuration))
    {
        if (nullptr != (fp = LockFile("r+", g_lockWaitMillis, g_lockWaitMaxRetries)))
        {
            std::string configurationJson = BuildConfigurationJson(configuration);

            int rc = std::fwrite(configurationJson.c_str(), 1, configurationJson.length(), fp);
            if ((0 > rc) || (EOF == rc))
            {
                OsConfigLogError(ZtsiLog::Get(), "Failed to write to file %s", m_agentConfigurationFile.c_str());
                status = errno ? errno : EINVAL;
            }
            else
            {
                ftruncate(fileno(fp), rc);
                m_lastAvailableConfiguration = configuration;
            }

            UnlockFile(fp);
        }
        else
        {
            status = errno;
        }
    }
    else
    {
        status = EINVAL;
    }

    return status;
}

int Ztsi::CreateConfigurationFile(const AgentConfiguration& configuration)
{
    int status = 0;
    struct stat sb;

    if (IsValidConfiguration(configuration))
    {
        // Create /etc/ztsi/ if it does not exist
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

        // Create /etc/ztsi/agent.conf if it does not exist
        if (0 != stat(m_agentConfigurationFile.c_str(), &sb))
        {
            std::ofstream newFile(m_agentConfigurationFile, std::ios::out | std::ios::trunc);
            if (newFile.good())
            {
                std::string configurationJson = BuildConfigurationJson(configuration);
                newFile << configurationJson;
                newFile.close();
                RestrictFileAccessToCurrentAccountOnly(m_agentConfigurationFile.c_str());

                m_lastAvailableConfiguration = configuration;
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "Failed to create file %s", m_agentConfigurationFile.c_str());
                status = errno;
            }
        }
    }
    else
    {
        status = EINVAL;
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

    writer.Key(g_configurationPropertyServiceUrl);
    writer.String(configuration.serviceUrl.c_str());

    writer.EndObject();

    return buffer.GetString();
}
