// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctime>
#include <cstdio>
#include <fstream>
#include <Mmi.h>
#include <sys/stat.h>

#include <CommandRunner.h>
#include <CommonUtils.h>

const char commandStatusTemplate[] = "{\"CommandId\":\"\",\"ResultCode\":,\"TextResult\":\"\",\"CurrentState\":}";
const char resultCodeChar[] = "-32767";

static const std::string TMP_ENV_VAR = "TMPDIR";
static const std::string TMP_DEFAULT_DIR = "/tmp";
static const std::string TMP_FILE_PREFIX = "~osconfig-";

OSCONFIG_LOG_HANDLE CommandRunnerLog::m_log = nullptr;

bool FileExists(const std::string& fileName)
{
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

CommandRunner::CommandRunner(std::string name, std::function<int()> persistentCacheFunction, unsigned int maxSizeInBytes) : curIndexCommandBuffer(0)
{
    clientName = name;
    cacheFunction = persistentCacheFunction;
    maxPayloadSizeInBytes = maxSizeInBytes;
}

CommandRunner::~CommandRunner()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner shutting down");

    try
    {
        commandWorkerThread.join();
    }
    catch (const std::exception& e) {}
}

int CommandRunner::Run(CommandArguments command)
{
    int status = MMI_OK;
    if (commandArgumentsBuffer.empty() && commandWorkerThread.joinable())
    {
        commandWorkerThread.join();
    }

    switch (command.action)
    {
        case CommandRunner::Action::None:
            OsConfigLogInfo(CommandRunnerLog::Get(), "No action to perform");
            break;

        case CommandRunner::Action::RefreshCommandStatus:
            OsConfigLogInfo(CommandRunnerLog::Get(), "Refreshing commandId: %s", command.commandId.c_str());
            status = SetCommandIdToRefresh(command.commandId);
            break;

        default:
            AddCommandStatus(command.commandId, true);
            UpdatePartialCommandStatus(command.commandId, 0, CommandState::Running);

            if (cacheFunction)
            {
                if (0 != (status = cacheFunction()))
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "Unable to persist to cache, skipping command '%s' with %d", command.commandId.c_str(), status);
                    return status;
                }
            }

            // Push command onto command buffer and start worker thread if not already running
            commandArgumentsBuffer.push(command);
            if (!commandWorkerThread.joinable())
            {
                commandWorkerThread = std::thread(CommandWorkerThread, std::ref(*this), std::ref(commandArgumentsBuffer));
            }
    }
    return status;
}

void CommandRunner::CommandWorkerThread(CommandRunner& commandRunnerInstance, std::queue<CommandArguments>& commandArgumentsBuffer)
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner worker thread started. Processing commands");
    while (!commandArgumentsBuffer.empty())
    {
        auto command = commandArgumentsBuffer.front();

        // Execute command
        switch (command.action)
        {
            case CommandRunner::Action::Reboot:
                OsConfigLogInfo(CommandRunnerLog::Get(), "Attempting to reboot");
                CommandRunner::Execute(commandRunnerInstance, command.action, command.commandId, "shutdown -r now", CommandState::Succeeded, 0, true);
                break;

            case CommandRunner::Action::Shutdown:
                OsConfigLogInfo(CommandRunnerLog::Get(), "Attempting to shutdown");
                CommandRunner::Execute(commandRunnerInstance, command.action, command.commandId, "shutdown now", CommandState::Succeeded, 0, true);
                break;

            case CommandRunner::Action::RunCommand:
                CommandRunner::Execute(commandRunnerInstance, command.action, command.commandId, command.arguments, CommandState::Running, command.timeout, command.singleLineTextResult);
                break;

            default:
                OsConfigLogError(CommandRunnerLog::Get(), "Invalid action: %d", command.action);
        }

        commandArgumentsBuffer.pop();
    }
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner worker thread finished. No more commands to process");
}

std::string CommandRunner::GetTmpFilePath(std::string uniqueId)
{
    static std::string tmp;
    if (tmp.empty())
    {
        char const* dir = getenv(TMP_ENV_VAR.c_str());
        tmp = (NULL == dir) ? TMP_DEFAULT_DIR : dir;
    }

    return tmp + "/" + TMP_FILE_PREFIX + uniqueId;
}

int CommandRunner::CommandExecutionCallback(void* context)
{
    int result = 0;
    if (nullptr != context)
    {
        std::string tmpFilePath(reinterpret_cast<char*>(context));
        if (FileExists(tmpFilePath))
        {
            remove(tmpFilePath.c_str());
            result = 1;
        }
    }
    return result;
}

