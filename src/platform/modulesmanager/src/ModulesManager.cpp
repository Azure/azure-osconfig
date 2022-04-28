// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <future>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <thread>
#include <tuple>
#include <vector>

#include <CommonUtils.h>
#include <Logging.h>
#include <ModulesManager.h>
#include <Mpi.h>
#include <ScopeGuard.h>

static const std::string g_moduleDir = "/usr/lib/osconfig";
static const std::string g_moduleExtension = ".so";

static const std::string g_configJson = "/etc/osconfig/osconfig.json";
static const char g_configReported[] = "Reported";
static const char g_configComponentName[] = "ComponentName";
static const char g_configObjectName[] = "ObjectName";

static ModulesManager modulesManager;

OSCONFIG_LOG_HANDLE ModulesManagerLog::m_log = nullptr;

// MPI

void MpiInitialize(void)
{
    ModulesManagerLog::OpenLog();
    modulesManager.LoadModules(g_moduleDir, g_configJson);
};

void MpiShutdown(void)
{
    modulesManager.UnloadModules();
    ModulesManagerLog::CloseLog();
};

MPI_HANDLE MpiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    MPI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (nullptr != handle)
        {
            OsConfigLogInfo(ModulesManagerLog::Get(), "MpiOpen(%s, %u) returned %p", clientName, maxPayloadSizeBytes, handle);
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiOpen(%s, %u) failed", clientName, maxPayloadSizeBytes);
        }
    }};

    if (nullptr != clientName)
    {
        MpiSession* session = new (std::nothrow) MpiSession(modulesManager, clientName, maxPayloadSizeBytes);
        if (nullptr != session && (0 == session->Open()))
        {
            handle = reinterpret_cast<MPI_HANDLE>(session);
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiOpen(%s, %u) failed to open a new client session", clientName, maxPayloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiOpen(%s, %u) called without an invalid client name", clientName, maxPayloadSizeBytes);
    }

    return handle;
}

void MpiClose(MPI_HANDLE handle)
{
    if (nullptr != handle)
    {
        MpiSession* session = reinterpret_cast<MpiSession*>(handle);
        session->Close();
        delete session;
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiClose(%p) called with an invalid session", handle);
    }
}

int MpiSet(
    MPI_HANDLE handle,
    const char* componentName,
    const char* objectName,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MPI_OK;
    MpiSession* session = reinterpret_cast<MpiSession*>(handle);

    if (nullptr != session)
    {
        status = session->Set(componentName, objectName, payload, payloadSizeBytes);
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet called with invalid client session '%p'", handle);
        status = EINVAL;
    }

    return status;
}

int MpiGet(
    MPI_HANDLE handle,
    const char* componentName,
    const char* objectName,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MPI_OK;
    MpiSession* session = reinterpret_cast<MpiSession*>(handle);

    if (nullptr != session)
    {
        status = session->Get(componentName, objectName, payload, payloadSizeBytes);
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGet called with invalid client session '%p'", handle);
        status = EINVAL;
    }

    return status;
}

int MpiSetDesired(
    MPI_HANDLE handle,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MPI_OK;
    MpiSession* session = reinterpret_cast<MpiSession*>(handle);

    if (nullptr != session)
    {
        if (MPI_OK != (status = session->SetDesired(payload, payloadSizeBytes)))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired(%p, %p, %d) failed to set the desired state", handle, payload, payloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired(%p, %p, %d) called without an invalid handle", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }

    return status;
}

int MpiGetReported(
    MPI_HANDLE handle,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MPI_OK;
    MpiSession* session = reinterpret_cast<MpiSession*>(handle);

    if (nullptr != session)
    {
        if (MPI_OK != (status = session->GetReported(payload, payloadSizeBytes)))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported(%p, %p, %p) failed to get the reported state", handle, payload, payloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported(%p, %p, %p) called without an invalid handle", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }

    return status;
}

void MpiFree(MPI_JSON_STRING payload)
{
    delete[] payload;
}

