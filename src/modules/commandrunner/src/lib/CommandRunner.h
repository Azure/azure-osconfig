// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <thread>

#include <Command.h>
#include <Mmi.h>

const std::string g_commandRunner = "CommandRunner";

const std::string g_clientName = "ClientName";
const std::string g_commandStatusValues = "CommandStatusValues";

#define CACHEFILE "/etc/osconfig/osconfig_commandrunner.cache"

class CommandRunner
{
public:
    static const unsigned int MAX_CACHE_SIZE = 10;

    CommandRunner(std::string name, unsigned int maxSizeInBytes = 0, std::function<int()> persistCacheFunction = nullptr);
    virtual ~CommandRunner();

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    unsigned int GetMaxPayloadSizeBytes();

    Command::Status GetStatusToPersist();
    void WaitForCommands();
    std::string GetClientName();

private:
    class SafeQueue
    {
    public:
        SafeQueue();
        ~SafeQueue() { }

        void Push(std::weak_ptr<Command> element);
        std::weak_ptr<Command> Pop();
        std::weak_ptr<Command> Front();
        bool Empty();
        void WaitUntilEmpty();

    private:
        std::queue<std::weak_ptr<Command>> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_condition;
        std::condition_variable m_conditionEmpty;
    };

    const std::string m_clientName;
    const unsigned int m_maxPayloadSizeBytes;
    std::function<int()> m_persistCacheFunction;

    std::thread m_workerThread;
    SafeQueue m_commandQueue;

    std::deque<std::shared_ptr<Command>> m_cacheBuffer;
    std::map<std::string, std::weak_ptr<Command>> m_commandMap;
    std::mutex m_cacheMutex;

    std::string m_reportedStatusId;
    std::mutex m_reportedStatusIdMutex;

    int Run(const std::string id, std::string arguments, unsigned int timeout, bool singleLineTextResult);
    int Reboot(const std::string id);
    int Shutdown(const std::string id);
    int Cancel(const std::string id);
    void CancelAll();
    int Refresh(const std::string id);

    int ScheduleCommand(std::shared_ptr<Command> command);
    int CacheCommand(std::shared_ptr<Command> command);

    void SetReportedStatusId(const std::string id);
    std::string GetReportedStatusId();
    Command::Status GetReportedStatus();

    static void WorkerThread(CommandRunner& instance);
    void Execute(Command command);

    static int CopyJsonPayload(rapidjson::StringBuffer& buffer, MMI_JSON_STRING* payload, int* payloadSizeBytes);
};

#endif // COMMANDRUNNER_H