int CommandRunner::Cancel(std::string commandId)
{
    int status = MMI_OK;
    CommandStatus* commandStatus = GetCommandStatus(commandId);
    if ((nullptr != commandStatus) && (CommandState::Canceled != commandStatus->commandState))
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Canceling command with CommandId: %s", commandId.c_str());

        UpdatePartialCommandStatus(commandId, ECANCELED, CommandState::Canceled);

        std::string tmpFilePath = GetTmpFilePath(commandStatus->GetUniqueId());
        if (!FileExists(tmpFilePath))
        {
            // Create empty temporary file
            std::ofstream output(tmpFilePath);
            output.close();
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Unable to cancel command with CommandId: %s", commandId.c_str());
        status = EINVAL;
    }
    return status;
}

void CommandRunner::CancelAll()
{
    // Cancel and remove all remaining commands from command buffer
    while (!commandArgumentsBuffer.empty())
    {
        auto command = commandArgumentsBuffer.front();
        Cancel(command.commandId);
        commandArgumentsBuffer.pop();
    }

    if ((CommandState::Running == persistedCommandStatus.commandState) || (CommandState::Unknown == persistedCommandStatus.commandState))
    {
        UpdatePartialCommandStatus(persistedCommandStatus.commandId, ECANCELED, CommandState::Canceled);
    }
}

CommandRunner::CommandState CommandRunner::CommandStateFromStatusCode(int status)
{
    CommandState state = CommandState::Unknown;
    switch (status)
    {
        case EXIT_SUCCESS:
            state = CommandState::Succeeded;
            break;
        case ETIME:
            state = CommandState::TimedOut;
            break;
        case ECANCELED:
            state = CommandState::Canceled;
            break;
        default:
            state = CommandState::Failed;
    }
    return state;
}

int CommandRunner::Execute(CommandRunner& instance, CommandRunner::Action action, std::string commandId, std::string command, CommandState initialState, unsigned int timeoutSeconds, bool replaceEol)
{
    int status = EINVAL;
    char* textResult = nullptr;

    // Check if command has been canceled
    if (instance.IsCanceled(commandId))
    {
        return status;
    }

    OsConfigLogInfo(CommandRunnerLog::Get(), "Running command '%s'", commandId.c_str());
    instance.SetCommandIdToRefresh(commandId);
    instance.UpdatePartialCommandStatus(commandId, 0, initialState);

    if ((nullptr != instance.cacheFunction) && (0 != (status = instance.cacheFunction())))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Unable to persist to cache, skipping command '%s' with %d", commandId.c_str(), status);
        return status;
    }

    unsigned int maxTextResultBytes = 0;
    unsigned int maxPayloadSizeInBytes = static_cast<unsigned int>(instance.GetMaxPayloadSizeInBytes());
    if (maxPayloadSizeInBytes > 0)
    {
        unsigned int estimatedSize = strlen(commandStatusTemplate) + strlen(commandId.c_str()) + (3 * strlen(resultCodeChar));
        maxTextResultBytes = (maxPayloadSizeInBytes > estimatedSize) ? (maxPayloadSizeInBytes - estimatedSize) : 1;
    }

    void* context = nullptr;
    CommandStatus* commandStatus = nullptr;
    std::string tmpFilePath;
    if (nullptr != (commandStatus = instance.GetCommandStatus(commandId)))
    {
        tmpFilePath = CommandRunner::GetTmpFilePath(commandStatus->GetUniqueId());
        context = reinterpret_cast<void*>(const_cast<char*>(tmpFilePath.c_str()));
    }

    status = ExecuteCommand(context, command.c_str(), replaceEol, true, maxTextResultBytes, timeoutSeconds, &textResult, &(CommandRunner::CommandExecutionCallback), CommandRunnerLog::Get());

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' ('%s') completed with %d and '%s'", commandId.c_str(), command.c_str(), status, textResult ? textResult : "no text result");
    }
    else
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' completed with %d", commandId.c_str(), status);
    }

    std::string results;
    if (textResult)
    {
        results = std::string(textResult, strlen(textResult));
        free(textResult);
    }

    if ((CommandRunner::Action::Reboot != action) && (CommandRunner::Action::Shutdown != action))
    {
        // Update command status with results from ExecuteCommand()
        instance.UpdateCommandStatus(commandId, status, results, CommandStateFromStatusCode(status));
        if (ECANCELED != status)
        {
            instance.SetCommandIdToRefresh(commandId);
        }

        if ((nullptr != instance.cacheFunction) && (0 != (status = instance.cacheFunction())))
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Post command operation failed for command '%s' with %d", commandId.c_str(), status);
        }
    }

    return status;
}

const std::string& CommandRunner::GetCommandIdToRefresh()
{
    return commandIdToRefresh;
}

int CommandRunner::SetCommandIdToRefresh(std::string commandId)
{
    if (CommandExists(commandId))
    {
        commandIdToRefresh = commandId;
        return MMI_OK;
    }
    else
    {
        return EINVAL;
    }
}

