// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <condition_variable>
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

class CommandRunner
{
public:
    static const unsigned int MAX_CACHE_SIZE = 10;
    static const std::string PERSISTED_COMMANDSTATUS_FILE;

    CommandRunner(std::string name, unsigned int maxSizeInBytes = 0, bool usePersistedCache = true);
    ~CommandRunner();

    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    const std::string& GetClientName() const;
    unsigned int GetMaxPayloadSizeBytes() const;

    // Helper method to wait for the worker thread during unit tests
    void WaitForCommands();

    class Factory
    {
    public:
        static std::shared_ptr<CommandRunner> Create(std::string clientName, int maxPayloadSizeBytes = 0);
        static void Destroy(CommandRunner* commandRunner);
        static void Clear();

        // Helper method for validating sessions during unit tests
        static int GetClientCount(const std::string& clientName);

    private:
        class Session
        {
        public:
            Session(std::string clientName, int maxPayloadSizeBytes);
            ~Session();

            std::shared_ptr<CommandRunner> Get();
            int Release();
            int GetClientCount();

        private:
            std::mutex m_mutex;
            int m_clients;
            std::shared_ptr<CommandRunner> m_instance;
        };

        static std::map<std::string, std::shared_ptr<Session>> m_sessions;
        static std::mutex m_mutex;

        Factory() = delete;
        ~Factory() = delete;
    };

    friend class Factory;

private:
    template<class T>
    class SafeQueue
    {
    public:
        SafeQueue();
        ~SafeQueue() { }

        void Push(T element);
        T Pop();
        T Front();
        bool Empty();
        void WaitUntilEmpty();

    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_condition;
        std::condition_variable m_conditionEmpty;
    };

    const std::string m_clientName;
    const unsigned int m_maxPayloadSizeBytes;
    const bool m_usePersistedCache;

    std::thread m_workerThread;
    SafeQueue<std::weak_ptr<Command>> m_commandQueue;

    std::deque<std::shared_ptr<Command>> m_cacheBuffer;
    std::map<std::string, std::shared_ptr<Command>> m_commandMap;
    std::mutex m_cacheMutex;

    std::string m_reportedStatusId;
    std::mutex m_reportedStatusIdMutex;

    static std::mutex m_diskCacheMutex;

    int Run(const std::string id, std::string arguments, unsigned int timeout, bool singleLineTextResult);
    int Reboot(const std::string id);
    int Shutdown(const std::string id);
    int Cancel(const std::string id);
    void CancelAll();
    int Refresh(const std::string id);

    bool CommandExists(const std::string& id);
    int ScheduleCommand(std::shared_ptr<Command> command);
    int CacheCommand(std::shared_ptr<Command> command);

    void SetReportedStatusId(const std::string id);
    std::string GetReportedStatusId();
    Command::Status GetReportedStatus();

    static void WorkerThread(CommandRunner& instance);
    void Execute(Command command);

    Command::Status GetStatusToPersist();
    int LoadPersistedCommandStatus(const std::string& clientName);
    int PersistCommandStatus(const Command::Status& status);
    static int PersistCommandStatus(const std::string& clientName, const Command::Status status);

    static int WriteFile(const std::string& fileName, const rapidjson::StringBuffer& buffer);
    static int CopyJsonPayload(MMI_JSON_STRING* payload, int* payloadSizeBytes, const rapidjson::StringBuffer& buffer);
};

#endif // COMMANDRUNNER_H