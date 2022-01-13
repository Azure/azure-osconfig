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

constexpr unsigned int g_defaultModuleCleanup = 60 * 30; // 30 minutes
const std::string g_moduleDir = "/usr/lib/osconfig";
const std::string g_moduleExtension = ".so";

const char* g_configJson = "/etc/osconfig/osconfig.json";
const char* g_configReported = "Reported";
const char* g_configComponentName = "ComponentName";
const char* g_configObjectName = "ObjectName";

// The local desired and reported configuration files
const std::string g_desiredFile = "/etc/osconfig/osconfig_desired.json";
const std::string g_reportedFile = "/etc/osconfig/osconfig_reported.json";

// Manager mapping clientName <-> ModulesManager
std::map<std::string, std::weak_ptr<ModulesManager>> manMap;
// Manager mapping MPI_HANDLE <-> ModulesManager
std::map<MPI_HANDLE, std::shared_ptr<ModulesManager>> mpiMap;

OSCONFIG_LOG_HANDLE ModulesManagerLog::m_log = nullptr;

// MPI
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

    ModulesManagerLog::OpenLog();

    if (nullptr == clientName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiOpen called without a clientName.");
        return nullptr;
    }

    if (manMap.end() == manMap.find(clientName))
    {
        // No manager found for clientName, create manager
        auto modulesManager = std::shared_ptr<ModulesManager>(new ModulesManager(clientName));
        MPI_HANDLE mpiHandle = reinterpret_cast<MPI_HANDLE>(modulesManager.get());
        manMap[clientName] = modulesManager;
        mpiMap[mpiHandle] = modulesManager;
#ifndef _TEST_
        modulesManager->LoadModules();
#endif
        handle = mpiHandle;
    }
    else if (!manMap[clientName].expired())
    {
        handle = reinterpret_cast<MMI_HANDLE>(manMap[clientName].lock().get());
        if (nullptr == handle)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MpiOpen already called for %s, but handle is nullptr", clientName);
        }
        else
        {
            OsConfigLogInfo(ModulesManagerLog::Get(), "MpiOpen already called for %s, returning original handle %p", clientName, handle);
        }
    }

    return handle;
}

void MpiClose(MPI_HANDLE clientSession)
{
    OsConfigLogInfo(ModulesManagerLog::Get(), "MpiClose(%p)", clientSession);
    if (mpiMap.end() != mpiMap.find(clientSession))
    {
        mpiMap[clientSession]->UnloadAllModules();
        mpiMap[clientSession].reset();
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiClose invalid MPI_HANDLE. handle=%p", clientSession);
    }

    if (0 == mpiMap.size())
    {
        ModulesManagerLog::CloseLog();
    }
}

int MpiSet(
    MPI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MPI_OK;
    std::string clientName;

    if (mpiMap.end() == mpiMap.find(clientSession))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet called with an invalid clientSession: %p, return: %d", clientSession, EINVAL);
        return EINVAL;
    }

    auto handle = mpiMap[clientSession];
    clientName = handle->GetClientName();
    status = handle->MpiSet(componentName, objectName, payload, payloadSizeBytes);

    return status;
}

int MpiGet(
    MPI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MPI_OK;
    std::string clientName;

    if (mpiMap.end() == mpiMap.find(clientSession))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGet called with an invalid clientSession: %p", clientSession);
        status = EINVAL;
        return status;
    }

    auto handle = mpiMap[clientSession];
    clientName = handle->GetClientName();
    status = handle->MpiGet(componentName, objectName, payload, payloadSizeBytes);

    return status;
}

int MpiSetDesired(
    const char* clientName,
    const MPI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MPI_OK;

    if (manMap.end() == manMap.find(clientName))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired called with an invalid clientName: %s, return: %d", clientName, EINVAL);
        return EINVAL;
    }

    auto handle = manMap[clientName].lock();
    if (nullptr == handle)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired called with an invalid clientName: %s, return: %d", clientName, EINVAL);
        return EINVAL;
    }

    status = handle->MpiSetDesired(clientName, payload, payloadSizeBytes);

    return status;
}

