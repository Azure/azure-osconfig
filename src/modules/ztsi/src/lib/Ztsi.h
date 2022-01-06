// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>

#define ZTSI_LOGFILE "/var/log/osconfig_ztsi.log"
#define ZTSI_ROLLEDLOGFILE "/var/log/osconfig_ztsi.bak"

class ZtsiLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(ZTSI_LOGFILE, ZTSI_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

private:
    static OSCONFIG_LOG_HANDLE m_log;
};

class Ztsi
{
public:
    enum EnabledState
    {
        Unknown = 0,
        Enabled,
        Disabled
    };

    struct AgentConfiguration
    {
        std::string serviceUrl;
        bool enabled;
    };

    Ztsi(std::string filePath, unsigned int maxPayloadSizeBytes = 0);
    virtual ~Ztsi() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);

    virtual EnabledState GetEnabledState();
    virtual std::string GetServiceUrl();
    virtual int SetEnabled(bool enabled);
    virtual int SetServiceUrl(const std::string& serviceUrl);
    virtual unsigned int GetMaxPayloadSizeBytes();

private:
    static bool IsValidConfiguration(const AgentConfiguration& configuration);

    virtual std::FILE* OpenAndLockFile(const char* mode);
    virtual std::FILE* OpenAndLockFile(const char* mode, unsigned int milliseconds, int count);
    virtual void CloseAndUnlockFile(std::FILE* fp);

    virtual int ReadAgentConfiguration(AgentConfiguration& configuration);
    virtual int WriteAgentConfiguration(const AgentConfiguration& configuration);
    virtual int CreateConfigurationFile(const AgentConfiguration& configuration);
    virtual int ParseAgentConfiguration(const std::string& configurationJson, AgentConfiguration& configuration);
    virtual std::string BuildConfigurationJson(const AgentConfiguration& configuration);

    static const std::string m_componentName;
    static const std::string m_desiredServiceUrl;
    static const std::string m_desiredEnabled;
    static const std::string m_reportedServiceUrl;
    static const std::string m_reportedEnabled;

    std::string m_agentConfigurationDir;
    std::string m_agentConfigurationFile;

    unsigned int m_maxPayloadSizeBytes;
    AgentConfiguration m_lastAvailableConfiguration;
    bool m_lastEnabledState;
};
