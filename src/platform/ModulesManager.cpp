// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ModulesManager.h>

static const std::string g_moduleDir = "/usr/lib/osconfig";
static const std::string g_moduleExtension = ".so";

static const std::string g_configJson = "/etc/osconfig/osconfig.json";
static const char g_configReported[] = "Reported";
static const char g_configComponentName[] = "ComponentName";
static const char g_configObjectName[] = "ObjectName";

static const std::string g_mmiFuncMmiGetInfo = "MmiGetInfo";
static const std::string g_mmiFuncMmiOpen = "MmiOpen";
static const std::string g_mmiFuncMmiClose = "MmiClose";
static const std::string g_mmiFuncMmiSet = "MmiSet";
static const std::string g_mmiFuncMmiGet = "MmiGet";
static const std::string g_mmiFuncMmiFree = "MmiFree";

static const char g_mmiGetInfoName[] = "Name";
static const char g_mmiGetInfoDescription[] = "Description";
static const char g_mmiGetInfoManufacturer[] = "Manufacturer";
static const char g_mmiGetInfoVersionMajor[] = "VersionMajor";
static const char g_mmiGetInfoVersionMinor[] = "VersionMinor";
static const char g_mmiGetInfoVersionPatch[] = "VersionPatch";
static const char g_mmiGetInfoVersionTweak[] = "VersionTweak";
static const char g_mmiGetInfoVersionInfo[] = "VersionInfo";
static const char g_mmiGetInfoComponents[] = "Components";
static const char g_mmiGetInfoLifetime[] = "Lifetime";
static const char g_mmiGetInfoLicenseUri[] = "LicenseUri";
static const char g_mmiGetInfoProjectUri[] = "ProjectUri";
static const char g_mmiGetInfoUserAccount[] = "UserAccount";

typedef void (*mmi_t)();

ManagementModule::ManagementModule() : ManagementModule("") {}

ManagementModule::ManagementModule(const std::string path) :
    m_modulePath(path),
    m_handle(nullptr)
{
    m_info.lifetime = Lifetime::Undefined;
    m_info.userAccount= 0;
}

ManagementModule::~ManagementModule()
{
    Unload();
}