ModulesManager::ModulesManager() {}

ModulesManager::~ModulesManager()
{
    UnloadModules();
}

int ModulesManager::LoadModules(std::string modulePath, std::string configJson)
{
    int status = 0;

    DIR* dir;
    struct dirent* ent;
    std::vector<std::string> fileList;

    OsConfigLogInfo(ModulesManagerLog::Get(), "Loading modules from: %s", modulePath.c_str());

    if ((dir = opendir(modulePath.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            std::string filename = modulePath + "/" + ent->d_name;
            // Find all .so's
            if (filename.length() > g_moduleExtension.length() && (0 == filename.compare(filename.length() - g_moduleExtension.length(), g_moduleExtension.length(), g_moduleExtension.c_str())))
            {
                fileList.push_back(filename);
            }
        }
        closedir(dir);

        sort(fileList.begin(), fileList.end());

        // Build map for module name -> ManagementModule
        for (auto &filePath : fileList)
        {
            std::shared_ptr<ManagementModule> mm = std::make_shared<ManagementModule>(filePath);
            if (0 == mm->Load())
            {
                ManagementModule::Info info = mm->GetInfo();

                if (m_modules.find(info.name) != m_modules.end())
                {
                    auto currentInfo = m_modules[info.name]->GetInfo();

                    // Use the module with the latest version
                    if (currentInfo.version < info.version)
                    {
                        OsConfigLogInfo(ModulesManagerLog::Get(), "Found newer version of '%s' module (v%s), loading newer version from '%s'", info.name.c_str(), info.version.ToString().c_str(), filePath.c_str());
                        m_modules[info.name] = mm;

                        RegisterModuleComponents(info.name, info.components, true);
                    }
                    else
                    {
                        OsConfigLogInfo(ModulesManagerLog::Get(), "Newer version of '%s' module already loaded (v%s), skipping '%s'", info.name.c_str(), currentInfo.version.ToString().c_str(), filePath.c_str());
                    }
                }
                else
                {
                    m_modules[info.name] = mm;
                    RegisterModuleComponents(info.name, info.components);
                }
            }
        }

        status = SetReportedObjects(configJson);
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to open directory: %s", modulePath.c_str());
        status = ENOENT;
    }

    return status;
}

int ModulesManager::SetReportedObjects(const std::string& configJson)
{
    int status = MPI_OK;
    std::ifstream ifs(configJson);

    if (!ifs.good())
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to open configuration file: %s", configJson.c_str());
        return ENOENT;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document document;
    if (document.ParseStream(isw).HasParseError())
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to parse configuration file: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.IsObject())
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Root configuration JSON is not an object: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.HasMember(g_configReported))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "No valid %s array in configuration: %s", g_configReported, configJson.c_str());
        status = EINVAL;
    }
    else if (!document[g_configReported].IsArray())
    {
        OsConfigLogError(ModulesManagerLog::Get(), "%s is not an array in configuration: %s", g_configReported, configJson.c_str());
        status = EINVAL;
    }
    else
    {
        int index = 0;
        std::set<std::pair<std::string, std::string>> objects;
        for (auto& reported : document[g_configReported].GetArray())
        {
            if (reported.IsObject())
            {
                if (reported.HasMember(g_configComponentName) && reported.HasMember(g_configObjectName))
                {
                    std::string componentName = reported[g_configComponentName].GetString();
                    std::string objectName = reported[g_configObjectName].GetString();

                    if (m_reportedComponents.find(componentName) == m_reportedComponents.end())
                    {
                        m_reportedComponents[componentName] = std::vector<std::string>();
                    }

                    if (objects.find({componentName, objectName}) == objects.end())
                    {
                        objects.insert({componentName, objectName});
                        m_reportedComponents[componentName].push_back(objectName);
                    }
                }
                else
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "'%s' or '%s' missing at index %d", g_configComponentName, g_configObjectName, index);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "%s array element %d is not an object: %s", g_configReported, index, configJson.c_str());
                status = EINVAL;
            }
        }
    }

    return status;
}

