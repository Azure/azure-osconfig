// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>

#define ZTSI_LOGFILE "/var/log/osconfig_ztsi.log"
#define ZTSI_ROLLEDLOGFILE "/var/log/osconfig_ztsi.bak"

bool IsValidClientName(const std::string& clientName);

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

    struct AgentConfig
    {
        std::string serviceUrl;
        bool enabled;
    };

    Ztsi(std::string filePath, unsigned int maxPayloadSizeBytes = 0);
    virtual ~Ztsi() = default;

    virtual unsigned int GetMaxPayloadSizeBytes();
    virtual EnabledState GetEnabledState();
    virtual std::string GetServiceUrl();
    virtual int SetEnabled(bool enabled);
    virtual int SetServiceUrl(const std::string& serviceUrl);

private:
    static bool IsValidConfig(const AgentConfig& config);
    virtual std::FILE* LockFile(const char* mode);
    virtual std::FILE* LockFile(const char* mode, unsigned int millis, int count);
    virtual void UnlockFile(std::FILE* fp);
    static bool FileExists(const std::string& filePath);
    virtual int ReadAgentConfig(AgentConfig& config);
    virtual int WriteAgentConfig(const AgentConfig& config);
    virtual int CreateConfigFile(const AgentConfig& config);
    virtual int ParseAgentConfig(const std::string& configJson, AgentConfig& config);
    virtual std::string BuildConfigJson(const AgentConfig& config);

    std::string m_agentConfigDir;
    std::string m_agentConfigFile;
    unsigned int m_maxPayloadSizeBytes;
    AgentConfig m_lastAvailableConfig;
};