int ManagementModule::Load()
{
    int status = 0;

    if (nullptr != m_handle)
    {
        return status;
    }

    void* m_handle = dlopen(m_modulePath.c_str(), RTLD_LAZY);
    if (nullptr != m_handle)
    {
        const std::vector<std::string> symbols = {g_mmiFuncMmiGetInfo, g_mmiFuncMmiOpen, g_mmiFuncMmiClose, g_mmiFuncMmiSet, g_mmiFuncMmiGet, g_mmiFuncMmiFree};

        for (auto &symbol : symbols)
        {
            mmi_t funcPtr = (mmi_t)dlsym(m_handle, symbol.c_str());
            if (nullptr == funcPtr)
            {
                OsConfigLogError(GetLog(), "Function '%s()' is not exported via the MMI for module: '%s'", symbol.c_str(), m_modulePath.c_str());
                status = EINVAL;
            }
        }

        if (0 == status)
        {
            m_mmiOpen = reinterpret_cast<Mmi_Open>(dlsym(m_handle, g_mmiFuncMmiOpen.c_str()));
            m_mmiGetInfo = reinterpret_cast<Mmi_GetInfo>(dlsym(m_handle, g_mmiFuncMmiGetInfo.c_str()));
            m_mmiClose = reinterpret_cast<Mmi_Close>(dlsym(m_handle, g_mmiFuncMmiClose.c_str()));
            m_mmiSet = reinterpret_cast<Mmi_Set>(dlsym(m_handle, g_mmiFuncMmiSet.c_str()));
            m_mmiGet = reinterpret_cast<Mmi_Get>(dlsym(m_handle, g_mmiFuncMmiGet.c_str()));
            m_mmiFree = reinterpret_cast<Mmi_Free>(dlsym(m_handle, g_mmiFuncMmiFree.c_str()));

            MMI_JSON_STRING payload = nullptr;
            int payloadSizeBytes = 0;

            if (MMI_OK == CallMmiGetInfo("Azure OsConfig", &payload, &payloadSizeBytes))
            {
                rapidjson::Document document;
                if (document.Parse(payload, payloadSizeBytes).HasParseError())
                {
                    OsConfigLogError(GetLog(), "Failed to parse info JSON for module '%s'", m_modulePath.c_str());
                    status = EINVAL;
                }
                else if (0 != Info::Deserialize(document, m_info))
                {
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(GetLog(), "Failed to get info for module '%s'", m_modulePath.c_str());
                status = EINVAL;
            }
        }
    }
    else
    {
        status = EINVAL;
    }

    if (0 == status)
    {
        std::stringstream ss;
        ss << "[";
        if (m_info.components.size() != 0)
        {
            std::copy(m_info.components.begin(), m_info.components.end() - 1, std::ostream_iterator<std::string>(ss, ", "));
            ss << m_info.components.back();
        }
        ss << "]";

        OsConfigLogInfo(GetLog(), "Loaded '%s' module (v%s) from '%s', supported components: %s", m_info.name.c_str(), m_info.version.ToString().c_str(), m_modulePath.c_str(), ss.str().c_str());
    }
    else
    {
        OsConfigLogError(GetLog(), "Failed to load module '%s'", m_modulePath.c_str());
        if (nullptr != m_handle)
        {
            dlclose(m_handle);
            m_handle = nullptr;
        }
    }

    return status;
}

void ManagementModule::Unload()
{
    if (nullptr != m_handle)
    {
        dlclose(m_handle);
        m_handle = nullptr;
    }
}

ManagementModule::Info ManagementModule::GetInfo() const
{
    return m_info;
}

int ManagementModule::CallMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return (nullptr != m_mmiGetInfo) ? m_mmiGetInfo(clientName, payload, payloadSizeBytes) : EINVAL;
}

MMI_HANDLE ManagementModule::CallMmiOpen(const char* clientName, unsigned int maxPayloadSizeBytes)
{
    return (nullptr != m_mmiOpen) ? m_mmiOpen(clientName, maxPayloadSizeBytes) : nullptr;
}

void ManagementModule::CallMmiClose(MMI_HANDLE handle)
{
    if (nullptr != m_mmiClose)
    {
        m_mmiClose(handle);
    }
}

int ManagementModule::CallMmiSet(MMI_HANDLE handle, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr != m_mmiSet && IsValidMimObjectPayload(payload, payloadSizeBytes, GetLog()))
    {
        status = m_mmiSet(handle, componentName, objectName, payload, payloadSizeBytes);
    }
    else
    {
        status = EINVAL;
    }

    return status;
}

int ManagementModule::CallMmiGet(MMI_HANDLE handle, const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes)
{
    int status = MMI_OK;

    if ((nullptr != m_mmiGet) && (MMI_OK == (status = m_mmiGet(handle, componentName, objectName, payload, payloadSizeBytes))))
    {
        // Validate payload from MmiGet
        status = IsValidMimObjectPayload(*payload, *payloadSizeBytes, GetLog()) ? MMI_OK : EINVAL;
    }

    return status;
}

