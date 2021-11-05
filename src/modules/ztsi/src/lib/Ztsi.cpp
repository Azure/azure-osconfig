// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sys/stat.h>

#include "CommonUtils.h"
#include "Ztsi.h"

static const char g_configPropertyEnabled[] = "enabled";
static const char g_configPropertyServiceUrl[] = "serviceUrl";

static const Ztsi::EnabledState g_defaultEnabledState = Ztsi::EnabledState::Unknown;
static const bool g_defaultEnabled = false;
static const std::string g_defaultServiceUrl = "";

static const std::string g_urlRegex = "((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]";

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
    m_agentConfigFile = filePath;
    m_agentConfigDir = filePath.substr(0, filePath.find_last_of("/"));
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

unsigned int Ztsi::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

Ztsi::EnabledState Ztsi::GetEnabledState()
{
    AgentConfig config = {g_defaultServiceUrl, g_defaultEnabled};
    return (0 == ReadAgentConfig(config)) ? (config.enabled ? EnabledState::Enabled : EnabledState::Disabled) : g_defaultEnabledState;
}

std::string Ztsi::GetServiceUrl()
{
    AgentConfig config = {g_defaultServiceUrl, g_defaultEnabled};
    return (0 == Ztsi::ReadAgentConfig(config)) ? config.serviceUrl : g_defaultServiceUrl;
}

int Ztsi::SetEnabled(bool enabled)
{
    int status = 0;
    AgentConfig config = {g_defaultServiceUrl, g_defaultEnabled};

    status = ReadAgentConfig(config);
    if (0 == status)
    {
        // Check if the state is already set to the desired state
        if (enabled != config.enabled)
        {
            config.enabled = enabled;
            status = WriteAgentConfig(config);
        }
    }
    else if (ENOENT == status)
    {
        // If the config file doesn't exist, create it with the desired enabled state and default serviceUrl
        config.enabled = enabled;
        config.serviceUrl = g_defaultServiceUrl;
        status = WriteAgentConfig(config);
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "Failed to set enabled property");
    }

    return status;
}

int Ztsi::SetServiceUrl(const std::string& serviceUrl)
{
    int status = 0;
    AgentConfig config = { g_defaultServiceUrl, g_defaultEnabled };

    status = ReadAgentConfig(config);
    if (0 == status)
    {
        // Check if the serviceUrl is already set to the desired value
        if (serviceUrl != config.serviceUrl)
        {
            config.serviceUrl = serviceUrl;
            status = WriteAgentConfig(config);
        }
    }
    else if (ENOENT == status)
    {
        // If the config file doesn't exist, create it with the desired serviceUrl
        config.enabled = g_defaultEnabledState;
        config.serviceUrl = serviceUrl;
        status = WriteAgentConfig(config);
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "Failed to set serviceUrl property");
    }

    return status;
}

bool Ztsi::IsValidConfig(const Ztsi::AgentConfig& config)
{
    bool isValid = true;

    if (config.serviceUrl.empty() && config.enabled)
    {
        OsConfigLogError(ZtsiLog::Get(), "ServiceUrl is empty and enabled is true");
        isValid = false;
    }

    std::regex urlPattern(g_urlRegex);
    if (!config.serviceUrl.empty() && !regex_match(config.serviceUrl, urlPattern))
    {
        OsConfigLogError(ZtsiLog::Get(), "Invalid serviceUrl '%s'", config.serviceUrl.c_str());
        isValid = false;
    }

    return isValid;
}

int Ztsi::ReadAgentConfig(Ztsi::AgentConfig& config)
{
    int status = 0;

    std::ifstream file(m_agentConfigFile);
    if (!file.good())
    {
        status = ENOENT;
        return status;
    }

    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document document;
    if (document.ParseStream(isw).HasParseError())
    {
        OsConfigLogError(ZtsiLog::Get(), "Failed to parse config file: %s", m_agentConfigFile.c_str());
        status = EINVAL;
        return status;
    }

    if (document.HasMember(g_configPropertyEnabled))
    {
        if (document[g_configPropertyEnabled].IsBool())
        {
            config.enabled = document[g_configPropertyEnabled].GetBool();
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configPropertyEnabled);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configPropertyEnabled, m_agentConfigFile.c_str());
        status = EINVAL;
    }

    if (document.HasMember(g_configPropertyServiceUrl))
    {
        if (document[g_configPropertyServiceUrl].IsString())
        {
            config.serviceUrl = document[g_configPropertyServiceUrl].GetString();
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Invalid value for %s", g_configPropertyServiceUrl);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ZtsiLog::Get(), "Missing field '%s' in file %s", g_configPropertyServiceUrl, m_agentConfigFile.c_str());
        status = EINVAL;
    }

    return status;
}

int Ztsi::ConfigFileExists()
{
    int status = 0;
    struct stat st;

    // Create /etc/ztsi if it does not exist
    if (0 != stat(m_agentConfigDir.c_str(), &st))
    {
        if (0 == mkdir(m_agentConfigDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR))
        {
            RestrictFileAccessToCurrentAccountOnly(m_agentConfigDir.c_str());
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to create directory %s", m_agentConfigDir.c_str());
            status = errno;
        }
    }

    // Create /etc/ztsi/agent.conf if it does not exist
    if (0 != stat(m_agentConfigFile.c_str(), &st))
    {
        std::ofstream newFile(m_agentConfigFile, std::ios::out | std::ios::trunc);
        if (newFile.good())
        {
            newFile.close();
            RestrictFileAccessToCurrentAccountOnly(m_agentConfigFile.c_str());
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to create file %s", m_agentConfigFile.c_str());
            status = errno;
        }
    }

    return status;
}

int Ztsi::WriteAgentConfig(const Ztsi::AgentConfig& config)
{
    int status = 0;

    if (!Ztsi::IsValidConfig(config))
    {
        OsConfigLogError(ZtsiLog::Get(), "Invalid config");
        status = EINVAL;
        return status;
    }

    if (0 != (status = ConfigFileExists()))
    {
        OsConfigLogError(ZtsiLog::Get(), "File %s does not exist and could not be created", m_agentConfigFile.c_str());
        return status;
    }


    std::FILE* file = std::fopen(m_agentConfigFile.c_str(), "w");
    if (nullptr == file)
    {
        OsConfigLogError(ZtsiLog::Get(), "Failed to open file %s", m_agentConfigFile.c_str());
        status = errno;
    }
    else
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

        writer.StartObject();

        writer.Key(g_configPropertyEnabled);
        writer.Bool(config.enabled);

        writer.Key(g_configPropertyServiceUrl);
        writer.String(config.serviceUrl.c_str());

        writer.EndObject();

        int rc = std::fwrite(buffer.GetString(), 1, buffer.GetSize(), file);
        if ((0 > rc) || (EOF == rc))
        {
            OsConfigLogError(ZtsiLog::Get(), "Failed to write to file %s", m_agentConfigFile.c_str());
            status = errno ? errno : EINVAL;
        }
        else
        {
            fflush(file);
            std::fclose(file);
        }
    }

    return status;
}
