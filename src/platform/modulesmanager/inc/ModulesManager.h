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

#include <ManagementModule.h>
#include <CommonUtils.h>
#include <Logging.h>

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
    ModulesManager(std::string clientName);
    ~ModulesManager();

    // Set the default cleanup timespan in seconds for Management Modules (MMs), unloads the MM if Lifespan is "Short" after
    void SetDefaultCleanupTimespan(unsigned int timespan);
    // Loads Management Modules (MMs) into the ModuleLoader from the default path (/usr/lib/osconfig)
    int LoadModules();
    // Loads Management Modules (MMs) into the ModuleLoader from the given path
    int LoadModules(std::string modulePath, std::string configJson);
    // Perform an MpiSet operation
    int MpiSet(const char* componentName, const char* propertyName, const char* payload, const int payloadSizeBytes);
    // Perform an MpiGet operation
    int MpiGet(const char* componentName, const char* propertyName, char** payload, int* payloadSizeBytes);
    // Perform an MpiSetDesired operation
    int MpiSetDesired(const char* clientName, const char* payload, const int payloadSizeBytes);
    // Perform an MpiGetReported operation
    int MpiGetReported(const char* clientName, const unsigned int maxPayloadSizeBytes, char** payload, int* payloadSizeBytes);
    // Retreives the client of the ModulesManager
    std::string GetClientName();
    void UnloadAllModules();
    void UnloadModules();
    void SetMaxPayloadSize(unsigned int maxSize);
    void DoWork();

private:
    struct ModuleMetadata
    {
        std::shared_ptr<ManagementModule> module;
        std::chrono::system_clock::time_point lastOperation;
        bool operationInProgress;
    };

    int SetReportedObjects(const std::string& configJson);
    int MpiSetInternal(const char* componentName, const char* propertyName, const char* payload, const int payloadSizeBytes);
    int MpiGetInternal(const char* componentName, const char* propertyName, char** payload, int* payloadSizeBytes);
    int MpiSetDesiredInternal(rapidjson::Document& document);
    int MpiGetReportedInternal(char** payload, int* payloadSizeBytes);
    ModuleMetadata* GetModuleMetadata(const char* componentName);
    void ScheduleUnloadModule(ModuleMetadata& mm);

    // Module mapping componentName <-> ModuleMetadata
    std::map<std::string, ModuleMetadata> modMap;
    // Vector of modules to unload
    std::vector<ModuleMetadata> modulesToUnload;
    std::hash<std::string> hashString;

    // Timespan in seconds in order to unload loaded MMs, eg. unload module after x secs
    unsigned int cleanupTimespan;
    // The name of the client using the module manager
    std::string clientName;
    // The maximum payload size
    int maxPayloadSizeBytes;

    friend class Tests::ModuleManagerTests;
    friend class Tests::ModuleManagerTests_ModuleCleanup_Test;
};

#endif // MODULESMANAGER_H