int ManagementModule::Info::Deserialize(const rapidjson::Value& object, ManagementModule::Info& info)
{
    int status = 0;

    if (!object.IsObject())
    {
        OsConfigLogError(GetLog(), "Failed to deserialize info JSON, expected object");
        return EINVAL;
    }

    // Required fields

    // Name
    if (object.HasMember(g_mmiGetInfoName))
    {
        if (object[g_mmiGetInfoName].IsString())
        {
            info.name = object[g_mmiGetInfoName].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoName);
        status = EINVAL;
    }

    // Description
    if (object.HasMember(g_mmiGetInfoDescription))
    {
        if (object[g_mmiGetInfoDescription].IsString())
        {
            info.description = object[g_mmiGetInfoDescription].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoDescription);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoDescription);
        status = EINVAL;
    }

    // Manufacturer
    if (object.HasMember(g_mmiGetInfoManufacturer))
    {
        if (object[g_mmiGetInfoManufacturer].IsString())
        {
            info.manufacturer = object[g_mmiGetInfoManufacturer].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoManufacturer);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoManufacturer);
        status = EINVAL;
    }

    // Version Major
    if (object.HasMember(g_mmiGetInfoVersionMajor))
    {
        if (object[g_mmiGetInfoVersionMajor].IsInt())
        {
            info.version.major = object[g_mmiGetInfoVersionMajor].GetInt();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionMajor);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionMajor);
        status = EINVAL;
    }

    // Version Minor
    if (object.HasMember(g_mmiGetInfoVersionMinor))
    {
        if (object[g_mmiGetInfoVersionMinor].IsInt())
        {
            info.version.minor = object[g_mmiGetInfoVersionMinor].GetInt();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionMinor);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionMinor);
        status = EINVAL;
    }

    // Version Info
    if (object.HasMember(g_mmiGetInfoVersionInfo))
    {
        if (object[g_mmiGetInfoVersionInfo].IsString())
        {
            info.versionInfo = object[g_mmiGetInfoVersionInfo].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoVersionInfo);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionInfo);
        status = EINVAL;
    }

    // Components
    if (object.HasMember(g_mmiGetInfoComponents))
    {
        if (object[g_mmiGetInfoComponents].IsArray())
        {
            std::unordered_set<std::string> components;
            for (auto& component : object[g_mmiGetInfoComponents].GetArray())
            {
                if (component.IsString() && (components.find(component.GetString()) == components.end()))
                {
                    info.components.push_back(component.GetString());
                    components.insert(component.GetString());
                }
                else
                {
                    OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoComponents);
                }
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an array", g_mmiGetInfoComponents);
            status = EINVAL;
        }
    }

    // Lifetime
    if (object.HasMember(g_mmiGetInfoLifetime))
    {
        if (object[g_mmiGetInfoLifetime].IsInt())
        {
            int lifetime = object[g_mmiGetInfoLifetime].GetInt();
            if (0 <= lifetime && lifetime <= 2)
            {
                info.lifetime = static_cast<Lifetime>(lifetime);
            }
            else
            {
                OsConfigLogError(GetLog(), "Module info field '%s' is not a valid lifetime (%d)", g_mmiGetInfoLifetime, lifetime);
                info.lifetime = Lifetime::Undefined;
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an integer", g_mmiGetInfoLifetime);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Module info is missing required field: '%s'", g_mmiGetInfoLifetime);
        status = EINVAL;
    }

    // Optional fields

    // Version Patch
    if (object.HasMember(g_mmiGetInfoVersionPatch))
    {
        if (object[g_mmiGetInfoVersionPatch].IsInt())
        {
            info.version.patch = object[g_mmiGetInfoVersionPatch].GetInt();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionPatch);
        }
    }

    // Version Tweak
    if (object.HasMember(g_mmiGetInfoVersionTweak))
    {
        if (object[g_mmiGetInfoVersionTweak].IsInt())
        {
            info.version.tweak = object[g_mmiGetInfoVersionTweak].GetInt();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionTweak);
        }
    }


    // License URI
    if (object.HasMember(g_mmiGetInfoLicenseUri))
    {
        if (object[g_mmiGetInfoLicenseUri].IsString())
        {
            info.licenseUri = object[g_mmiGetInfoLicenseUri].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoLicenseUri);
        }
    }

    // Project URI
    if (object.HasMember(g_mmiGetInfoProjectUri))
    {
        if (object[g_mmiGetInfoProjectUri].IsString())
        {
            info.projectUri = object[g_mmiGetInfoProjectUri].GetString();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not a string", g_mmiGetInfoProjectUri);
        }
    }

    // User Account
    if (object.HasMember(g_mmiGetInfoUserAccount))
    {
        if (object[g_mmiGetInfoUserAccount].IsUint())
        {
            info.userAccount = object[g_mmiGetInfoUserAccount].GetUint();
        }
        else
        {
            OsConfigLogError(GetLog(), "Module info field '%s' is not an unsigned integer", g_mmiGetInfoUserAccount);
        }
    }

    return status;
}