int MpiGetReported(
    const char* clientName,
    const unsigned int mayPayloadSizeBytes,
    MPI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MPI_OK;

    if (manMap.end() == manMap.find(clientName))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported called with an invalid clientName: %s, return: %d", clientName, EINVAL);
        return EINVAL;
    }

    auto handle = manMap[clientName].lock();
    if (nullptr == handle)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported called with an invalid clientName: %s, return: %d", clientName, EINVAL);
        return EINVAL;
    }

    status = handle->MpiGetReported(clientName, mayPayloadSizeBytes, payload, payloadSizeBytes);

    return status;
}

void MpiFree(MPI_JSON_STRING payload)
{
    delete[] payload;
}

void MpiDoWork()
{
    // Perform work on all client sessions
    std::map<MPI_HANDLE, std::shared_ptr<ModulesManager>>::iterator it;
    for (it = mpiMap.begin(); it != mpiMap.end(); ++it)
    {
        it->second->DoWork();
    }
}

ModulesManager::ModulesManager(std::string client) : cleanupTimespan(g_defaultModuleCleanup), clientName(client), maxPayloadSizeBytes(0)
{
}

ModulesManager::~ModulesManager()
{
    UnloadAllModules();
}

int ModulesManager::LoadModules()
{
    return LoadModules(g_moduleDir, g_configJson);
}

