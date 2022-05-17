// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <MpiServer.h>

static const std::string g_moduleDir = "/usr/lib/osconfig";
static const std::string g_moduleExtension = ".so";

static const std::string g_configJson = "/etc/osconfig/osconfig.json";
static const char g_configReported[] = "Reported";
static const char g_configComponentName[] = "ComponentName";
static const char g_configObjectName[] = "ObjectName";

#define UUID_LENGTH 36

static ModulesManager modulesManager;
static std::map<std::string, std::shared_ptr<MpiSession>> g_sessions;

static bool g_modulesLoaded = false;

void AreModulesLoadedAndLoadIfNot()
{
    if (false == g_modulesLoaded)
    {
        g_modulesLoaded = (bool)(0 == modulesManager.LoadModules(g_moduleDir, g_configJson));
    }
}

void UnloadModules()
{
    for (auto& session : g_sessions)
    {
        session.second->Close();
    }

    g_sessions.clear();
    modulesManager.UnloadModules();
}

void MpiInitialize(void)
{
    MpiServerInitialize();
}

void MpiShutdown(void)
{
    MpiServerShutdown();
}

void MpiDoWork() {}

MPI_HANDLE MpiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    MPI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (nullptr != handle)
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiOpen(%s, %u) returned %p ('%s')", clientName, maxPayloadSizeBytes, handle, reinterpret_cast<char*>(handle));
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiOpen(%s, %u) failed", clientName, maxPayloadSizeBytes);
        }
    }};

    if (nullptr != clientName)
    {
        std::shared_ptr<MpiSession> session = std::make_shared<MpiSession>(modulesManager, clientName, maxPayloadSizeBytes);
        if ((nullptr != session) && (0 == session->Open()))
        {
            char* uuid = session->GetUuid();
            g_sessions[uuid] = session;
            handle = reinterpret_cast<MPI_HANDLE>(uuid);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiOpen(%s, %u) failed to open a new client session", clientName, maxPayloadSizeBytes);
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen(%s, %u) called without an invalid null client name", clientName, maxPayloadSizeBytes);
    }

    return handle;
}

