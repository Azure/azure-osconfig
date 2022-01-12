// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <array>
#include <functional>
#include <Logging.h>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>

#define COMMANDSTATUS_CACHE_MAX 10
#define COMMANDRUNNER_LOGFILE "/var/log/osconfig_commandrunner.log"
#define COMMADRUNNER_ROLLEDLOGFILE "/var/log/osconfig_commandrunner.bak"

#define COMMAND_STATUS_UNIQUE_ID_LENGTH 10

static const char alphanum[] = "0123456789"\
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
    "abcdefghijklmnopqrstuvwxyz";

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

class CommandRunner
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

    enum CommandState
    {
        Unknown = 0,
        Running,
        Succeeded,
        Failed,
        TimedOut,
        Canceled
    };

    struct CommandArguments
    {
        std::string commandId;
        std::string arguments;
        Action action;
        unsigned int timeout;
        bool singleLineTextResult;
    };

    class CommandStatus
    {
    public:
        std::string commandId;
        int resultCode;
        std::string textResult;
        CommandState commandState;

        CommandStatus();
        CommandStatus(const CommandStatus& other);
        virtual ~CommandStatus() {};
        CommandStatus& operator=(CommandStatus other)
        {
            std::swap(commandId, other.commandId);
            std::swap(resultCode, other.resultCode);
            std::swap(textResult, other.textResult);
            std::swap(commandState, other.commandState);
            return *this;
        }
        virtual std::string GetUniqueId();

    private:
        std::string uniqueId;
    };

    typedef std::map<std::string, std::weak_ptr<CommandStatus>> CommandResults;

    CommandRunner(std::string name, std::function<int()> persistentCacheFunction, unsigned int maxSizeInBytes = 0);
    virtual ~CommandRunner();
    virtual int Run(CommandArguments command);
    virtual int Cancel(std::string commandId);
    virtual void CancelAll();
    static void CommandWorkerThread(CommandRunner& commandRunnerInstance, std::queue<CommandArguments>& commandArgumentsBuffer);
    virtual CommandStatus* GetCommandStatus(std::string commandId);
    static int Execute(CommandRunner& commandRunnerInstance, CommandRunner::Action action, std::string commandId, std::string command, CommandState initialState, unsigned int timeoutSeconds, bool replaceEol);
    virtual const std::string& GetCommandIdToRefresh();
    virtual int SetCommandIdToRefresh(std::string commandId);
    virtual void WaitForCommandResults();
    virtual CommandStatus GetCommandStatusToPersist();
    virtual void PersistCommandStatus(CommandStatus commandStatus);
    virtual void AddCommandStatus(std::string commandId, bool updateCommandToRefresh);
    virtual void UpdatePartialCommandStatus(std::string commandId, int resultCode, CommandState commandState);
    virtual void UpdateCommandStatus(std::string commandId, int resultCode, std::string textResult, CommandState commandState);
    virtual bool CommandExists(std::string commandId);
    virtual bool IsCanceled(std::string commandId);
    virtual int GetMaxPayloadSizeInBytes();
    virtual std::string GetClientName();

private:
    static std::string GetTmpFilePath(std::string uniqueId);
    static int CommandExecutionCallback(void* context);
    static CommandState CommandStateFromStatusCode(int status);

    std::function<int()> cacheFunction;
    std::mutex cacheMutex;
    std::queue<CommandArguments> commandArgumentsBuffer;
    std::thread commandWorkerThread;
    std::array<std::shared_ptr<CommandStatus>, COMMANDSTATUS_CACHE_MAX> commandStatusBuffer;
    int curIndexCommandBuffer;
    CommandResults commmandMap;
    CommandStatus persistedCommandStatus;
    std::string commandIdToRefresh;
    std::string clientName;
    unsigned int maxPayloadSizeInBytes;
};


#endif // COMMANDRUNNER_H