int ModulesManager::LoadModules(std::string modulePath, std::string configJson)
{
    DIR* dir;
    struct dirent* ent;
    std::vector<std::string> fileList;

    OsConfigLogInfo(ModulesManagerLog::Get(), "Loading modules using modulePath: %s", modulePath.c_str());

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
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to open directory. modulePath: %s", modulePath.c_str());
        return ENOENT;
    }

    // Build map for componentNames <-> MM
    for (auto &f : fileList)
    {
        std::shared_ptr<ManagementModule> mm = std::shared_ptr<ManagementModule>(new ManagementModule(clientName, f, maxPayloadSizeBytes));
        if (mm->IsValid())
        {
            std::string supportedComponents = "[";
            for (auto &c : mm->GetSupportedComponents())
            {
                supportedComponents += "\"" + c + "\", ";
                if (modMap.end() != modMap.find(c))
                {
                    // Already has an existing MM with the same component name
                    // Use the module with the latest version
                    auto &cur = modMap[c];
                    if (cur.module->GetVersion() < mm->GetVersion())
                    {
                        // Use newer version
                        OsConfigLogInfo(ModulesManagerLog::Get(), "Component %s found in module %s (version %s) and %s, selecting module %s (version %s)",
                            c.c_str(), mm->GetModulePath().c_str(), mm->GetVersion().ToString().c_str(), cur.module->GetModulePath().c_str(), mm->GetModulePath().c_str(), mm->GetVersion().ToString().c_str());
                    }
                    else
                    {
                        // Use current version
                        OsConfigLogInfo(ModulesManagerLog::Get(), "Component %s found in module %s (version %s) and %s, selecting module %s (version %s)",
                            c.c_str(), mm->GetModulePath().c_str(), mm->GetVersion().ToString().c_str(), cur.module->GetModulePath().c_str(), cur.module->GetModulePath().c_str(), cur.module->GetVersion().ToString().c_str());
                        continue;
                    }
                }
                modMap[c] = {mm, std::chrono::system_clock::now(), false};
            }
            supportedComponents = supportedComponents.substr(0, supportedComponents.length() - 2) + "]";
            OsConfigLogInfo(ModulesManagerLog::Get(), "Loaded Module: %s, version: %s, location: %s, supported components: %s", mm->GetName().c_str(), mm->GetVersion().ToString().c_str(), f.c_str(), supportedComponents.c_str());
        }
    }

    return SetReportedObjects(configJson);
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
        for (auto& reported : document[g_configReported].GetArray())
        {
            if (reported.IsObject())
            {
                if (reported.HasMember(g_configComponentName) && reported.HasMember(g_configObjectName))
                {
                    std::string componentName = reported[g_configComponentName].GetString();
                    std::string objectName = reported[g_configObjectName].GetString();
                    if (modMap.end() != modMap.find(componentName))
                    {
                        modMap[componentName].module->AddReportedObject(componentName, objectName);
                        OsConfigLogInfo(ModulesManagerLog::Get(), "Found reported object %s for component %s", objectName.c_str(), componentName.c_str());
                    }
                    else
                    {
                        OsConfigLogError(ModulesManagerLog::Get(), "Found reported object %s for component %s, but no module has been loaded", objectName.c_str(), componentName.c_str());
                        status = EINVAL;
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

void ModulesManager::ScheduleUnloadModule(ModuleMetadata &moduleMetadata)
{
    std::vector<ModuleMetadata>::iterator it;
    for (it = modulesToUnload.begin(); it != modulesToUnload.end(); )
    {
        if (it->module == moduleMetadata.module)
        {
            // Module already scheduled to unload, remove it so it can be re-ordered to the end of the vector
            modulesToUnload.erase(it);
            break;
        }
        else
        {
            ++it;
        }
    }
    modulesToUnload.push_back(moduleMetadata);
}

void ModulesManager::UnloadModules()
{
    std::vector<ModuleMetadata>::iterator it;
    for (it = modulesToUnload.begin(); it != modulesToUnload.end(); )
    {
        if (it->operationInProgress)
        {
            // Do not try to unload a module with an operation in progress. Skip to next module...
            continue;
        }

        auto start = it->lastOperation;
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);

        if (diff.count() >= cleanupTimespan)
        {
            OsConfigLogInfo(ModulesManagerLog::Get(), "Unloading %s module due to inactivity", it->module->GetName().c_str());
            it->module->UnloadModule();
            it = modulesToUnload.erase(it);
        }
        else
        {
            // modulesToUnload are in order of removal
            break;
        }
    }
}

void ModulesManager::UnloadAllModules()
{
    std::map<std::string, ModuleMetadata>::iterator it;
    for (it = modMap.begin(); it != modMap.end(); ++it)
    {
        it->second.module.reset();
    }
    modMap.clear();
}

int ModulesManager::MpiSet(const char* componentName, const char* propertyName, const char* payload, const int payloadSizeBytes)
{
    ModuleMetadata* moduleMetadata = GetModuleMetadata(componentName);
    int retValue = ETIME;
    if (manMap.end() == manMap.find(GetClientName()))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Cannot find ClientSession for %s", GetClientName().c_str());
    }
    auto mpiHandle = manMap[GetClientName()].lock().get();
    ScopeGuard sg{[&]()
    {
        if (MPI_OK == retValue)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSet(%p, %s, %s, %.*s, %d) returned %d", mpiHandle, componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, retValue);
            }
            else
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSet(%p, %s, %s, -, %d) returned %d", mpiHandle, componentName, propertyName, payloadSizeBytes, retValue);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSet(%p, %s, %s, %.*s, %d) returned %d", mpiHandle, componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, retValue);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSet(%p, %s, %s, -, %d) returned %d", mpiHandle, componentName, propertyName, payloadSizeBytes, retValue);
            }
        }
    }};

    if (nullptr == moduleMetadata)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSet invalid componentName: %s", componentName);
        retValue = ENOENT;
        return retValue;
    }

    bool timeout = ManagementModule::Lifetime::KeepAlive != moduleMetadata->module->GetLifetime();
    moduleMetadata->operationInProgress = true;

    retValue = MpiSetInternal(componentName, propertyName, payload, payloadSizeBytes);

    // Schedule an unload module if theres a timeout allowed
    if (timeout)
    {
        ScheduleUnloadModule(*moduleMetadata);
    }

    return retValue;
}

