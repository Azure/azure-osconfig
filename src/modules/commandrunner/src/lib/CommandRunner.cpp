// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

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
        if (0 != LoadPersistedCommandStatus(clientName))
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to load persisted command status for client %s", clientName.c_str());
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
    }
}

int CommandRunner::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
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
    rapidjson::Document document;

    if (document.Parse(payload, payloadSizeBytes).HasParseError())
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Unabled to parse JSON payload: %s", payload);
        status = EINVAL;
    }
    else
    {
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
                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogInfo(CommandRunnerLog::Get(), "Updating command (%s) loaded from disk, with complete payload", arguments.m_id.c_str());
                        }

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
                            OsConfigLogError(CommandRunnerLog::Get(), "Unsupported action: %d", static_cast<int>(arguments.m_action));
                            status = EINVAL;
                    }
                }
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Invalid object name: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int CommandRunner::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == payload)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;

        if (0 == CommandRunner::m_componentName.compare(componentName))
        {
            if (0 == g_commandStatus.compare(objectName))
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

                Command::Status commandStatus = GetReportedStatus();
                Command::Status::Serialize(writer, commandStatus);

                *payload = new (std::nothrow) char[buffer.GetSize()];

                if (nullptr != *payload)
                {
                    std::fill(*payload, *payload + buffer.GetSize(), 0);
                    std::memcpy(*payload, buffer.GetString(), buffer.GetSize());
                    *payloadSizeBytes = buffer.GetSize();
                }
                else
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
                    status = ENOMEM;
                }
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Invalid object name: %s", objectName);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Invalid component name: %s", componentName);
            status = EINVAL;
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
        OsConfigLogError(CommandRunnerLog::Get(), "Command does not exist and cannot be canceled: %s", id.c_str());
        status = EINVAL;
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
                }
            }
            else
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist command to disk. Skipping command: %s", command->GetId().c_str());
            }
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Command already exists with id: %s", command->GetId().c_str());
            status = EINVAL;
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Command already recieved: %s (%s)", command->GetId().c_str(), command->m_arguments.c_str());
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
            OsConfigLogError(CommandRunnerLog::Get(), "Cannot cache command with duplicate id: %s", command->GetId().c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Cannot cache command with empty id");
        status = EINVAL;
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

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' (%s) completed with code: %d", command->GetId().c_str(), command->m_arguments.c_str(), exitCode);
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
        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document document;

        if (document.ParseStream(isw).HasParseError())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to parse cache file");
            status = EINVAL;
        }
        else if (!document.IsObject())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Cache file JSON is not an array");
            status = EINVAL;
        }
        else if (document.HasMember(clientName.c_str()))
        {
            const rapidjson::Value& client = document[clientName.c_str()];

            for (auto& it : client.GetArray())
            {
                Command::Status commandStatus = Command::Status::Deserialize(it);

                std::shared_ptr<Command> command = std::make_shared<Command>(commandStatus.m_id, "", 0, "");
                command->SetStatus(commandStatus.m_exitCode, commandStatus.m_textResult, commandStatus.m_state);

                if (0 != CacheCommand(command))
                {
                    OsConfigLogError(CommandRunnerLog::Get(), "Failed to cache command: %s", commandStatus.m_id.c_str());
                    status = -1;
                }
            }
        }
        else if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Cache file does not contain a status for client: %s", clientName.c_str());
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
    rapidjson::Document document;
    rapidjson::Document statusDocument;
    statusDocument.Parse(Command::Status::Serialize(commandStatus, false).c_str());
    std::lock_guard<std::mutex> lock(m_diskCacheMutex);

    std::ifstream file(m_persistedCacheFile);
    if (file.good())
    {
        rapidjson::IStreamWrapper isw(file);
        if (document.ParseStream(isw).HasParseError() || (!document.IsObject()))
        {
            document.Parse(m_defaultCacheTemplate);
        }
    }
    else
    {
        document.Parse(m_defaultCacheTemplate);
    }

    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    if (document.HasMember(clientName.c_str()))
    {
        bool updated = false;
        rapidjson::Value& client = document[clientName.c_str()];

        if (!client.IsArray())
        {
            client.SetArray();
        }

        for (auto& it : client.GetArray())
        {
            if (it.HasMember(g_commandId.c_str()) && it[g_commandId.c_str()].IsString() && (it[g_commandId.c_str()].GetString() == commandStatus.m_id))
            {
                it.CopyFrom(statusDocument, allocator);
                updated = true;
                break;
            }
        }

        if (!updated)
        {
            if (client.Size() >= m_maxCacheSize)
            {
                client.Erase(client.Begin());
            }

            client.PushBack(statusDocument, allocator);
        }
    }
    else
    {
        rapidjson::Value object(rapidjson::kArrayType);
        object.PushBack(statusDocument, allocator);
        document.AddMember(rapidjson::Value(clientName.c_str(), allocator), object, allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    if (buffer.GetSize() > 0)
    {
        std::FILE* file = std::fopen(m_persistedCacheFile, "w+");
        if (nullptr == file)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to open file: %s", m_persistedCacheFile);
            status = EACCES;
        }
        else
        {
            int rc = std::fputs(buffer.GetString(), file);

            if ((0 > rc) || (EOF == rc))
            {
                status = errno ? errno : EINVAL;
                OsConfigLogError(CommandRunnerLog::Get(), "Failed write to file %s, error: %d %s", m_persistedCacheFile, status, errno ? strerror(errno) : "-");
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
