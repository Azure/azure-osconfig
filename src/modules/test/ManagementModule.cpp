// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

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
                ADD_FAILURE() << symbol.c_str() << " not exported by module " << m_modulePath.c_str();
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
                    ADD_FAILURE() << "Failed to parse info JSON for module '" << m_modulePath.c_str() << "'";
                    status = EINVAL;
                }
                else if (0 != Info::Deserialize(document, m_info))
                {
                    status = EINVAL;
                }
            }
            else
            {
                ADD_FAILURE() << "Failed to get info for module '" << m_modulePath.c_str() << "'";
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

        OsConfigLogInfo(NULL, "Loaded '%s' module (v%s) from '%s', supported components: %s", m_info.name.c_str(), m_info.version.ToString().c_str(), m_modulePath.c_str(), ss.str().c_str());
    }
    else
    {
        ADD_FAILURE() << "Failed to load module '" << m_modulePath.c_str() << "'";
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

    if (nullptr != m_mmiSet && IsValidMimObjectPayload(payload, payloadSizeBytes, NULL))
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
        status = IsValidMimObjectPayload(*payload, *payloadSizeBytes, NULL) ? MMI_OK : EINVAL;
    }

    return status;
}

int ManagementModule::Info::Deserialize(const rapidjson::Value& object, ManagementModule::Info& info)
{
    int status = 0;

    if (!object.IsObject())
    {
        ADD_FAILURE() << "Failed to deserialize info JSON, expected object";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoName << "' is not a string";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoName << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoDescription << "' is not a string";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoDescription << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoManufacturer << "' is not a string";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoManufacturer << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoVersionMajor << "' is not an intege";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoVersionMajor << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoVersionMinor << "' is not an integer";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoVersionMinor << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoVersionInfo << "' is not a string";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoVersionInfo << "'";
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
                    ADD_FAILURE() << "Module info field '" << g_mmiGetInfoComponents << "' is not a string";
                }
            }
        }
        else
        {
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoComponents << "' is not an array";
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
                ADD_FAILURE() << "Module info field '" << g_mmiGetInfoLifetime << "' is not a valid lifetime (" << lifetime << ")";
                info.lifetime = Lifetime::Undefined;
                status = EINVAL;
            }
        }
        else
        {
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoLifetime << "' is not an integer";
            status = EINVAL;
        }
    }
    else
    {
        ADD_FAILURE() << "Module info is missing required field: '" << g_mmiGetInfoLifetime << "'";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoVersionPatch << "' is not an integer";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoVersionTweak << "' is not an integer";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoLicenseUri << "' is not a string";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoProjectUri << "' is not a string";
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
            ADD_FAILURE() << "Module info field '" << g_mmiGetInfoUserAccount << "' is not an unsigned integer";
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
                ADD_FAILURE() << "Failed to open MMI session for client '" << m_clientName.c_str() << "'";
            }
        }
        else
        {
            status = EINVAL;
            ADD_FAILURE() << "MMI session already open";
        }
    }
    else
    {
        status = EINVAL;
        ADD_FAILURE() << "MMI session not attached to a valid module";
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