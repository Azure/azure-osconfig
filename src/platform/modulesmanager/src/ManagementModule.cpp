// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <dlfcn.h>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <unordered_set>
#include <vector>

#include <Logging.h>
#include <CommonUtils.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <ScopeGuard.h>

static const std::string g_mmiFuncMmiGetInfo = "MmiGetInfo";
static const std::string g_mmiFuncMmiOpen = "MmiOpen";
static const std::string g_mmiFuncMmiClose = "MmiClose";
static const std::string g_mmiFuncMmiSet = "MmiSet";
static const std::string g_mmiFuncMmiGet = "MmiGet";
static const std::string g_mmiFuncMmiFree = "MmiFree";

static const std::string g_mmiGetInfoName = "Name";
static const std::string g_mmiGetInfoDescription = "Description";
static const std::string g_mmiGetInfoManufacturer = "Manufacturer";
static const std::string g_mmiGetInfoVersionMajor = "VersionMajor";
static const std::string g_mmiGetInfoVersionMinor = "VersionMinor";
static const std::string g_mmiGetInfoVersionPatch = "VersionPatch";
static const std::string g_mmiGetInfoVersionTweak = "VersionTweak";
static const std::string g_mmiGetInfoVersionInfo = "VersionInfo";
static const std::string g_mmiGetInfoComponents = "Components";
static const std::string g_mmiGetInfoLifetime = "Lifetime";
static const std::string g_mmiGetInfoLicenseUri = "LicenseUri";
static const std::string g_mmiGetInfoProjectUri = "ProjectUri";
static const std::string g_mmiGetInfoUserAccount = "UserAccount";

typedef void (*mmi_t)();

ManagementModule::ManagementModule() :
    m_modulePath(""),
    m_isValid(false),
    m_handle(nullptr) {}

ManagementModule::ManagementModule(const std::string path) :
    m_modulePath(path),
    m_isValid(true),
    m_handle(nullptr)
{
    m_info.lifetime = Lifetime::Undefined;
    m_info.userAccount= 0;

    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (nullptr != handle)
    {
        const std::vector<std::string> symbols = { g_mmiFuncMmiGetInfo, g_mmiFuncMmiOpen, g_mmiFuncMmiClose, g_mmiFuncMmiSet, g_mmiFuncMmiGet, g_mmiFuncMmiFree };

        for (auto &symbol : symbols)
        {
            mmi_t funcPtr = (mmi_t)dlsym(handle, symbol.c_str());
            if (nullptr == funcPtr)
            {
                OsConfigLogError(ModulesManagerLog::Get(), "Function '%s()' is not exported via the MMI for module: '%s'", symbol.c_str(), m_modulePath.c_str());
                m_isValid = false;
            }
        }

        if (m_isValid)
        {
            m_mmiOpen = reinterpret_cast<Mmi_Open>(dlsym(handle, g_mmiFuncMmiOpen.c_str()));
            m_mmiGetInfo = reinterpret_cast<Mmi_GetInfo>(dlsym(handle, g_mmiFuncMmiGetInfo.c_str()));
            m_mmiClose = reinterpret_cast<Mmi_Close>(dlsym(handle, g_mmiFuncMmiClose.c_str()));
            m_mmiSet = reinterpret_cast<Mmi_Set>(dlsym(handle, g_mmiFuncMmiSet.c_str()));
            m_mmiGet = reinterpret_cast<Mmi_Get>(dlsym(handle, g_mmiFuncMmiGet.c_str()));
            m_mmiFree = reinterpret_cast<Mmi_Free>(dlsym(handle, g_mmiFuncMmiFree.c_str()));

            MMI_JSON_STRING payload = nullptr;
            int payloadSizeBytes = 0;

            if (MMI_OK == CallMmiGetInfo("Azure OsConfig", &payload, &payloadSizeBytes))
            {
                rapidjson::Document document;
                if (document.Parse(payload, payloadSizeBytes).HasParseError())
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "Failed to parse info JSON for module '%s'", m_modulePath.c_str());
                    m_isValid = false;
                }
                else if (0 != Info::Deserialize(document, m_info))
                {
                    m_isValid = false;
                }
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "Failed to get info for module '%s'", m_modulePath.c_str());
                m_isValid = false;
            }
        }
    }
    else
    {
        m_isValid = false;
    }

    if (m_isValid)
    {
        std::stringstream ss;
        ss << "[";
        std::copy(m_info.components.begin(), m_info.components.end(), std::ostream_iterator<std::string>(ss, ", "));
        ss << "]";

        OsConfigLogInfo(ModulesManagerLog::Get(), "Loaded '%s' module (%s) from '%s', supported components: %s", m_info.name.c_str(), m_info.version.ToString().c_str(), m_modulePath.c_str(), ss.str().c_str());
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Failed to load module '%s'", m_modulePath.c_str());
    }
}