void ModulesManager::RegisterModuleComponents(const std::string& moduleName, const std::vector<std::string>& components, bool replace)
{
    for (auto& component : components)
    {
        if (replace || (m_moduleComponentName.find(component) == m_moduleComponentName.end()))
        {
            m_moduleComponentName[component] = moduleName;
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Component '%s' is already registered to module '%s'", component.c_str(), m_moduleComponentName[component].c_str());
        }
    }
}

void ModulesManager::UnloadModules()
{
    for (auto& module : m_modules)
    {
        module.second->Unload();
        module.second.reset();
    }

    m_modules.clear();
}

MpiSession::MpiSession(ModulesManager& modulesManager, std::string clientName, unsigned int maxPayloadSizeBytes) :
    m_modulesManager(modulesManager),
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}

MpiSession::~MpiSession()
{
    Close();
}

int MpiSession::Open()
{
    int status = 0;

    for (auto& module : m_modulesManager.m_modules)
    {
        int mmiStatus = 0;
        std::shared_ptr<MmiSession> mmiSession = std::make_shared<MmiSession>(module.second, m_clientName, m_maxPayloadSizeBytes);

        if (0 == (mmiStatus = mmiSession->Open()))
        {
            m_mmiSessions[module.first] = mmiSession;
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Unable to open MMI session for module '%s'", module.first.c_str());
            status = EINVAL;
        }
    }

    return status;
}

void MpiSession::Close()
{
    for (auto& mmiSession : m_mmiSessions)
    {
        mmiSession.second->Close();
        mmiSession.second.reset();
    }

    m_mmiSessions.clear();
}

std::shared_ptr<MmiSession> MpiSession::GetSession(const std::string& componentName)
{
    std::shared_ptr<MmiSession> mmiSession;

    if (m_modulesManager.m_moduleComponentName.find(componentName) != m_modulesManager.m_moduleComponentName.end())
    {
        std::string moduleName = m_modulesManager.m_moduleComponentName[componentName];
        if (m_mmiSessions.find(moduleName) != m_mmiSessions.end())
        {
            mmiSession = m_mmiSessions[moduleName];
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Unable to find MMI session for component '%s'", componentName.c_str());
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to find module for component '%s'", componentName.c_str());
    }

    return mmiSession;
}

int MpiSession::Set(const char* componentName, const char* objectName, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if (MPI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (0 >= payloadSizeBytes)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid payloadSizeBytes: %d", payloadSizeBytes);
        status = EINVAL;
    }
    else
    {
        std::shared_ptr<MmiSession> moduleSession;
        if (nullptr != (moduleSession = GetSession(componentName)))
        {
            status = moduleSession->Set(componentName, objectName, (MMI_JSON_STRING)payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiSet componentName %s not found", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int MpiSession::Get(const char* componentName, const char* objectName, MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if ((MMI_OK == status) && (nullptr != *payload) && (nullptr != payloadSizeBytes) && (0 != *payloadSizeBytes))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::shared_ptr<MmiSession> moduleSession;
        if (nullptr != (moduleSession = GetSession(componentName)))
        {
            status = moduleSession->Get(componentName, objectName, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiSet componentName %s not found", componentName);
            status = EINVAL;
        }
    }

    return status;
}

int MpiSession::SetDesired(const MPI_JSON_STRING payload, int payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MPI_OK == status)
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired invalid payload: %s", payload);
        status = EINVAL;
    }
    else if (0 == payloadSizeBytes)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired invalid payloadSizeBytes: %d", payloadSizeBytes);
        status = EINVAL;
    }
    else
    {
        rapidjson::Document document;
        document.Parse(payload, payloadSizeBytes);

        if (document.HasParseError())
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
            }

            status = EINVAL;
        }
        else if (!document.IsObject())
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
            }

            status = EINVAL;
        }
        else
        {
            status = SetDesiredPayload(document);
        }
    }

    return status;
}

