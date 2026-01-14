// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <nlohmann/json.hpp>

#include <Command.h>
#include <CommandRunner.h>
#include <Mmi.h>

const std::string CommandRunner::m_componentName = "CommandRunner";
const unsigned int CommandRunner::m_maxCacheSize = 10;
const char* CommandRunner::m_persistedCacheFile = "/etc/osconfig/osconfig_commandrunner.cache";
const char* CommandRunner::m_defaultCacheTemplate = "{}";

constexpr const char g_moduleInfo[] = R""""({
    "Name": "CommandRunner",
    "Description": "Provides functionality to remotely run commands on the device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["CommandRunner"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

std::mutex CommandRunner::m_diskCacheMutex;

CommandRunner::CommandRunner(std::string clientName, unsigned int maxPayloadSizeBytes, bool usePersistedCache) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_usePersistedCache(usePersistedCache),
    m_lastPayloadHash(0)
{
    if (m_usePersistedCache)
    {
        int status = LoadPersistedCommandStatus(clientName);
        if (0 != status)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to load persisted command status for client %s", clientName.c_str());
            OSConfigTelemetryStatusTrace("LoadPersistedCommandStatus", status);
        }
        else if (m_commandMap.size() > 0)
        {
            m_commandIdLoadedFromDisk = m_commandMap.rbegin()->first;
        }
    }
    else
    {
        m_commandIdLoadedFromDisk = "";
    }

    // Start the worker thread
    m_workerThread = std::thread(&CommandRunner::WorkerThread, std::ref(*this));
}

CommandRunner::~CommandRunner()
{
    while (!m_commandQueue.Empty())
    {
        std::shared_ptr<Command> command = m_commandQueue.Pop().lock();
        if (nullptr != command)
        {
            command->Cancel();
        }
    }

    // Push nullptr to signal the worker thread to exit
    m_commandQueue.Push(std::weak_ptr<Command>());

    try
    {
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    }
    catch (const std::exception& e) {}

    Command::Status status = GetStatusToPersist();
    if (!status.m_id.empty() && (0 != PersistCommandStatus(status)))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist command status for session %s during shutdown", m_clientName.c_str());
        OSConfigTelemetryStatusTrace("PersistCommandStatus", status.m_exitCode);
    }
}

int CommandRunner::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid clientName");
        OSConfigTelemetryStatusTrace("clientName", status);
    }
    else if (nullptr == payload)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payload");
        OSConfigTelemetryStatusTrace("payload", status);
    }
    else if (nullptr == payloadSizeBytes)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payloadSizeBytes");
        OSConfigTelemetryStatusTrace("payloadSizeBytes", status);
    }
    else
    {
        std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
            OSConfigTelemetryStatusTrace("payload", ENOMEM);
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_moduleInfo, len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

int CommandRunner::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    nlohmann::json document;

    try
    {
        document = nlohmann::json::parse(std::string(payload, payloadSizeBytes));
    }
    catch (const nlohmann::json::exception& e)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Unable to parse JSON payload: %s", payload);
        OSConfigTelemetryStatusTrace("Parse", status);
        return status;
    }

    if (0 == CommandRunner::m_componentName.compare(componentName))
    {
        if (0 == g_commandArguments.compare(objectName))
        {
            size_t payloadHash = HashString(payload);
            Command::Arguments arguments = Command::Arguments::Deserialize(document);

            if (m_usePersistedCache)
            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                if ((m_commandMap.find(arguments.m_id) != m_commandMap.end()) && (m_commandMap[arguments.m_id]->GetId() == m_commandIdLoadedFromDisk))
                {
                    OsConfigLogDebug(CommandRunnerLog::Get(), "Updating command (%s) loaded from disk, with complete payload", arguments.m_id.c_str());

                    // Update the partial command loaded from the persisted cache
                    Command::Status currentStatus = m_commandMap[arguments.m_id]->GetStatus();

                    std::shared_ptr<Command> command = std::make_shared<Command>(arguments.m_id, arguments.m_arguments, arguments.m_timeout, arguments.m_singleLineTextResult);
                    command->SetStatus(currentStatus.m_exitCode, currentStatus.m_textResult, currentStatus.m_state);

                    m_commandMap[arguments.m_id] = command;
                }
            }

            if (m_lastPayloadHash != payloadHash)
            {
                m_lastPayloadHash = payloadHash;

                switch (arguments.m_action)
                {
                    case Command::Action::RunCommand:
                        status = Run(arguments.m_id, arguments.m_arguments, arguments.m_timeout, arguments.m_singleLineTextResult);
                        break;
                    case Command::Action::Reboot:
                        status = Reboot(arguments.m_id);
                        break;
                    case Command::Action::Shutdown:
                        status = Shutdown(arguments.m_id);
                        break;
                    case Command::Action::CancelCommand:
                        status = Cancel(arguments.m_id);
                        break;
                    case Command::Action::RefreshCommandStatus:
                        status = Refresh(arguments.m_id);
                        break;
                    case Command::Action::None:
                        OsConfigLogInfo(CommandRunnerLog::Get(), "No action for command: %s", arguments.m_id.c_str());
                        break;
                    default:
                        status = EINVAL;
                        OsConfigLogError(CommandRunnerLog::Get(), "Unsupported action: %d", static_cast<int>(arguments.m_action));
                        OSConfigTelemetryStatusTrace("m_action", EINVAL);
                }
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Invalid object name: %s", objectName);
            OSConfigTelemetryStatusTrace("objectName", status);
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid component name: %s", componentName);
        OSConfigTelemetryStatusTrace("componentName", status);
    }

    return status;
}

int CommandRunner::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payload)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payload");
        OSConfigTelemetryStatusTrace("payload", status);
    }
    else if (nullptr == payloadSizeBytes)
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payloadSizeBytes");
        OSConfigTelemetryStatusTrace("payloadSizeBytes", status);
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        if (0 == CommandRunner::m_componentName.compare(componentName))
        {
            if (0 == g_commandStatus.compare(objectName))
            {
                Command::Status commandStatus = GetReportedStatus();
                std::string jsonStr = Command::Status::ToJson(commandStatus).dump();

                *payload = new (std::nothrow) char[jsonStr.size()];

                if (nullptr != *payload)
                {
                    std::fill(*payload, *payload + jsonStr.size(), 0);
                    std::memcpy(*payload, jsonStr.c_str(), jsonStr.size());
                    *payloadSizeBytes = jsonStr.size();
                }
                else
                {
                    status = ENOMEM;
                    OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
                    OSConfigTelemetryStatusTrace("payload", status);
                }
            }
            else
            {
                status = EINVAL;
                OsConfigLogError(CommandRunnerLog::Get(), "Invalid object name: %s", objectName);
                OSConfigTelemetryStatusTrace("objectName", status);
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Invalid component name: %s", componentName);
            OSConfigTelemetryStatusTrace("componentName", status);
        }
    }

    return status;
}

const std::string& CommandRunner::GetClientName() const
{
    return m_clientName;
}

unsigned int CommandRunner::GetMaxPayloadSizeBytes() const
{
    return m_maxPayloadSizeBytes;
}

void CommandRunner::WaitForCommands()
{
    m_commandQueue.WaitUntilEmpty();
}

int CommandRunner::Run(const std::string id, std::string arguments, unsigned int timeout, bool singleLineTextResult)
{
    std::shared_ptr<Command> command = std::make_shared<Command>(id, arguments, timeout, singleLineTextResult);
    return ScheduleCommand(command);
}

int CommandRunner::Reboot(const std::string id)
{
    std::shared_ptr<Command> command = std::make_shared<ShutdownCommand>(id, "shutdown -r now", 0, false);
    return ScheduleCommand(command);
}

int CommandRunner::Shutdown(const std::string id)
{
    std::shared_ptr<Command> command = std::make_shared<ShutdownCommand>(id, "shutdown now", 0, false);
    return ScheduleCommand(command);
}

int CommandRunner::Cancel(const std::string id)
{
    int status = 0;
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if ((m_commandMap.find(id) != m_commandMap.end()))
    {
        std::shared_ptr<Command> command = m_commandMap[id];
        OsConfigLogInfo(CommandRunnerLog::Get(), "Canceling command: %s", id.c_str());
        status = command->Cancel();
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Command does not exist and cannot be canceled: %s", id.c_str());
        OSConfigTelemetryStatusTrace("id", status);
    }

    return status;
}

int CommandRunner::Refresh(const std::string id)
{
    int status = 0;

    if (CommandIdExists(id))
    {
        SetReportedStatusId(id);
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Command does not exist and cannot be refreshed: %s", id.c_str());
        OSConfigTelemetryStatusTrace("CommandIdExists", status);
    }

    return status;
}

bool CommandRunner::CommandExists(std::shared_ptr<Command> command)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    std::string id = command->GetId();
    return (m_commandMap.find(id) != m_commandMap.end()) && (*m_commandMap[id] == *command);
}

bool CommandRunner::CommandIdExists(const std::string& id)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_commandMap.find(id) != m_commandMap.end();
}

int CommandRunner::ScheduleCommand(std::shared_ptr<Command> command)
{
    int status = 0;

    if (!CommandExists(command))
    {
        if (!CommandIdExists(command->GetId()))
        {
            if (0 == (status = PersistCommandStatus(command->GetStatus())))
            {
                if (0 == (status = CacheCommand(command)))
                {
                    m_commandQueue.Push(command);
                }
                else
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "Failed to cache command: %s", command->GetId().c_str());
                    OSConfigTelemetryStatusTrace("CacheCommand", status);
                }
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist command to disk. Skipping command: %s", command->GetId().c_str());
                OSConfigTelemetryStatusTrace("PersistCommandStatus", status);
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Command already exists with id: %s", command->GetId().c_str());
            OSConfigTelemetryStatusTrace("CommandIdExists", status);
        }
    }
    else
    {
        OsConfigLogDebug(CommandRunnerLog::Get(), "Command already recieved: %s (%s)", command->GetId().c_str(), command->m_arguments.c_str());
    }

    return status;
}

int CommandRunner::CacheCommand(std::shared_ptr<Command> command)
{
    int status = 0;
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (!command->GetId().empty())
    {
        if (m_commandMap.find(command->GetId()) == m_commandMap.end())
        {
            m_commandMap[command->GetId()] = command;
            m_cacheBuffer.push_front(command);
            SetReportedStatusId(command->GetId());

            // Remove any completed commands from the cache if the cache size is greater than the maximum size
            while (m_cacheBuffer.size() > m_maxCacheSize)
            {
                std::shared_ptr<Command> oldestCommand = m_cacheBuffer.back();
                if ((nullptr != oldestCommand) && (oldestCommand->IsComplete()))
                {
                    m_cacheBuffer.pop_back();
                    m_commandMap.erase(oldestCommand->GetId());
                }
            }
        }
        else
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Cannot cache command with duplicate id: %s", command->GetId().c_str());
            OSConfigTelemetryStatusTrace("find", status);
        }
    }
    else
    {
        status = EINVAL;
        OsConfigLogError(CommandRunnerLog::Get(), "Cannot cache command with empty id");
        OSConfigTelemetryStatusTrace("GetId", status);
    }

    return status;
}

void CommandRunner::SetReportedStatusId(const std::string id)
{
    std::lock_guard<std::mutex> lock(m_reportedStatusIdMutex);
    m_reportedStatusId = id;
}

std::string CommandRunner::GetReportedStatusId()
{
    std::lock_guard<std::mutex> lock(m_reportedStatusIdMutex);
    return m_reportedStatusId;
}

Command::Status CommandRunner::GetReportedStatus()
{
    std::string reportedCommandId = GetReportedStatusId();
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (m_commandMap.find(reportedCommandId) != m_commandMap.end())
    {
        return m_commandMap[reportedCommandId]->GetStatus();
    }
    else
    {
        return Command::Status("", 0, "", Command::State::Unknown);
    }
}

void CommandRunner::WorkerThread(CommandRunner& instance)
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "Starting worker thread for session: %s", instance.m_clientName.c_str());

    std::shared_ptr<Command> command;
    while (nullptr != (command = instance.m_commandQueue.Front().lock()))
    {
        int exitCode = command->Execute(instance.m_maxPayloadSizeBytes);

        if (IsDebugLoggingEnabled())
        {
            OsConfigLogDebug(CommandRunnerLog::Get(), "Command '%s' (%s) completed with code: %d", command->GetId().c_str(), command->m_arguments.c_str(), exitCode);
        }
        else
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' completed with code: %d", command->GetId().c_str(), exitCode);
        }

        instance.PersistCommandStatus(command->GetStatus());
        instance.m_commandQueue.Pop();
    }

    OsConfigLogInfo(CommandRunnerLog::Get(), "Worker thread stopped for session: %s", instance.m_clientName.c_str());
}

Command::Status CommandRunner::GetStatusToPersist()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    if (m_cacheBuffer.empty())
    {
        return Command::Status("", 0, "", Command::State::Unknown);
    }
    else
    {
        return m_cacheBuffer.front()->GetStatus();
    }
}

int CommandRunner::LoadPersistedCommandStatus(const std::string& clientName)
{
    int status = 0;

    std::lock_guard<std::mutex> lock(m_diskCacheMutex);
    std::ifstream file(m_persistedCacheFile);

    if (file.good())
    {
        nlohmann::json document;

        try
        {
            file >> document;
        }
        catch (const nlohmann::json::exception& e)
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to parse cache file");
            OSConfigTelemetryStatusTrace("ParseStream", status);
            return status;
        }

        if (!document.is_object())
        {
            status = EINVAL;
            OsConfigLogError(CommandRunnerLog::Get(), "Cache file JSON is not an object");
            OSConfigTelemetryStatusTrace("is_object", status);
        }
        else if (document.contains(clientName))
        {
            const nlohmann::json& client = document[clientName];

            if (client.is_array())
            {
                for (const auto& it : client)
                {
                    Command::Status commandStatus = Command::Status::Deserialize(it);

                    std::shared_ptr<Command> command = std::make_shared<Command>(commandStatus.m_id, "", 0, "");
                    command->SetStatus(commandStatus.m_exitCode, commandStatus.m_textResult, commandStatus.m_state);

                    if (0 != CacheCommand(command))
                    {
                        status = -1;
                        OsConfigLogError(CommandRunnerLog::Get(), "Failed to cache command: %s", commandStatus.m_id.c_str());
                        OSConfigTelemetryStatusTrace("CacheCommand", status);
                    }
                }
            }
        }
        else
        {
            OsConfigLogDebug(CommandRunnerLog::Get(), "Cache file does not contain a status for client: %s", clientName.c_str());
        }
    }

    return status;
}

int CommandRunner::PersistCommandStatus(const Command::Status& status)
{
    return m_usePersistedCache ? PersistCommandStatus(m_clientName, status) : 0;
}

int CommandRunner::PersistCommandStatus(const std::string& clientName, const Command::Status commandStatus)
{
    int status = 0;
    nlohmann::json document;
    nlohmann::json statusJson = Command::Status::ToJson(commandStatus, false);
    std::lock_guard<std::mutex> lock(m_diskCacheMutex);

    std::ifstream file(m_persistedCacheFile);
    if (file.good())
    {
        try
        {
            file >> document;
            if (!document.is_object())
            {
                document = nlohmann::json::parse(m_defaultCacheTemplate);
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            document = nlohmann::json::parse(m_defaultCacheTemplate);
        }
    }
    else
    {
        document = nlohmann::json::parse(m_defaultCacheTemplate);
    }

    if (document.contains(clientName))
    {
        bool updated = false;
        nlohmann::json& client = document[clientName];

        if (!client.is_array())
        {
            client = nlohmann::json::array();
        }

        for (auto& it : client)
        {
            if (it.contains(g_commandId) && it[g_commandId].is_string() && (it[g_commandId].get<std::string>() == commandStatus.m_id))
            {
                it = statusJson;
                updated = true;
                break;
            }
        }

        if (!updated)
        {
            if (client.size() >= m_maxCacheSize)
            {
                client.erase(client.begin());
            }

            client.push_back(statusJson);
        }
    }
    else
    {
        nlohmann::json clientArray = nlohmann::json::array();
        clientArray.push_back(statusJson);
        document[clientName] = clientArray;
    }

    std::string jsonStr = document.dump(4);  // 4 spaces for indentation

    if (jsonStr.size() > 0)
    {
        std::FILE* file = std::fopen(m_persistedCacheFile, "w+");
        if (nullptr == file)
        {
            status = EACCES;
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to open file: %s", m_persistedCacheFile);
            OSConfigTelemetryStatusTrace("fopen", status);
        }
        else
        {
            int rc = std::fputs(jsonStr.c_str(), file);

            if ((0 > rc) || (EOF == rc))
            {
                status = errno ? errno : EINVAL;
                OsConfigLogError(CommandRunnerLog::Get(), "Failed write to file %s, error: %d %s", m_persistedCacheFile, status, errno ? strerror(errno) : "-");
                OSConfigTelemetryStatusTrace("fputs", status);
            }

            fflush(file);
            std::fclose(file);

            RestrictFileAccessToCurrentAccountOnly(m_persistedCacheFile);
        }
    }

    return status;
}

template<class T>
CommandRunner::SafeQueue<T>::SafeQueue() :
    m_queue(),
    m_mutex(),
    m_condition() { }

template<class T>
void CommandRunner::SafeQueue<T>::Push(T value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(value);
    m_condition.notify_one();
}

template<class T>
T CommandRunner::SafeQueue<T>::Pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty(); });
    T value = m_queue.front();
    m_queue.pop();

    if (m_queue.empty())
    {
        m_conditionEmpty.notify_one();
    }

    return value;
}

template<class T>
T CommandRunner::SafeQueue<T>::Front()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty(); });
    return m_queue.front();
}

template<class T>
bool CommandRunner::SafeQueue<T>::Empty()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template<class T>
void CommandRunner::SafeQueue<T>::WaitUntilEmpty()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_conditionEmpty.wait(lock, [this] { return m_queue.empty(); });
}
