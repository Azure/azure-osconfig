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
#include <set>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <ManagementModule.h>
#include <Mpi.h>

#define MODULESMANAGER_LOGFILE "/var/log/osconfig_platform.log"
#define MODULESMANAGER_ROLLEDLOGFILE "/var/log/osconfig_platform.bak"

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

class MpiSession;

class ModulesManager
{
public:
    ModulesManager();
    ~ModulesManager();

    int LoadModules(std::string modulePath, std::string configJson);
    void UnloadModules();

protected:
    std::map<std::string, std::vector<std::string>> m_reportedComponents;
    std::map<std::string, std::string> m_moduleComponentName;
    std::map<std::string, std::shared_ptr<ManagementModule>> m_modules;

    int SetReportedObjects(const std::string& configJson);
    void RegisterModuleComponents(const std::string& moduleName, const std::vector<std::string>& components, bool replace = false);

    friend class MpiSession;
};

class MpiSession
{
public:
    MpiSession(ModulesManager& modulesManager, std::string clientName, const unsigned int maxPayloadSizeBytes = 0);
    ~MpiSession();

    int Open();
    void Close();

    int Set(const char* componentName, const char* objectName, const MPI_JSON_STRING payload, const int payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MPI_JSON_STRING* payload, int* payloadSizeBytes);
    int SetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes);
    int GetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes);

private:
    ModulesManager& m_modulesManager;
    std::string m_clientName;
    unsigned int m_maxPayloadSizeBytes;

    std::map<std::string, std::shared_ptr<MmiSession>> m_mmiSessions;
    std::shared_ptr<MmiSession> GetSession(const std::string& componentName);

    int SetDesiredPayload(rapidjson::Document& document);
    int GetReportedPayload(MPI_JSON_STRING* payload, int* payloadSizeBytes);
};

#endif // MODULESMANAGER_H