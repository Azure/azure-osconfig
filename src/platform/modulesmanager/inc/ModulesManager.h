// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULESMANAGER_H
#define MODULESMANAGER_H

#include <chrono>
#include <ctime>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <rapidjson/document.h>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <ManagementModule.h>
#include <Mpi.h>

#define MODULESMANAGER_LOGFILE "/var/log/osconfig_platform.log"
#define MODULESMANAGER_ROLLEDLOGFILE "/var/log/osconfig_platform.bak"

namespace Tests
{
    class ModuleManagerTests_ModuleCleanup_Test;
    class ModuleManagerTests;
} // namespace Tests

class ModulesManagerLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_log;
    }

    static void OpenLog()
    {
        m_log = ::OpenLog(MODULESMANAGER_LOGFILE, MODULESMANAGER_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_log);
    }

    static OSCONFIG_LOG_HANDLE m_log;
};

class ModulesManager
{
public:
    ModulesManager(std::string clientName, const unsigned int maxPayloadSizeBytes = 0);
    virtual ~ModulesManager();

    std::string GetClientName();

    // Set the default cleanup timespan in seconds for Management Modules (MMs), unloads the MM if Lifespan is "Short"
    void SetDefaultCleanupTimespan(unsigned int timespan);

    // Loads Management Modules (MMs) into the Modules Manager
    int LoadModules();
    int LoadModules(std::string modulePath);
    int LoadModules(std::string modulePath, std::string configJson);

    int MpiSet(const char* componentName, const char* objectName, const MPI_JSON_STRING payload, const int payloadSizeBytes);
    int MpiGet(const char* componentName, const char* objectName, MPI_JSON_STRING* payload, int* payloadSizeBytes);
    int MpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes);
    int MpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes);

    void UnloadModules();
    void UnloadAllModules();
    void DoWork();

protected:
    struct ModuleMetadata
    {
        std::shared_ptr<ManagementModule> module;
        std::chrono::system_clock::time_point lastOperation;
        bool operationInProgress;
    };

    // Module mapping componentName <-> ModuleMetadata
    std::map<std::string, ModuleMetadata> modMap;
    std::vector<ModuleMetadata> modulesToUnload;

private:
    static std::string moduleDirectory;
    static std::string configurationJsonFile;

    std::string clientName;
    unsigned int maxPayloadSizeBytes;

    // Timespan in seconds in order to unload loaded MMs, eg. unload module after x secs
    unsigned int cleanupTimespan;

    int SetReportedObjects(const std::string& configJson);

    int MpiSetDesiredInternal(rapidjson::Document& document);
    int MpiGetReportedInternal(char** payload, int* payloadSizeBytes);

    void ScheduleUnloadModule(ModuleMetadata& mm);
};

#endif // MODULESMANAGER_H