ManagementModule::~ManagementModule()
{
    if (nullptr != m_handle)
    {
        dlclose(m_handle);
        m_handle = nullptr;
    }
}

bool ManagementModule::IsValid() const
{
    return m_isValid;
}

bool ManagementModule::IsLoaded() const
{
    return (nullptr != m_handle) || (nullptr != m_mmiGetInfo) || (nullptr != m_mmiOpen) || (nullptr != m_mmiClose) || (nullptr != m_mmiSet) || (nullptr != m_mmiGet) || (nullptr != m_mmiFree);
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

    if (nullptr != m_mmiSet && IsValidMimObjectPayload(payload, payloadSizeBytes, ModulesManagerLog::Get()))
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
        status = IsValidMimObjectPayload(*payload, *payloadSizeBytes, ModulesManagerLog::Get()) ? MMI_OK : EINVAL;
    }

    return status;
}

int ManagementModule::Info::Deserialize(const rapidjson::Value& object, ManagementModule::Info& info)
{
    int status = 0;

    // Required fields

    // Name
    if (object.HasMember(g_mmiGetInfoName.c_str()))
    {
        if (object[g_mmiGetInfoName.c_str()].IsString())
        {
            info.name = object[g_mmiGetInfoName.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoName.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoName.c_str());
        status = EINVAL;
    }

    // Description
    if (object.HasMember(g_mmiGetInfoDescription.c_str()))
    {
        if (object[g_mmiGetInfoDescription.c_str()].IsString())
        {
            info.description = object[g_mmiGetInfoDescription.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoDescription.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoDescription.c_str());
        status = EINVAL;
    }

    // Manufacturer
    if (object.HasMember(g_mmiGetInfoManufacturer.c_str()))
    {
        if (object[g_mmiGetInfoManufacturer.c_str()].IsString())
        {
            info.manufacturer = object[g_mmiGetInfoManufacturer.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoManufacturer.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoManufacturer.c_str());
        status = EINVAL;
    }

    // Version Major
    if (object.HasMember(g_mmiGetInfoVersionMajor.c_str()))
    {
        if (object[g_mmiGetInfoVersionMajor.c_str()].IsInt())
        {
            info.version.major = object[g_mmiGetInfoVersionMajor.c_str()].GetInt();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionMajor.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionMajor.c_str());
        status = EINVAL;
    }

    // Version Minor
    if (object.HasMember(g_mmiGetInfoVersionMinor.c_str()))
    {
        if (object[g_mmiGetInfoVersionMinor.c_str()].IsInt())
        {
            info.version.minor = object[g_mmiGetInfoVersionMinor.c_str()].GetInt();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionMinor.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionMinor.c_str());
        status = EINVAL;
    }

    // Version Info
    if (object.HasMember(g_mmiGetInfoVersionInfo.c_str()))
    {
        if (object[g_mmiGetInfoVersionInfo.c_str()].IsString())
        {
            info.versionInfo = object[g_mmiGetInfoVersionInfo.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoVersionInfo.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoVersionInfo.c_str());
        status = EINVAL;
    }

    // Components
    if (object.HasMember(g_mmiGetInfoComponents.c_str()))
    {
        if (object[g_mmiGetInfoComponents.c_str()].IsArray())
        {
            std::unordered_set<std::string> components;
            for (auto& component : object[g_mmiGetInfoComponents.c_str()].GetArray())
            {
                if (component.IsString() && (components.find(component.GetString()) == components.end()))
                {
                    info.components.push_back(component.GetString());
                    components.insert(component.GetString());
                }
                else
                {
                    OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoComponents.c_str());
                }
            }
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an array", g_mmiGetInfoComponents.c_str());
            status = EINVAL;
        }
    }

    // Lifetime
    if (object.HasMember(g_mmiGetInfoLifetime.c_str()))
    {
        if (object[g_mmiGetInfoLifetime.c_str()].IsInt())
        {
            int lifetime = object[g_mmiGetInfoLifetime.c_str()].GetInt();
            if (0 <= lifetime && lifetime <= 2)
            {
                info.lifetime = static_cast<Lifetime>(lifetime);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a valid lifetime (%d)", g_mmiGetInfoLifetime.c_str(), lifetime);
                info.lifetime = Lifetime::Undefined;
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an integer", g_mmiGetInfoLifetime.c_str());
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module info is missing required field: '%s'", g_mmiGetInfoLifetime.c_str());
        status = EINVAL;
    }

    // Optional fields

    // Version Patch
    if (object.HasMember(g_mmiGetInfoVersionPatch.c_str()))
    {
        if (object[g_mmiGetInfoVersionPatch.c_str()].IsInt())
        {
            info.version.patch = object[g_mmiGetInfoVersionPatch.c_str()].GetInt();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionPatch.c_str());
        }
    }

    // Version Tweak
    if (object.HasMember(g_mmiGetInfoVersionTweak.c_str()))
    {
        if (object[g_mmiGetInfoVersionTweak.c_str()].IsInt())
        {
            info.version.tweak = object[g_mmiGetInfoVersionTweak.c_str()].GetInt();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an integer", g_mmiGetInfoVersionTweak.c_str());
        }
    }


    // License URI
    if (object.HasMember(g_mmiGetInfoLicenseUri.c_str()))
    {
        if (object[g_mmiGetInfoLicenseUri.c_str()].IsString())
        {
            info.licenseUri = object[g_mmiGetInfoLicenseUri.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoLicenseUri.c_str());
        }
    }

    // Project URI
    if (object.HasMember(g_mmiGetInfoProjectUri.c_str()))
    {
        if (object[g_mmiGetInfoProjectUri.c_str()].IsString())
        {
            info.projectUri = object[g_mmiGetInfoProjectUri.c_str()].GetString();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not a string", g_mmiGetInfoProjectUri.c_str());
        }
    }

    // User Account
    if (object.HasMember(g_mmiGetInfoUserAccount.c_str()))
    {
        if (object[g_mmiGetInfoUserAccount.c_str()].IsUint())
        {
            info.userAccount = object[g_mmiGetInfoUserAccount.c_str()].GetUint();
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module info field '%s' is not an unsigned integer", g_mmiGetInfoUserAccount.c_str());
        }
    }

    return status;
}

MmiSession::MmiSession(std::shared_ptr<ManagementModule> module, const std::string& clientName, unsigned int maxPayloadSizeBytes) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes),
    m_module(module),
    m_handle(nullptr)
{
    if (m_module)
    {
        m_handle = m_module->CallMmiOpen(m_clientName.c_str(), m_maxPayloadSizeBytes);
    }
}

MmiSession::~MmiSession()
{
    if (m_module)
    {
        m_module->CallMmiClose(m_handle);
    }
}

int MmiSession::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return (nullptr != m_module) ? m_module->CallMmiSet(m_handle, componentName, objectName, payload, payloadSizeBytes) : EINVAL;
}

int MmiSession::Get(const char* componentName, const char* objectName, MMI_JSON_STRING *payload, int *payloadSizeBytes)
{
    return (nullptr != m_module) ? m_module->CallMmiGet(m_handle, componentName, objectName, payload, payloadSizeBytes) : EINVAL;
}

ManagementModule::Info MmiSession::GetInfo()
{
    return (nullptr != m_module) ? m_module->GetInfo() : ManagementModule::Info();
}