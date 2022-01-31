// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <map>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define APTINSTALL_LOGFILE "/var/log/osconfig_aptinstall.log"
#define APTINSTALL_ROLLEDLOGFILE "/var/log/osconfig_aptinstall.bak"

class AptInstallLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(APTINSTALL_LOGFILE, APTINSTALL_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

private:
    static OSCONFIG_LOG_HANDLE m_log;
};

class AptInstallBase
{
public:
    enum ExecutionState
    {
        Unknown = 0,
        Running,
        Succeeded,
        Failed,
        TimedOut
    };

    struct DesiredPackages
    {
        std::vector<std::string> packages;
    };

    struct State
    {
        ExecutionState executionState;
        std::string packagesFingerprint;
        std::map<std::string, std::string> packages;
    };

    AptInstallBase(unsigned int maxPayloadSizeBytes);
    virtual ~AptInstallBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();
    virtual int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) = 0;

private:
    int ExecuteUpdate(const std::string& value);
    int ExecuteUpdates(const std::vector<std::string> packages);
    std::string GetFingerprint();
    std::map<std::string, std::string> GetReportedPackages(std::vector<std::string> packages);

    static int SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int DeserializeDesiredPackages(rapidjson::Document& document, DesiredPackages& object);
    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    static ExecutionState GetStateFromStatusCode(int status);
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
    static std::vector<std::string> GetPackagesNames(std::vector<std::string> packages);
    static std::string TrimStart(const std::string& str, const std::string& trim);
    static std::string TrimEnd(const std::string& str, const std::string& trim);
    static std::string Trim(const std::string& str, const std::string& trim);   

    // Store desired settings for reporting
    ExecutionState m_executionState = ExecutionState::Unknown;
    std::vector<std::string> m_desiredPackages; 
    unsigned int m_maxPayloadSizeBytes;
};

class AptInstall : public AptInstallBase
{
public:
    AptInstall(unsigned int maxPayloadSizeBytes);
    ~AptInstall() = default;

    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;
};