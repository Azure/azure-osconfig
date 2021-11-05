// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <CommonUtils.h>
#include <condition_variable>
#include <cstring>
#include <dirent.h>
#include <future>
#include <Logging.h>
#include <ModulesManager.h>
#include <Mpi.h>
#include <mutex>
#include <ScopeGuard.h>
#include <thread>
#include <tuple>
#include <vector>

constexpr unsigned int DefaultModuleCleanup = 60 * 30; // 30 minutes
const std::string ModuleDir = "/usr/lib/osconfig";
const std::string ModuleExtension = ".so";

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
        modulesManager->SetMaxPayloadSize(maxPayloadSizeBytes);
        modulesManager->LoadModules();
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

ModulesManager::ModulesManager(std::string client) : cleanupTimespan(DefaultModuleCleanup), clientName(client), maxPayloadSizeBytes(0)
{
}

ModulesManager::~ModulesManager()
{
    UnloadAllModules();
}

int ModulesManager::LoadModules()
{
    return LoadModules(ModuleDir);
}

int ModulesManager::LoadModules(std::string modulePath)
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
            if (filename.length() > ModuleExtension.length() && (0 == filename.compare(filename.length() - ModuleExtension.length(), ModuleExtension.length(), ModuleExtension.c_str())))
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

    return 0;
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
    int ret = moduleMetadata.module->MmiSet(componentName, propertyName, (MMI_JSON_STRING)payload, payloadSizeBytes);
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

    ret = moduleMetadata.module->MmiGet(componentName, propertyName, payload, payloadSizeBytes);
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
    UNUSED(clientName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MPI_OK;
}

int ModulesManager::MpiGetReported(const char* clientName, const unsigned int mayPayloadSizeBytes, char** payload, int* payloadSizeBytes)
{
    UNUSED(clientName);
    UNUSED(mayPayloadSizeBytes);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MPI_OK;
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