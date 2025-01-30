// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMAND_H
#define COMMAND_H

#include <cstring>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>

const std::string g_commandArguments = "commandArguments";
const std::string g_commandId = "commandId";
const std::string g_arguments = "arguments";
const std::string g_action = "action";
const std::string g_timeout = "timeout";
const std::string g_singleLineTextResult = "singleLineTextResult";

const std::string g_commandStatus = "commandStatus";
const std::string g_resultCode = "resultCode";
const std::string g_textResult = "textResult";
const std::string g_currentState = "currentState";

#define COMMANDRUNNER_LOGFILE "/var/log/osconfig_commandrunner.log"
#define COMMADRUNNER_ROLLEDLOGFILE "/var/log/osconfig_commandrunner.bak"

class CommandRunnerLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(COMMANDRUNNER_LOGFILE, COMMADRUNNER_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

    static OSCONFIG_LOG_HANDLE m_log;
};

class Command
{
public:
    enum Action
    {
        None = 0,
        Reboot,
        Shutdown,
        RunCommand,
        RefreshCommandStatus,
        CancelCommand
    };

    enum State
    {
        Unknown = 0,
        Running,
        Succeeded,
        Failed,
        TimedOut,
        Canceled
    };

    class Arguments
    {
    public:
        const std::string m_id;
        const std::string m_arguments;
        const Command::Action m_action;
        const unsigned int m_timeout;
        const bool m_singleLineTextResult;

        Arguments(std::string id, std::string command, Command::Action action, unsigned int timeout, bool singleLineTextResult);

        static std::string Serialize(const Command::Arguments& arguments);
        static void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Arguments& arguments);
        static Arguments Deserialize(const rapidjson::Value& object);
    };

    class Status
    {
    public:
        const std::string m_id;
        int m_exitCode;
        std::string m_textResult;
        Command::State m_state;

        Status(const std::string id, int exitCode, std::string textResult, Command::State state);

        static std::string Serialize(const Command::Status& status, bool serializeTextResult = true);
        static void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Status& status, bool serializeTextResult = true);
        static Command::Status Deserialize(const rapidjson::Value& object);
    };

    const std::string m_arguments;
    const unsigned int m_timeout;
    const bool m_replaceEol;

    Command(std::string id, std::string command, unsigned int timeout, bool replaceEol);
    virtual ~Command();

    virtual int Execute(unsigned int maxPayloadSizeBytes);
    int Cancel();

    bool IsComplete();
    bool IsCanceled();

    std::string GetId();
    Status GetStatus();
    void SetStatus(int exitCode, std::string textResult = "");
    void SetStatus(int exitCode, std::string textResult, State state);

    bool operator ==(const Command& other) const;

protected:
    Status m_status;
    std::mutex m_statusMutex;

    std::string m_tmpFile;

    static int ExecutionCallback(void* context);
};

class ShutdownCommand : public Command
{
public:
    ShutdownCommand(std::string id, std::string command, unsigned int timeout, bool replaceEol);

    int Execute(unsigned int maxPayloadSizeBytes) override;
};

#endif // COMMAND_H