CommandRunner::CommandStatus* CommandRunner::GetCommandStatus(std::string commandId)
{
    try
    {
        if (commmandMap.end() == commmandMap.find(commandId))
        {
            return nullptr;
        }
        else if (commmandMap[commandId].expired())
        {
            commmandMap.erase(commmandMap.find(commandId));
            return nullptr;
        }

        cacheMutex.lock();
        CommandStatus* commandStatus = commmandMap[commandId].lock().get();
        CommandStatus* commandStatusCopy = new CommandStatus(*commandStatus);
        cacheMutex.unlock();

        return commandStatusCopy;
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Unable to retreive CommandStatus for commandId: %s", commandId.c_str());
        return nullptr;
    }
}

CommandRunner::CommandStatus CommandRunner::GetCommandStatusToPersist()
{
    return persistedCommandStatus;
}

void CommandRunner::PersistCommandStatus(CommandStatus commandStatus)
{
    persistedCommandStatus = commandStatus;
}

void CommandRunner::AddCommandStatus(std::string commandId, bool updateCommandToRefresh)
{
    if (!commandId.empty())
    {
        cacheMutex.lock();
        std::shared_ptr<CommandStatus> commandStatus;
        if ((commmandMap.end() == commmandMap.find(commandId)))
        {
            // Add new CommandStatus
            commandStatus.reset(new CommandStatus());
            commandStatusBuffer[curIndexCommandBuffer] = commandStatus;
            commmandMap[commandId] = std::weak_ptr<CommandStatus>(commandStatusBuffer[curIndexCommandBuffer]);
            curIndexCommandBuffer = (curIndexCommandBuffer + 1) % COMMANDSTATUS_CACHE_MAX;

            // Set default values
            commandStatus->commandId = commandId;
            commandStatus->commandState = CommandState::Unknown;
            commandStatus->resultCode = 0;
            commandStatus->textResult = "";

            if (updateCommandToRefresh || commandIdToRefresh.empty())
            {
                SetCommandIdToRefresh(commandId);
            }

            // Persist new command
            PersistCommandStatus(*commandStatus.get());
        }
        cacheMutex.unlock();
    }
}

void CommandRunner::UpdateCommandStatus(std::string commandId, int resultCode, std::string textResult, CommandState commandState)
{
    if (!commandId.empty())
    {
        cacheMutex.lock();
        std::shared_ptr<CommandStatus> commandStatus;
        if ((commmandMap.end() != commmandMap.find(commandId)) && !commmandMap[commandId].expired())
        {
            commandStatus = commmandMap[commandId].lock();

            commandStatus->commandId = commandId;
            commandStatus->commandState = commandState;
            commandStatus->resultCode = resultCode;
            commandStatus->textResult = textResult;

            if ((CommandState::Running != commandState) && (commandId == persistedCommandStatus.commandId))
            {
                PersistCommandStatus(*commandStatus.get());
            }
        }
        cacheMutex.unlock();
    }
}

void CommandRunner::UpdatePartialCommandStatus(std::string commandId, int resultCode, CommandState commandState)
{
    if (!commandId.empty())
    {
        cacheMutex.lock();
        std::shared_ptr<CommandStatus> commandStatus;
        if ((commmandMap.end() != commmandMap.find(commandId)) && !commmandMap[commandId].expired())
        {
            commandStatus = commmandMap[commandId].lock();
            commandStatus->resultCode = resultCode;
            commandStatus->commandState = commandState;

            if ((CommandState::Running != commandState) && (commandId == persistedCommandStatus.commandId))
            {
                PersistCommandStatus(*commandStatus.get());
            }
        }
        cacheMutex.unlock();
    }
}

bool CommandRunner::CommandExists(std::string commandId)
{
    return commmandMap.end() != commmandMap.find(commandId);
}

bool CommandRunner::IsCanceled(std::string commandId)
{
    if (CommandExists(commandId))
    {
        CommandStatus* commandStatus = GetCommandStatus(commandId);
        return FileExists(GetTmpFilePath(commandStatus->GetUniqueId()));
    }
    return false;
}

std::string CommandRunner::GetClientName()
{
    return clientName;
}

int CommandRunner::GetMaxPayloadSizeInBytes()
{
    return maxPayloadSizeInBytes;
}

void CommandRunner::WaitForCommandResults()
{
    if (!commandArgumentsBuffer.empty() && commandWorkerThread.joinable())
    {
        commandWorkerThread.join();
    }
}

CommandRunner::CommandStatus::CommandStatus()
{
    srand((unsigned)time(NULL) * getpid());
    uniqueId.reserve(COMMAND_STATUS_UNIQUE_ID_LENGTH);

    for (int i = 0; i < COMMAND_STATUS_UNIQUE_ID_LENGTH; ++i)
    {
        uniqueId += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}

CommandRunner::CommandStatus::CommandStatus(const CommandStatus& other)
{
    commandId = other.commandId;
    commandState = other.commandState;
    resultCode = other.resultCode;
    textResult = other.textResult;
    uniqueId = other.uniqueId;
}

std::string CommandRunner::CommandStatus::GetUniqueId()
{
    return uniqueId;
}