int ModulesManager::MpiSetInternal(const char* componentName, const char* propertyName, const char* payload, const int payloadSizeBytes)
{
    // Dispatch call to the right module
    if (modMap.end() == modMap.find(componentName))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to find %s in module map", componentName);
        return EINVAL;
    }
    ModuleMetadata &moduleMetadata = modMap[componentName];
    moduleMetadata.lastOperation = std::chrono::system_clock::now();
    int ret = moduleMetadata.module->CallMmiSet(componentName, propertyName, (MMI_JSON_STRING)payload, payloadSizeBytes);
    moduleMetadata.operationInProgress = false;

    if (MMI_OK == ret)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(ModulesManagerLog::Get(), "MmiSet(%s, %s, %.*s, %d) to %s returned %d", componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
        }
        else
        {
            OsConfigLogInfo(ModulesManagerLog::Get(), "MmiSet(%s, %s, -, %d) to %s returned %d", componentName, propertyName, payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
        }
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MmiSet(%s, %s, %.*s, %d) to %s returned %d", componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "MmiSet(%s, %s, -, %d) to %s returned %d", componentName, propertyName, payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
        }
    }

    return ret;
}

int ModulesManager::MpiGet(const char* componentName, const char* propertyName, char** payload, int* payloadSizeBytes)
{
    ModuleMetadata* moduleMetadata = GetModuleMetadata(componentName);
    int retValue = ETIMEDOUT;
    auto mpiHandle = manMap[GetClientName()].lock().get();

    ScopeGuard sg{[&]()
    {
        if ((MMI_OK == retValue) && (nullptr != *payload) && (nullptr != payloadSizeBytes) && (0 != *payloadSizeBytes))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiGet(%p, %s, %s, %.*s, %d) returned %d", mpiHandle, componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, retValue);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiGet(%p, %s, %s, %.*s, %d) returned %d", mpiHandle, componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, retValue);
            }
        }
    }};

    if (nullptr == moduleMetadata)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGet invalid componentName: %s", componentName);
        return ENOENT;
    }

    bool timeout = ManagementModule::Lifetime::KeepAlive != moduleMetadata->module->GetLifetime();
    moduleMetadata->operationInProgress = true;

    retValue = MpiGetInternal(componentName, propertyName, payload, payloadSizeBytes);

    // Schedule an unload module if theres a timeout allowed
    if (timeout)
    {
        ScheduleUnloadModule(*moduleMetadata);
    }

    return retValue;
}

int ModulesManager::MpiGetInternal(const char* componentName, const char* propertyName, char** payload, int* payloadSizeBytes)
{
    // Dispatch call to the right module
    if (modMap.end() == modMap.find(componentName))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to find %s in module map", componentName);
        return EINVAL;
    }

    auto &moduleMetadata = modMap[componentName];
    moduleMetadata.lastOperation = std::chrono::system_clock::now();

    int ret = MMI_OK;
    ScopeGuard sg{[&]()
    {
        if ((MMI_OK == ret) && (nullptr != *payload) && (nullptr != payloadSizeBytes) && (0 != *payloadSizeBytes))
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MmiGet(%s, %s, %.*s, %d) to %s returned %d", componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiGet(%s, %s, %.*s, %d) to %s returned %d", componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, moduleMetadata.module.get()->GetName().c_str(), ret);
            }
        }
    }};

    ret = moduleMetadata.module->CallMmiGet(componentName, propertyName, payload, payloadSizeBytes);
    moduleMetadata.operationInProgress = false;

    return ret;
}

ModulesManager::ModuleMetadata* ModulesManager::GetModuleMetadata(const char* componentName)
{
    if (modMap.end() == modMap.find(componentName))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Unable to find %s in module map", componentName);
        return nullptr;
    }

    return &modMap[componentName];
}

int ModulesManager::MpiSetDesired(const char* clientName, const char* payload, int payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MPI_OK == status)
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiSetDesired(%s, %.*s, %d) returned %d", clientName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired(%s, %.*s, %d) returned %d", clientName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiSetDesired invalid clientName: %s", clientName);
        status = EINVAL;
    }
    else if (nullptr == payload)
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
            status = MpiSetDesiredInternal(document);
        }
    }

    return status;
}

