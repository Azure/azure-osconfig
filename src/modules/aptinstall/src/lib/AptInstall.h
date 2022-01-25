// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
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
    enum CurrentState
    {
        Unknown = 0,
        Running,
        Succeeded,
        Failed,
        TimedOut
    };

    struct DesiredPackages
    {
        std::string stateId;
        std::vector<std::string> packages;
    };

    struct State
    {
        std::string stateId;
        CurrentState currentState;
        std::string packagesFingerprint;
    };

    AptInstallBase(unsigned int maxPayloadSizeBytes);
    virtual ~AptInstallBase() = default;

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    virtual int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    virtual unsigned int GetMaxPayloadSizeBytes();
    virtual int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) = 0;

private:
    static int SerializeState(State reportedState, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes);
    static int DeserializeDesiredPackages(rapidjson::Document& document, DesiredPackages& object);
    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int ExecuteUpdate(const std::string& value);
    int ExecuteUpdates(const std::vector<std::string> packages);
    static CurrentState GetStateFromStatusCode(int status);
    std::string GetFingerprint();

    // Store desired settings for reporting
    std::string m_stateId;
    CurrentState m_currentState = CurrentState::Unknown;

    unsigned int m_maxPayloadSizeBytes;
};

class AptInstall : public AptInstallBase
{
public:
    AptInstall(unsigned int maxPayloadSizeBytes);
    ~AptInstall() = default;

    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;
};