MmiSession::MmiSession(std::shared_ptr<ManagementModule> module, const std::string& clientName, unsigned int maxPayloadSizeBytes) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_module(module),
    m_mmiHandle(nullptr) {}

MmiSession::~MmiSession()
{
    Close();
}

int MmiSession::Open()
{
    int status = 0;

    if (nullptr != m_module)
    {
        if (nullptr == m_mmiHandle)
        {
            if (nullptr == (m_mmiHandle = m_module->CallMmiOpen(m_clientName.c_str(), m_maxPayloadSizeBytes)))
            {
                OsConfigLogError(GetLog(), "Failed to open MMI session for client '%s'", m_clientName.c_str());
            }
        }
        else
        {
            status = EINVAL;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetLog(), "MMI session already open");
            }
        }
    }
    else
    {
        status = EINVAL;
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(GetLog(), "MMI session not attached to a valid module");
        }
    }

    return status;
}

void MmiSession::Close()
{
    if (nullptr != m_module)
    {
        if (nullptr != m_mmiHandle)
        {
            m_module->CallMmiClose(m_mmiHandle);
            m_mmiHandle = nullptr;
        }
    }
}

int MmiSession::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return (nullptr != m_module) ? m_module->CallMmiSet(m_mmiHandle, componentName, objectName, payload, payloadSizeBytes) : EINVAL;
}

int MmiSession::Get(const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes)
{
    return (nullptr != m_module) ? m_module->CallMmiGet(m_mmiHandle, componentName, objectName, payload, payloadSizeBytes) : EINVAL;
}

ManagementModule::Info MmiSession::GetInfo()
{
    return (nullptr != m_module) ? m_module->GetInfo() : ManagementModule::Info();
}

static ModulesManager modulesManager;

// MPI

void MpiInitialize(void)
{
    modulesManager.LoadModules(g_moduleDir, g_configJson);
    MpiApiInitialize();
};

void MpiShutdown(void)
{
    MpiApiShutdown();
    modulesManager.UnloadModules();
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
            OsConfigLogInfo(GetLog(), "MpiOpen(%s, %u) returned %p", clientName, maxPayloadSizeBytes, handle);
        }
        else
        {
            OsConfigLogError(GetLog(), "MpiOpen(%s, %u) failed", clientName, maxPayloadSizeBytes);
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
            OsConfigLogError(GetLog(), "MpiOpen(%s, %u) failed to open a new client session", clientName, maxPayloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "MpiOpen(%s, %u) called without an invalid client name", clientName, maxPayloadSizeBytes);
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
        OsConfigLogError(GetLog(), "MpiClose(%p) called with an invalid session", handle);
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
        OsConfigLogError(GetLog(), "MpiSet called with invalid client session '%p'", handle);
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
        OsConfigLogError(GetLog(), "MpiGet called with invalid client session '%p'", handle);
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
            OsConfigLogError(GetLog(), "MpiSetDesired(%p, %p, %d) failed to set the desired state", handle, payload, payloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "MpiSetDesired(%p, %p, %d) called without an invalid handle", handle, payload, payloadSizeBytes);
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
            OsConfigLogError(GetLog(), "MpiGetReported(%p, %p, %p) failed to get the reported state", handle, payload, payloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "MpiGetReported(%p, %p, %p) called without an invalid handle", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }

    return status;
}

void MpiFree(MPI_JSON_STRING payload)
{
    delete[] payload;
}