void MpiClose(MPI_HANDLE handle)
{
    if (nullptr != handle)
    {
        std::string uuid = reinterpret_cast<const char*>(handle);

        if (g_sessions.find(uuid) != g_sessions.end())
        {
            g_sessions[uuid]->Close();
            g_sessions.erase(uuid);
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiClose(%p) called with an invalid null handle", handle);
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

    if (nullptr != handle)
    {
        std::string uuid = reinterpret_cast<const char*>(handle);

        if (g_sessions.find(uuid) != g_sessions.end())
        {
            status = g_sessions[uuid]->Set(componentName, objectName, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiSet called with an invalid handle: %p ('%s')", handle, reinterpret_cast<char*>(handle));
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet called with invalid null handle");
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

    if (nullptr != handle)
    {
        std::string uuid = reinterpret_cast<const char*>(handle);

        if (g_sessions.find(uuid) != g_sessions.end())
        {
            status = g_sessions[uuid]->Get(componentName, objectName, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiGet called with an invalid handle: %p ('%s')", handle, reinterpret_cast<char*>(handle));
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet called with invalid null handle");
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

    if (nullptr != handle)
    {
        std::string uuid = reinterpret_cast<const char*>(handle);

        if (g_sessions.find(uuid) != g_sessions.end())
        {
            status = g_sessions[uuid]->SetDesired(payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiSetDesired called with an invalid handle: %p ('%s')", handle, reinterpret_cast<char*>(handle));
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired called with invalid null handle");
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

    if (nullptr != handle)
    {
        std::string uuid = reinterpret_cast<const char*>(handle);

        if (g_sessions.find(uuid) != g_sessions.end())
        {
            status = g_sessions[uuid]->GetReported(payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiGetReported called with an invalid handle: %p ('%s')", handle, reinterpret_cast<char*>(handle));
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported called with invalid null handle");
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

    OsConfigLogInfo(GetPlatformLog(), "Loading modules from: %s", modulePath.c_str());

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
                        OsConfigLogInfo(GetPlatformLog(), "Found newer version of '%s' module (v%s), loading newer version from '%s'", info.name.c_str(), info.version.ToString().c_str(), filePath.c_str());
                        m_modules[info.name] = mm;

                        RegisterModuleComponents(info.name, info.components, true);
                    }
                    else
                    {
                        OsConfigLogInfo(GetPlatformLog(), "Newer version of '%s' module already loaded (v%s), skipping '%s'", info.name.c_str(), currentInfo.version.ToString().c_str(), filePath.c_str());
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
        OsConfigLogError(GetPlatformLog(), "Unable to open directory: %s", modulePath.c_str());
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
        OsConfigLogError(GetPlatformLog(), "Unable to open configuration file: %s", configJson.c_str());
        return ENOENT;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document document;
    if (document.ParseStream(isw).HasParseError())
    {
        OsConfigLogError(GetPlatformLog(), "Unable to parse configuration file: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.IsObject())
    {
        OsConfigLogError(GetPlatformLog(), "Root configuration JSON is not an object: %s", configJson.c_str());
        status = EINVAL;
    }
    else if (!document.HasMember(g_configReported))
    {
        OsConfigLogError(GetPlatformLog(), "No valid %s array in configuration: %s", g_configReported, configJson.c_str());
        status = EINVAL;
    }
    else if (!document[g_configReported].IsArray())
    {
        OsConfigLogError(GetPlatformLog(), "%s is not an array in configuration: %s", g_configReported, configJson.c_str());
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
                    OsConfigLogError(GetPlatformLog(), "'%s' or '%s' missing at index %d", g_configComponentName, g_configObjectName, index);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "%s array element %d is not an object: %s", g_configReported, index, configJson.c_str());
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
            OsConfigLogError(GetPlatformLog(), "Component '%s' is already registered to module '%s'", component.c_str(), m_moduleComponentName[component].c_str());
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

static char* GenerateUuid()
{
    char* uuid = NULL;
    static const char uuidTemplate[] = "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx";
    const char* hex = "0123456789ABCDEF-";

    uuid = (char*)malloc(UUID_LENGTH + 1);
    if (uuid == NULL)
    {
        return NULL;
    }

    srand(clock());

    for (int i = 0; i < UUID_LENGTH + 1; i++)
    {
        int random = rand() % 16;
        char c = ' ';

        switch (uuidTemplate[i])
        {
            case 'x':
                c = hex[random];
                break;
            case '-':
                c = '-';
                break;
            case 'M':
                c = hex[(random & 0x03) | 0x08];
                break;
            case 'N':
                c = '4';
                break;
        }
        uuid[i] = (i < UUID_LENGTH) ? c : 0x00;
    }

    return uuid;
}

MpiSession::MpiSession(ModulesManager& modulesManager, std::string clientName, unsigned int maxPayloadSizeBytes) :
    m_modulesManager(modulesManager),
    m_uuid(GenerateUuid()),
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}

MpiSession::~MpiSession()
{
    Close();
}

char* MpiSession::GetUuid()
{
    char* uuid = new (std::nothrow) char[m_uuid.size() + 1];

    if (uuid != NULL)
    {
        std::copy(m_uuid.begin(), m_uuid.end(), uuid);
        uuid[m_uuid.size()] = '\0';
    }

    return uuid;
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
            OsConfigLogError(GetPlatformLog(), "Unable to open MMI session for module '%s'", module.first.c_str());
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
            OsConfigLogError(GetPlatformLog(), "Unable to find MMI session for component '%s'", componentName.c_str());
        }
    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Unable to find module for component '%s'", componentName.c_str());
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
                OsConfigLogInfo(GetPlatformLog(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(GetPlatformLog(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetPlatformLog(), "MpiSet(%s, %s, %.*s, %d) returned %d", componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "MpiSet(%s, %s, -, %d) returned %d", componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (0 >= payloadSizeBytes)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid payloadSizeBytes: %d", payloadSizeBytes);
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
            OsConfigLogError(GetPlatformLog(), "MpiSet componentName %s not found", componentName);
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
                OsConfigLogInfo(GetPlatformLog(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetPlatformLog(), "MpiGet(%s, %s, %.*s, %d) returned %d", componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == componentName)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid componentName: %s", componentName);
        status = EINVAL;
    }
    else if (nullptr == objectName)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid objectName: %s", objectName);
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet invalid payloadSizeBytes");
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
            OsConfigLogError(GetPlatformLog(), "MpiSet componentName %s not found", componentName);
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
                OsConfigLogInfo(GetPlatformLog(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "MpiSetDesired(%.*s, %d) returned %d", payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired invalid payload: %s", payload);
        status = EINVAL;
    }
    else if (0 == payloadSizeBytes)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired invalid payloadSizeBytes: %d", payloadSizeBytes);
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
                OsConfigLogError(GetPlatformLog(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
            }

            status = EINVAL;
        }
        else if (!document.IsObject())
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetPlatformLog(), "MpiSetDesired invalid payload: %.*s", payloadSizeBytes, payload);
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
                        OsConfigLogError(GetPlatformLog(), "MmiSet(%s, %s, %s, %d) to %s returned %d", componentName.c_str(), objectName.c_str(), buffer.GetString(), static_cast<int>(buffer.GetSize()), module->GetInfo().name.c_str(), moduleStatus);
                    }
                }
            }
            else
            {
                status = EINVAL;
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(GetPlatformLog(), "Unable to find module for component %s", componentName.c_str());
                }
            }
        }
        else
        {
            status = EINVAL;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(GetPlatformLog(), "Component value is not an object");
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
                OsConfigLogInfo(GetPlatformLog(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "MpiGetReported(%p, %p) returned %d", payload, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == payload)
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported invalid payload: %p", payload);
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported invalid payloadSizeBytes: %p", payloadSizeBytes);
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
                        OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s) returned invalid payload: %s", componentName.c_str(), objectName.c_str(), objectPayloadString.c_str());
                    }
                }
                else if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s) returned %d", componentName.c_str(), objectName.c_str(), moduleStatus);
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
            OsConfigLogError(GetPlatformLog(), "MpiGetReported unable to allocate %d bytes", *payloadSizeBytes);
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
        OsConfigLogError(GetPlatformLog(), "Could not allocate payload: %s", e.what());
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