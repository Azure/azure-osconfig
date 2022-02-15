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

static const std::string g_commandRunnerCacheFile = "/etc/osconfig/osconfig_commandrunner.cache";
static const unsigned int g_maxCommandCacheSize = 10;

CommandRunner::CommandRunner(std::string clientName, unsigned int maxPayloadSizeBytes, std::function<int()> persistCacheFunction) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_persistCacheFunction(persistCacheFunction)
{
    std::ifstream file(g_commandRunnerCacheFile);
    if (file.good())
    {
        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document document;

        if (document.ParseStream(isw).HasParseError())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to parse cache file: %s", g_commandRunnerCacheFile.c_str());
        }
        else if (!document.IsArray())
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Cache file JSON is not an array: %s", g_commandRunnerCacheFile.c_str());
        }
        else
        {
            for (auto& client : document.GetArray())
            {
                if (client.IsObject() && client.HasMember(g_clientName.c_str()) && client.HasMember(g_commandStatusValues.c_str()) && client[g_commandStatusValues.c_str()].IsArray())
                {
                    std::string name = client[g_clientName.c_str()].GetString();
                    if (0 == clientName.compare(name))
                    {
                        for (auto& statusJson : client[g_commandStatusValues.c_str()].GetArray())
                        {
                            if (statusJson.IsObject())
                            {
                                Command::Status commandStatus = Command::Status::Deserialize(statusJson.GetObject());

                                std::shared_ptr<Command> command = std::make_shared<Command>(commandStatus.id, "", 0, "");
                                command->SetStatus(commandStatus.exitCode, commandStatus.textResult, commandStatus.state);

                                if (0 == CacheCommand(command))
                                {
                                    OsConfigLogError(CommandRunnerLog::Get(), "Failed to add cache command with id '%s'", command->GetId().c_str());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

CommandRunner::~CommandRunner()
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "CommandRunner %s shutting down", m_clientName.c_str());

    while (!m_commandQueue.Empty())
    {
        std::shared_ptr<Command> command = m_commandQueue.Pop().lock();
        if (nullptr != command)
        {
            command->Cancel();
        }
    }

    try
    {
        m_workerThread.join();
    }
    catch (const std::exception& e) {}

    if (nullptr != m_persistCacheFunction)
    {
        m_persistCacheFunction();
    }
}

int CommandRunner::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo called with null clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo called with null payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo called with null payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        try
        {
            std::size_t len = ARRAY_SIZE(info) - 1;
            *payload = new (std::nothrow) char[len];
            if (nullptr == *payload)
            {
                OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                std::memcpy(*payload, info, len);
                *payloadSizeBytes = len;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "MmiGetInfo exception thrown: %s", e.what());
            status = EINTR;

            if (nullptr != *payload)
            {
                delete[] * payload;
                *payload = nullptr;
            }

            if (nullptr != payloadSizeBytes)
            {
                *payloadSizeBytes = 0;
            }
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

                switch (arguments.action)
                {
                    case Command::Action::RunCommand:
                        status = Run(arguments.id, arguments.command, arguments.timeout, arguments.singleLineTextResult);
                        break;
                    case Command::Action::Reboot:
                        status = Reboot(arguments.id);
                        break;
                    case Command::Action::Shutdown:
                        status = Shutdown(arguments.id);
                        break;
                    case Command::Action::CancelCommand:
                        status = Cancel(arguments.id);
                        break;
                    case Command::Action::RefreshCommandStatus:
                        status = Refresh(arguments.id);
                        break;
                    default:
                        OsConfigLogError(CommandRunnerLog::Get(), "Unsupported action: %d", static_cast<int>(arguments.action));
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
                status = CopyJsonPayload(buffer, payload, payloadSizeBytes);
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

unsigned int CommandRunner::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}

void CommandRunner::WaitForCommands()
{
    if (!m_commandQueue.Empty() && m_workerThread.joinable())
    {
        m_workerThread.join();
    }
}

std::string CommandRunner::GetClientName()
{
    return m_clientName;
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

    if ((m_commandMap.find(id) != m_commandMap.end()) && !m_commandMap[id].expired())
    {
        std::shared_ptr<Command> command = m_commandMap[id].lock();
        OsConfigLogInfo(CommandRunnerLog::Get(), "Canceling command with command id: %s", id.c_str());
        status = command->Cancel();
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Command with id '%s' does not exist and cannot be canceled", id.c_str());
        status = EINVAL;
    }

    return status;
}

int CommandRunner::Refresh(const std::string id)
{
    int status = 0;

    if ((m_commandMap.find(id) != m_commandMap.end()) && !m_commandMap[id].expired())
    {
        SetReportedStatusId(id);
    }
    else
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Command with id '%s' does not exist and cannot be refreshed", id.c_str());
        status = EINVAL;
    }

    return status;
}

int CommandRunner::ScheduleCommand(std::shared_ptr<Command> command)
{
    int status = 0;

    if ((nullptr != m_persistCacheFunction) && (0 != (status = m_persistCacheFunction())))
    {
        OsConfigLogError(CommandRunnerLog::Get(), "Failed to persist command to disk. Skipping command with id '%s'", command->GetId().c_str());
    }
    else
    {
        if (0 == (status = CacheCommand(command)))
        {
            command->SetStatus(0, "", Command::State::Running);
            m_commandQueue.Push(command);

            if (!m_workerThread.joinable())
            {
                m_workerThread = std::thread(&CommandRunner::WorkerThread, std::ref(*this));
            }
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Failed to cache command with id '%s'", command->GetId().c_str());
        }
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
            while (m_cacheBuffer.size() > g_maxCommandCacheSize)
            {
                std::weak_ptr<Command> oldestCommand = m_cacheBuffer.back();
                if (!oldestCommand.expired() && oldestCommand.lock()->IsComplete())
                {
                    m_cacheBuffer.pop_back();
                    m_commandMap.erase(oldestCommand.lock()->GetId());
                }
            }
        }
        else
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Cannot cache command with duplicate id '%s'", command->GetId().c_str());
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

    if ((m_commandMap.find(reportedCommandId) != m_commandMap.end()) && !m_commandMap[reportedCommandId].expired())
    {
        return m_commandMap[reportedCommandId].lock()->GetStatus();
    }
    else
    {
        return Command::Status("", 0, "", Command::State::Unknown);
    }
}

void CommandRunner::WorkerThread(CommandRunner& instance)
{
    OsConfigLogInfo(CommandRunnerLog::Get(), "Starting worker thread...");

    while (!instance.m_commandQueue.Empty())
    {
        std::shared_ptr<Command> command = instance.m_commandQueue.Front().lock();
        int exitCode = command->Execute(instance.m_persistCacheFunction, instance.m_maxPayloadSizeBytes);

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' ('%s') completed with code %d", command->GetId().c_str(), command->command.c_str(), exitCode);
        }
        else
        {
            OsConfigLogInfo(CommandRunnerLog::Get(), "Command '%s' completed with code %d", command->GetId().c_str(), exitCode);
        }

        instance.m_commandQueue.Pop();
    }

    OsConfigLogInfo(CommandRunnerLog::Get(), "Worker thread stopped");
}

Command::Status CommandRunner::GetStatusToPersist()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cacheBuffer.front()->GetStatus();
}

int CommandRunner::CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    try
    {
        *payload = new (std::nothrow) char[buffer.GetSize()];
        if (nullptr == *payload)
        {
            OsConfigLogError(CommandRunnerLog::Get(), "Unable to allocate memory for payload");
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

CommandRunner::SafeQueue::SafeQueue() :
    m_queue(),
    m_mutex(),
    m_condition() { }

void CommandRunner::SafeQueue::Push(std::weak_ptr<Command> value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(value);
    m_condition.notify_one();
}

std::weak_ptr<Command> CommandRunner::SafeQueue::Pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty(); });
    std::weak_ptr<Command> value = m_queue.front();
    m_queue.pop();

    if (m_queue.empty())
    {
        m_conditionEmpty.notify_one();
    }

    return value;
}

std::weak_ptr<Command> CommandRunner::SafeQueue::Front()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty(); });
    return m_queue.front();
}

bool CommandRunner::SafeQueue::Empty()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}