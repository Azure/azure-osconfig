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

constexpr const char info[] = R""""({
    "Name": "CommandRunner",
    "Description": "Provides functionality to remotely run commands on the device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["CommandRunner"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

const std::string CommandRunner::PERSISTED_COMMANDSTATUS_FILE = "/etc/osconfig/osconfig_commandrunner.cache";

std::mutex CommandRunner::m_diskCacheMutex;

std::map<std::string, std::shared_ptr<CommandRunner::Factory::Session>> CommandRunner::Factory::m_sessions;
std::mutex CommandRunner::Factory::m_mutex;

CommandRunner::CommandRunner(std::string clientName, unsigned int maxPayloadSizeBytes, bool usePersistedCache) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_usePersistedCache(usePersistedCache)
{
    if (m_usePersistedCache && (0 != LoadPersistedCommandStatus(clientName)))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to load persisted command status for client %s", clientName.c_str());
    }

    // Start the worker thread
    m_workerThread = std::thread(&CommandRunner::WorkerThread, std::ref(*this));
}

CommandRunner::~CommandRunner()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "Terminating command runner session for: %s", m_clientName.c_str());

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
        std::size_t len = ARRAY_SIZE(info) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, info, len);
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
        if (0 == g_commandRunner.compare(componentName))
        {
            if (0 == g_commandArguments.compare(objectName))
            {
                Command::Arguments arguments = Command::Arguments::Deserialize(document);

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

        if (0 == g_commandRunner.compare(componentName))
        {
            if (0 == g_commandStatus.compare(objectName))
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

                Command::Status commandStatus = GetReportedStatus();
                Command::Status::Serialize(writer, commandStatus);
                status = CopyJsonPayload(payload, payloadSizeBytes, buffer);
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

    if (CommandExists(id))
    {
        SetReportedStatusId(id);
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Command does not exist and cannot be refreshed: %s", id.c_str());
        status = EINVAL;
    }

    return status;
}

bool CommandRunner::CommandExists(const std::string& id)
{
    bool exists = false;
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (m_commandMap.find(id) != m_commandMap.end())
    {
        exists = true;
    }

    return exists;
}

int CommandRunner::ScheduleCommand(std::shared_ptr<Command> command)
{
    int status = 0;

    if (!CommandExists(command->GetId()))
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
        OsConfigLogError(CommandRunnerLog::Get(), "Command already exists: %s", command->GetId().c_str());
        status = EINVAL;
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
            while (m_cacheBuffer.size() > CommandRunner::MAX_CACHE_SIZE)
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
    std::ifstream file(CommandRunner::PERSISTED_COMMANDSTATUS_FILE);

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
            Command::Status commandStatus = Command::Status::Deserialize(client);

            std::shared_ptr<Command> command = std::make_shared<Command>(commandStatus.m_id, "", 0, "");
            command->SetStatus(commandStatus.m_exitCode, commandStatus.m_textResult, commandStatus.m_state);

            if (0 != CacheCommand(command))
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Failed to cache command: %s", commandStatus.m_id.c_str());
                status = -1;
            }
        }
        else if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Cache file does not contain a status for client: %s", clientName.c_str());
        }
    }
    else
    {
        OsConfigLogInfo(CommandRunnerLog::Get(), "Cache file does not exist");
        status = -1;
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
    std::lock_guard<std::mutex> lock(m_diskCacheMutex);

    std::ifstream file(CommandRunner::PERSISTED_COMMANDSTATUS_FILE);
    if (file.good())
    {
        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document document;

        if (document.ParseStream(isw).HasParseError())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to parse cache file: %s", CommandRunner::PERSISTED_COMMANDSTATUS_FILE.c_str());
            status = EINVAL;
        }
        else if (!document.IsObject())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Cache file JSON is not an object: %s", CommandRunner::PERSISTED_COMMANDSTATUS_FILE.c_str());
            status = EINVAL;
        }
        else
        {
            rapidjson::Document statusDocument;
            statusDocument.Parse(Command::Status::Serialize(commandStatus, false).c_str());

            rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

            if (document.HasMember(clientName.c_str()))
            {
                document[clientName.c_str()].CopyFrom(statusDocument, allocator);
            }
            else
            {
                rapidjson::Value object(rapidjson::kObjectType);
                object.CopyFrom(statusDocument, allocator);
                document.AddMember(rapidjson::Value(clientName.c_str(), allocator), object, allocator);
            }

            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);

            if (0 != (status = WriteFile(CommandRunner::PERSISTED_COMMANDSTATUS_FILE, buffer)))
            {
                OsConfigLogError(CommandRunnerLog::Get(), "Failed to write cache file: %s", CommandRunner::PERSISTED_COMMANDSTATUS_FILE.c_str());
            }
        }
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to open cache file: %s", CommandRunner::PERSISTED_COMMANDSTATUS_FILE.c_str());
        status = errno;
    }

    return status;
}

int CommandRunner::WriteFile(const std::string& fileName, const rapidjson::StringBuffer& buffer)
{
    int status = 0;

    if (buffer.GetSize() > 0)
    {
        std::FILE* file = std::fopen(fileName.c_str(), "w");
        if (nullptr == file)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to open file: %s", fileName.c_str());
            status = EACCES;
        }
        else
        {
            int rc = std::fputs(buffer.GetString(), file);

            if ((0 > rc) || (EOF == rc))
            {
                status = errno ? errno : EINVAL;
                OsConfigLogError(CommandRunnerLog::Get(), "Failed write to file %s, error: %d %s", fileName.c_str(), status, errno ? strerror(errno) : "-");
            }

            fflush(file);
            std::fclose(file);
        }
    }

    return 0;
}

int CommandRunner::CopyJsonPayload(MMI_JSON_STRING* payload, int* payloadSizeBytes, const rapidjson::StringBuffer& buffer)
{
    int status = MMI_OK;

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::fill(*payload, *payload + buffer.GetSize(), 0);
            std::memcpy(*payload, buffer.GetString(), buffer.GetSize());
            *payloadSizeBytes = buffer.GetSize();
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Could not allocate payload: %s", e.what());
        status = EINTR;

        if (nullptr != *payload)
        {
            delete[] *payload;
            *payload = nullptr;
        }

        if (nullptr != payloadSizeBytes)
        {
            *payloadSizeBytes = 0;
        }
    }

    return status;
}

std::shared_ptr<CommandRunner> CommandRunner::Factory::Create(std::string clientName, int maxPayloadSizeBytes)
{
    std::shared_ptr<Factory::Session> session;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_sessions.find(clientName) == m_sessions.end())
    {
        session = std::make_shared<Factory::Session>(clientName, maxPayloadSizeBytes);
        m_sessions[clientName] = session;
    }
    else
    {
        session = m_sessions[clientName];
    }

    return session->Get();
}

void CommandRunner::Factory::Destroy(CommandRunner* commandRunner)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string clientName = commandRunner->GetClientName();

    if (m_sessions.find(clientName) != m_sessions.end())
    {
        if (0 == m_sessions[clientName]->Release())
        {
            m_sessions[clientName].reset();
            m_sessions.erase(clientName);
        }
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(CommandRunnerLog::Get(), "CommandRunner not found for session: %s", clientName.c_str());
    }
}

void CommandRunner::Factory::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it)
    {
        it->second.reset();
    }

    m_sessions.clear();
}

CommandRunner::Factory::Session::Session(std::string clientName, int maxPayloadSizeBytes) :
    m_clients(0)
{
    m_instance = std::make_shared<CommandRunner>(clientName, maxPayloadSizeBytes);
}

CommandRunner::Factory::Session::~Session()
{
    m_instance.reset();
}

std::shared_ptr<CommandRunner> CommandRunner::Factory::Session::Get()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clients++;
    return m_instance;
}

int CommandRunner::Factory::Session::Release()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return --m_clients;
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