int ModulesManager::MpiSetDesiredInternal(rapidjson::Document& document)
{
    int status = MPI_OK;

    for (auto& component : document.GetObject())
    {
        if (component.value.IsObject())
        {
            std::string componentName = component.name.GetString();
            if (modMap.end() != modMap.find(componentName))
            {
                ModulesManager::ModuleMetadata& moduleMetadata = modMap[componentName];
                for (auto& object : component.value.GetObject())
                {
                    int moduleStatus = MMI_OK;
                    std::string objectName = object.name.GetString();

                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    object.value.Accept(writer);

                    moduleMetadata.operationInProgress = true;
                    moduleMetadata.lastOperation = std::chrono::system_clock::now();
                    moduleStatus = moduleMetadata.module->CallMmiSet(componentName.c_str(), objectName.c_str(), (MMI_JSON_STRING)buffer.GetString(), buffer.GetSize());
                    moduleMetadata.operationInProgress = false;

                    if ((moduleStatus != MMI_OK) && IsFullLoggingEnabled())
                    {
                        OsConfigLogError(ModulesManagerLog::Get(), "MmiSet(%s, %s, %s, %d) to %s returned %d", componentName.c_str(), objectName.c_str(), buffer.GetString(), static_cast<int>(buffer.GetSize()), moduleMetadata.module.get()->GetName().c_str(), moduleStatus);
                    }
                }
            }
            else
            {
                status = EINVAL;
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "Unable to find component %s in module map", componentName.c_str());
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

int ModulesManager::MpiGetReported(const char* clientName, const unsigned int mayPayloadSizeBytes, char** payload, int* payloadSizeBytes)
{
    int status = MPI_OK;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MPI_OK == status)
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MpiGetReported(%s, %d, %p, %p) returned %d", clientName, mayPayloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported(%s, %d, %p, %p) returned %d", clientName, mayPayloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(ModulesManagerLog::Get(), "MpiGetReported invalid clientName: %s", clientName);
        status = EINVAL;
    }
    else if (nullptr == payload)
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
        status = MpiGetReportedInternal(payload, payloadSizeBytes);
    }

    return status;
}

int ModulesManager::MpiGetReportedInternal(char** payload, int* payloadSizeBytes)
{
    int status = MPI_OK;
    rapidjson::Document document;
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    document.SetObject();

    for (auto& mod : modMap)
    {
        std::string componentName = mod.first;
        ModulesManager::ModuleMetadata& moduleMetadata = mod.second;
        std::vector<std::string> objectNames = moduleMetadata.module->GetReportedObjects(componentName);
        if (!objectNames.empty())
        {
            rapidjson::Value component(rapidjson::kObjectType);
            for (auto& objectName : objectNames)
            {
                char* objectPayload = nullptr;
                int objectPayloadSizeBytes = 0;
                int mmiGetStatus = MMI_OK;

                moduleMetadata.operationInProgress = true;
                moduleMetadata.lastOperation = std::chrono::system_clock::now();
                moduleMetadata.module->CallMmiGet(componentName.c_str(), objectName.c_str(), &objectPayload, &objectPayloadSizeBytes);
                moduleMetadata.operationInProgress = false;

                if ((MMI_OK == mmiGetStatus) && (nullptr != objectPayload) && (0 < objectPayloadSizeBytes))
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
                    OsConfigLogError(ModulesManagerLog::Get(), "MmiGet(%s, %s) returned %d", componentName.c_str(), objectName.c_str(), mmiGetStatus);
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
            std::fill(*payload, *payload + buffer.GetSize() + 1, 0);
            std::memcpy(*payload, buffer.GetString(), buffer.GetSize() + 1);
            *payloadSizeBytes = buffer.GetSize() + 1;
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

void ModulesManager::SetDefaultCleanupTimespan(unsigned int timespan)
{
    cleanupTimespan = timespan;
}

void ModulesManager::SetMaxPayloadSize(unsigned int maxSize)
{
    maxPayloadSizeBytes = maxSize;
}

std::string ModulesManager::GetClientName()
{
    return clientName;
}

void ModulesManager::DoWork()
{
    UnloadModules();
}