void MpiDoWork() {}

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

    OsConfigLogInfo(GetLog(), "Loading modules from: %s", modulePath.c_str());

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
                        OsConfigLogInfo(GetLog(), "Found newer version of '%s' module (v%s), loading newer version from '%s'", info.name.c_str(), info.version.ToString().c_str(), filePath.c_str());
                        m_modules[info.name] = mm;

                        RegisterModuleComponents(info.name, info.components, true);
                    }
                    else
                    {
                        OsConfigLogInfo(GetLog(), "Newer version of '%s' module already loaded (v%s), skipping '%s'", info.name.c_str(), currentInfo.version.ToString().c_str(), filePath.c_str());
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
        OsConfigLogError(GetLog(), "Unable to open directory: %s", modulePath.c_str());
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
        OsConfigLogError(GetLog(), "Unable to open configuration file: %s", configJson.c_str());
        return ENOENT;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document document;
    if (document.ParseStream(isw).HasParseError())
    {
        OsConfigLogError(GetLog(), "Unable to parse configuration file: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.IsObject())
    {
        OsConfigLogError(GetLog(), "Root configuration JSON is not an object: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.HasMember(g_configReported))
    {
        OsConfigLogError(GetLog(), "No valid %s array in configuration: %s", g_configReported, configJson.c_str());
        status = EINVAL;
    }
    else if (!document[g_configReported].IsArray())
    {
        OsConfigLogError(GetLog(), "%s is not an array in configuration: %s", g_configReported, configJson.c_str());
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
                    OsConfigLogError(GetLog(), "'%s' or '%s' missing at index %d", g_configComponentName, g_configObjectName, index);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(GetLog(), "%s array element %d is not an object: %s", g_configReported, index, configJson.c_str());
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
            OsConfigLogError(GetLog(), "Component '%s' is already registered to module '%s'", component.c_str(), m_moduleComponentName[component].c_str());
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
            OsConfigLogError(GetLog(), "Unable to open MMI session for module '%s'", module.first.c_str());
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
            OsConfigLogError(GetLog(), "Unable to find MMI session for component '%s'", componentName.c_str());
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "Unable to find module for component '%s'", componentName.c_str());
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
                OsConfigLogInfo(GetLog(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(GetLog(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetLog(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetLog(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (0 >= payloadSizeBytes)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid payloadSizeBytes: %d", payloadSizeBytes);
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
            OsConfigLogError(GetLog(), "MpiSet componentName %s not found", componentName);
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
                OsConfigLogInfo(GetLog(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetLog(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(GetLog(), "MpiSet invalid payloadSizeBytes");
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
            OsConfigLogError(GetLog(), "MpiSet componentName %s not found", componentName);
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
                OsConfigLogInfo(GetLog(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetLog(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(GetLog(), "MpiSetDesired invalid payload: %s", payload);
        status = EINVAL;
    }
    else if (0 == payloadSizeBytes)
    {
        OsConfigLogError(GetLog(), "MpiSetDesired invalid payloadSizeBytes: %d", payloadSizeBytes);
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
                OsConfigLogError(GetLog(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
            }

            status = EINVAL;
        }
        else if (!document.IsObject())
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetLog(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
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
                        OsConfigLogError(GetLog(), "MmiSet(%s, %s, %s, %d) to %s returned %d", componentName.c_str(), objectName.c_str(), buffer.GetString(), static_cast<int>(buffer.GetSize()), module->GetInfo().name.c_str(), moduleStatus);
                    }
                }
            }
            else
            {
                status = EINVAL;
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(GetLog(), "Unable to find module for component %s", componentName.c_str());
                }
            }
        }
        else
        {
            status = EINVAL;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetLog(), "Component value is not an object");
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
                OsConfigLogInfo(GetLog(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetLog(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(GetLog(), "MpiGetReported invalid payload: %p", payload);
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(GetLog(), "MpiGetReported invalid payloadSizeBytes: %p", payloadSizeBytes);
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
                        OsConfigLogError(GetLog(), "MmiGet(%s, %s) returned invalid payload: %s", componentName.c_str(), objectName.c_str(), objectPayloadString.c_str());
                    }
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(GetLog(), "MmiGet(%s, %s) returned %d", componentName.c_str(), objectName.c_str(), moduleStatus);
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
            OsConfigLogError(GetLog(), "MpiGetReported unable to allocate %d bytes", *payloadSizeBytes);
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
        OsConfigLogError(GetLog(), "Could not allocate payload: %s", e.what());
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