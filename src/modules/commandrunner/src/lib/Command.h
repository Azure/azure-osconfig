// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMAND_H
#define COMMAND_H

#include <cstring>
#include <functional>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>

const std::string g_commandArguments = "CommandArguments";
const std::string g_commandId = "CommandId";
const std::string g_arguments = "Arguments";
const std::string g_action = "Action";
const std::string g_timeout = "Timeout";
const std::string g_singleLineTextResult = "SingleLineTextResult";

const std::string g_commandStatus = "CommandStatus";
const std::string g_resultCode = "ResultCode";
const std::string g_textResult = "TextResult";
const std::string g_currentState = "CurrentState";

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
        const std::string id;
        const std::string command;
        const Command::Action action;
        const unsigned int timeout;
        const bool singleLineTextResult;

        Arguments(std::string id, std::string command, Command::Action action, unsigned int timeout, bool singleLineTextResult);

        static void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Arguments& arguments);
        static Arguments Deserialize(const rapidjson::Value& object);
    };

    class Status
    {
    public:
        const std::string id;
        int exitCode;
        std::string textResult;
        Command::State state;

        Status(const std::string id, int exitCode, std::string textResult, Command::State state);

        static void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Command::Status& status, bool serializeTextResult = true);
        static Command::Status Deserialize(const rapidjson::Value& object);
    };

    const std::string command;
    const unsigned int timeout;
    const bool replaceEol;

    Command(std::string id, std::string command, unsigned int timeout, bool replaceEol);
    Command(Status status);
    ~Command();

    virtual int Execute(std::function<int()> persistCacheToDisk, unsigned int maxPayloadSizeBytes);
    int Cancel();

    bool IsComplete();
    bool IsCanceled();

    std::string GetId();
    Status GetStatus();
    void SetStatus(int exitCode, std::string textResult = "");
    void SetStatus(int exitCode, std::string textResult, State state);

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

    int Execute(std::function<int()> persistCacheToDisk, unsigned int maxPayloadSizeBytes) override;
};

#endif // COMMAND_H