int MpiSession::SetDesiredPayload(rapidjson::Document& document)
{
    int status = MPI_OK;

    for (auto& component : document.GetObject())
    {
        if (component.value.IsObject())
        {
            std::string componentName = component.name.GetString();
            std::shared_ptr<MmiSession> module;

            if (nullptr != (module = GetSession(componentName)))
            {
                for (auto& object : component.value.GetObject())
                {
                    int moduleStatus = MMI_OK;
                    std::string objectName = object.name.GetString();

                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    object.value.Accept(writer);

                    moduleStatus = module->Set(componentName.c_str(), objectName.c_str(), (MMI_JSON_STRING)buffer.GetString(), buffer.GetSize());

                    if ((moduleStatus != MMI_OK) && IsFullLoggingEnabled())
                    {
                        OsConfigLogError(ModulesManagerLog::Get(), "MmiSet(%s, %s, %s, %d) to %s returned %d", componentName.c_str(), objectName.c_str(), buffer.GetString(), static_cast<int>(buffer.GetSize()), module->GetInfo().name.c_str(), moduleStatus);
                    }
                }
            }
            else
            {
                status = EINVAL;
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "Unable to find module for component %s", componentName.c_str());
                }
            }
        }
        else
        {
            status = EINVAL;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "Component value is not an object");
            }
        }
    }

    return status;
}

int MpiSession::GetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MPI_OK == status)
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported invalid payload: %p", payload);
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported invalid payloadSizeBytes: %p", payloadSizeBytes);
        status = EINVAL;
    }
    else
    {
        *payload = nullptr;
        *payloadSizeBytes = 0;
        status = GetReportedPayload(payload, payloadSizeBytes);
    }

    return status;
}

int MpiSession::GetReportedPayload(MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MPI_OK;
    rapidjson::Document document;
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    document.SetObject();

    for (auto reported : m_modulesManager.m_reportedComponents)
    {
        std::string componentName = reported.first;
        std::vector<std::string> objectNames = reported.second;
        std::shared_ptr<MmiSession> module = GetSession(componentName);

        if ((nullptr != module) && !objectNames.empty())
        {
            rapidjson::Value component(rapidjson::kObjectType);
            for (auto& objectName : objectNames)
            {
                char* objectPayload = nullptr;
                int objectPayloadSizeBytes = 0;
                int moduleStatus = MMI_OK;

                moduleStatus = module->Get(componentName.c_str(), objectName.c_str(), &objectPayload, &objectPayloadSizeBytes);

                if ((MMI_OK == moduleStatus) && (nullptr != objectPayload) && (0 < objectPayloadSizeBytes))
                {
                    std::string objectPayloadString(objectPayload, objectPayloadSizeBytes);
                    rapidjson::Document objectDocument;
                    objectDocument.Parse(objectPayloadString.c_str());

                    if (!objectDocument.HasParseError())
                    {
                        rapidjson::Value object(rapidjson::kObjectType);
                        object.CopyFrom(objectDocument, allocator);
                        component.AddMember(rapidjson::Value(objectName.c_str(), allocator), object, allocator);
                    }
                    else if (IsFullLoggingEnabled())
                    {
                        OsConfigLogError(ModulesManagerLog::Get(), "MmiGet(%s, %s) returned invalid payload: %s", componentName.c_str(), objectName.c_str(), objectPayloadString.c_str());
                    }
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "MmiGet(%s, %s) returned %d", componentName.c_str(), objectName.c_str(), moduleStatus);
                }
            }
            document.AddMember(rapidjson::Value(componentName.c_str(), allocator), component, allocator);
        }
    }

    try
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        document.Accept(writer);

        *payloadSizeBytes = buffer.GetSize();
        *payload = new (std::nothrow) char[*payloadSizeBytes];
        if (nullptr == *payload)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported unable to allocate %d bytes", *payloadSizeBytes);
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
        OsConfigLogError(ModulesManagerLog::Get(), "Could not allocate payload: %s", e.what());
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

    return status;
}