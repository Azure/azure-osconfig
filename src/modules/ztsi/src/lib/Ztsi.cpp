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
static const char g_configurationPropertyServiceUrl[] = "serviceUrl";

static const bool g_defaultEnabled = false;
static const std::string g_defaultServiceUrl = "";

// Block for a maximum of (20 milliseconds x 5 retries) 100ms
static const unsigned int g_lockWait = 20;
static const unsigned int g_lockWaitMaxRetries = 5;

const std::string Ztsi::m_componentName = "Ztsi";
const std::string Ztsi::m_desiredServiceUrl = "DesiredServiceUrl";
const std::string Ztsi::m_desiredEnabled = "DesiredEnabled";
const std::string Ztsi::m_reportedServiceUrl = "ServiceUrl";
const std::string Ztsi::m_reportedEnabled = "Enabled";

int SerializeJsonObject(MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, rapidjson::Document& document)
{
    int status = MMI_OK;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    if (buffer.GetSize() > maxPayloadSizeBytes)
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

OSCONFIG_LOG_HANDLE ZtsiLog::m_log = nullptr;

Ztsi::Ztsi(std::string filePath, unsigned int maxPayloadSizeBytes)
{
    m_agentConfigurationFile = filePath;
    m_agentConfigurationDir = filePath.substr(0, filePath.find_last_of("/"));
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
    m_lastAvailableConfiguration = {g_defaultServiceUrl, g_defaultEnabled};
    m_lastEnabledState = false;
}

int Ztsi::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    constexpr const char info[] = R""""({
        "Name": "Ztsi",
        "Description": "Provides functionality to remotely configure the ZTSI Agent on device",
        "Manufacturer": "Microsoft",
        "VersionMajor": 1,
        "VersionMinor": 0,
        "VersionInfo": "Nickel",
        "Components": ["Ztsi"],
        "Lifetime": 1,
        "UserAccount": 0})"""";

    if (nullptr == clientName)
    {
        OsConfigLogError(ZtsiLog::Get(), "GetInfo called with null clientName");
        status = EINVAL;
    }
    else if (!IsValidClientName(clientName))
    {
        OsConfigLogError(ZtsiLog::Get(), "GetInfo called with invalid clientName");
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
            std::size_t len = ARRAY_SIZE(info) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(ZtsiLog::Get(), "GetInfo failed to allocate memory");
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
            else if (0 == Ztsi::m_reportedServiceUrl.compare(objectName))
            {
                std::string serviceUrl = GetServiceUrl();
                document.SetString(serviceUrl.c_str(), document.GetAllocator());
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
            else if (0 == Ztsi::m_desiredServiceUrl.compare(objectName))
            {
                if (document.IsString())
                {
                    status = SetServiceUrl(document.GetString());
                }
                else
                {
                    OsConfigLogError(ZtsiLog::Get(), "'%s' is not of type string", Ztsi::m_desiredServiceUrl.c_str());
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
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};
    return (MMI_OK == ReadAgentConfiguration(configuration)) ? (configuration.enabled ? EnabledState::Enabled : EnabledState::Disabled) : EnabledState::Unknown;
}

std::string Ztsi::GetServiceUrl()
{
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};
    return (MMI_OK == ReadAgentConfiguration(configuration)) ? configuration.serviceUrl : g_defaultServiceUrl;
}

int Ztsi::SetEnabled(bool enabled)
{
    int status = MMI_OK;
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};
    m_lastEnabledState = enabled;

    status = ReadAgentConfiguration(configuration);
    if ((MMI_OK == status) || (EINVAL == status))
    {
        // Check if the state is already set to the desired state
        if (enabled != configuration.enabled)
        {
            configuration.enabled = enabled;
            status = IsValidConfiguration(configuration) ? WriteAgentConfiguration(configuration) : MMI_OK;
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired enabled state
        configuration.enabled = enabled;
        configuration.serviceUrl = g_defaultServiceUrl;
        status = IsValidConfiguration(configuration) ? CreateConfigurationFile(configuration) : MMI_OK;
    }

    return status;
}

int Ztsi::SetServiceUrl(const std::string& serviceUrl)
{
    int status = MMI_OK;
    AgentConfiguration configuration = {g_defaultServiceUrl, g_defaultEnabled};

    status = ReadAgentConfiguration(configuration);
    if ((MMI_OK == status) || (EINVAL == status))
    {
        if (serviceUrl != configuration.serviceUrl)
        {
            configuration.enabled = m_lastEnabledState;
            configuration.serviceUrl = serviceUrl;
            status = IsValidConfiguration(configuration) ? WriteAgentConfiguration(configuration) : EINVAL;
        }
    }
    else if (ENOENT == status)
    {
        // If the configuration file doesn't exist, create it with the desired serviceUrl
        configuration.enabled = m_lastEnabledState;
        configuration.serviceUrl = serviceUrl;
        status = IsValidConfiguration(configuration) ? CreateConfigurationFile(configuration) : EINVAL;
    }

    return status;
}

bool Ztsi::IsValidConfiguration(const Ztsi::AgentConfiguration& configuration)
{
    bool isValid = true;

    if (configuration.serviceUrl.empty() && configuration.enabled)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ZtsiLog::Get(), "ServiceUrl is empty and enabled is true");
        }

        isValid = false;
    }

    std::regex urlPattern("((http|https)://)(www.)?[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");
    if (!configuration.serviceUrl.empty() && !regex_match(configuration.serviceUrl, urlPattern))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ZtsiLog::Get(), "Invalid serviceUrl '%s'", configuration.serviceUrl.c_str());
        }

        isValid = false;
    }
    return isValid;
}

std::FILE* Ztsi::OpenAndLockFile(const char* mode)
{
    int fd = -1;
    std::FILE* fp = nullptr;

    if (nullptr != (fp = fopen(m_agentConfigurationFile.c_str(), mode)))
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

std::FILE* Ztsi::OpenAndLockFile(const char* mode, unsigned int milliseconds, int count)
{
    int i = 0;
    time_t seconds = milliseconds / 1000;
    long nanoseconds = (milliseconds % 1000) * 1000000;
    struct timespec lockTimeToSleep = {seconds, nanoseconds};
    std::FILE* fp = nullptr;

    while ((nullptr == (fp = OpenAndLockFile(mode))) && (i < count))
    {
        nanosleep(&lockTimeToSleep, nullptr);
        i++;
    }

    return fp;
}

void Ztsi::CloseAndUnlockFile(std::FILE* fp)
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
    int status = MMI_OK;
    std::string configurationJson;
    std::FILE* fp = nullptr;
    long fileSize = 0;
    size_t bytesRead = 0;
    char* buffer = nullptr;

    if (FileExists(m_agentConfigurationFile.c_str()))
    {
        if (nullptr != (fp = OpenAndLockFile("r")))
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

            CloseAndUnlockFile(fp);
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
    int status = MMI_OK;
    std::FILE* fp = nullptr;

    if (nullptr != (fp = OpenAndLockFile("r+", g_lockWait, g_lockWaitMaxRetries)))
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

        CloseAndUnlockFile(fp);
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

    writer.Key(g_configurationPropertyServiceUrl);
    writer.String(configuration.serviceUrl.c_str());

    writer.EndObject();

    return buffer.GetString();
}
