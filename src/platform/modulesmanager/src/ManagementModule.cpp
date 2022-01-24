// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <dlfcn.h>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <vector>
#include <unordered_set>

#include <Logging.h>
#include <CommonUtils.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <ScopeGuard.h>

const std::string MmiFuncMmiGetInfo = "MmiGetInfo";
const std::string MmiFuncMmiOpen = "MmiOpen";
const std::string MmiFuncMmiClose = "MmiClose";
const std::string MmiFuncMmiSet = "MmiSet";
const std::string MmiFuncMmiGet = "MmiGet";
const std::string MmiFuncMmiFree = "MmiFree";

const std::string GETMMIINFO_NAME = "Name";
const std::string GETMMIINFO_DESCRIPTION = "Description";
const std::string GETMMIINFO_MANUFACTURER = "Manufacturer";
const std::string GETMMIINFO_VERSIONMAJOR = "VersionMajor";
const std::string GETMMIINFO_VERSIONMINOR = "VersionMinor";
const std::string GETMMIINFO_VERSIONPATCH = "VersionPatch";
const std::string GETMMIINFO_VERSIOTWEAK = "VersionTweak";
const std::string GETMMIINFO_VERSIONINFO = "VersionInfo";
const std::string GETMMIINFO_COMPONENTS = "Components";
const std::string GETMMIINFO_LIFETIME = "Lifetime";
const std::string GETMMIINFO_LICENSEURI = "LicenseUri";
const std::string GETMMIINFO_PROJECTURI = "ProjectUri";
const std::string GETMMIINFO_USERACCOUNT = "UserAccount";

typedef void (*mmi_t)();

ManagementModule::ManagementModule(const std::string clientName, const std::string path, const unsigned int maxPayloadSize) :
    handle(nullptr),
    mmiHandle(nullptr),
    isValid(true),
    clientName(clientName),
    modulePath(path),
    maxPayloadSizeBytes(maxPayloadSize)
{
    info.lifetime = Lifetime::Undefined;
    info.userAccount= 0;
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!ManagementModule::IsExportingMmi(path))
    {
        OsConfigLogError(ModulesManagerLog::Get(), "%s does not export the required Managament Module Interface (MMI)", path.c_str());
        isValid = false;
        return;
    }

    // Get the Management Module info and validate schema
    Mmi_GetInfo mmiGetInfo = reinterpret_cast<Mmi_GetInfo>(dlsym(handle, MmiFuncMmiGetInfo.c_str()));
    Mmi_Free mmiFree = reinterpret_cast<Mmi_Free>(dlsym(handle, MmiFuncMmiFree.c_str()));
    MMI_JSON_STRING retJson = nullptr;
    int retJsonSize = 0;
    int retMmiGetInfo = MMI_OK;
    ScopeGuard sg{[&]()
    {
        if ((MMI_OK == retMmiGetInfo) && isValid)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MmiGetInfo(%s, %.*s, %d) to %s returned %d", clientName.c_str(), retJsonSize, retJson, retJsonSize, path.c_str(), retMmiGetInfo);
            }
            else
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MmiGetInfo(%s, -, %d) to %s returned %d", clientName.c_str(), retJsonSize, path.c_str(), retMmiGetInfo);
            }
        }
        else if (MMI_OK != retMmiGetInfo)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiGetInfo(%s, %.*s, %d) to %s returned %d", clientName.c_str(), retJsonSize, retJson, retJsonSize, path.c_str(), retMmiGetInfo);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiGetInfo(%s, -, %d) to %s returned %d", clientName.c_str(), retJsonSize, path.c_str(), retMmiGetInfo);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiGetInfo(%s, %.*s, %d) to %s returned invalid JSON payload", clientName.c_str(), retJsonSize, retJson, retJsonSize, path.c_str());
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiGetInfo(%s, -, %d) to %s returned invalid JSON payload", clientName.c_str(), retJsonSize, path.c_str());
            }
        }
    }};

    retMmiGetInfo = mmiGetInfo(clientName.c_str(), &retJson, &retJsonSize);
    if (MMI_OK != retMmiGetInfo)
    {
        isValid = false;
        return;
    }

    std::string retJsonStr(retJson, retJsonSize);
    mmiFree(retJson);
    rapidjson::Document json;
    if (json.Parse(retJsonStr.c_str()).HasParseError())
    {
        OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo JSON payload could not be parsed", path.c_str());
        dlclose(handle);
        isValid = false;
        return;
    }

    if (json.HasMember(GETMMIINFO_NAME.c_str()) && json[GETMMIINFO_NAME.c_str()].IsString())
    {
        info.name = json[GETMMIINFO_NAME.c_str()].GetString();
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_NAME.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_NAME.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not string", path.c_str(), GETMMIINFO_NAME.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_DESCRIPTION.c_str()) && json[GETMMIINFO_DESCRIPTION.c_str()].IsString())
    {
        info.description = json[GETMMIINFO_DESCRIPTION.c_str()].GetString();
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_DESCRIPTION.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_DESCRIPTION.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not string", path.c_str(), GETMMIINFO_DESCRIPTION.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_MANUFACTURER.c_str()) && json[GETMMIINFO_MANUFACTURER.c_str()].IsString())
    {
        info.manufacturer = json[GETMMIINFO_MANUFACTURER.c_str()].GetString();
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_MANUFACTURER.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_MANUFACTURER.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not string", path.c_str(), GETMMIINFO_MANUFACTURER.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_VERSIONMAJOR.c_str()) && json[GETMMIINFO_VERSIONMAJOR.c_str()].IsInt())
    {
        int major = json[GETMMIINFO_VERSIONMAJOR.c_str()].GetInt();
        if (major < 0)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo returned %s with invalid value %d, assuming 0", path.c_str(), GETMMIINFO_VERSIONMAJOR.c_str(), major);
            major = 0;
        }
        info.version.Major = static_cast<unsigned int>(major);
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_VERSIONMAJOR.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_VERSIONMAJOR.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not integer", path.c_str(), GETMMIINFO_VERSIONMAJOR.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_VERSIONMINOR.c_str()) && json[GETMMIINFO_VERSIONMINOR.c_str()].IsInt())
    {
        int minor = json[GETMMIINFO_VERSIONMINOR.c_str()].GetInt();
        if (minor < 0)
        {
            OsConfigLogError(ModulesManagerLog::Get(),  "Module %s MmiGetInfo returned %s with invalid value %d, assuming 0", path.c_str(), GETMMIINFO_VERSIONMINOR.c_str(), minor);
            minor = 0;
        }
        info.version.Minor = static_cast<unsigned int>(minor);
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_VERSIONMINOR.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_VERSIONMINOR.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not integer", path.c_str(), GETMMIINFO_VERSIONMINOR.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_VERSIONPATCH.c_str()) && json[GETMMIINFO_VERSIONPATCH.c_str()].IsInt())
    {
        int patch = json[GETMMIINFO_VERSIONPATCH.c_str()].GetInt();
        if (patch < 0)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo returned %s with invalid value %d, assuming 0", path.c_str(), GETMMIINFO_VERSIONPATCH.c_str(), patch);
            patch = 0;
        }
        info.version.Patch = static_cast<unsigned int>(patch);
    }

    if (json.HasMember(GETMMIINFO_VERSIOTWEAK.c_str()) && json[GETMMIINFO_VERSIOTWEAK.c_str()].IsInt())
    {
        int tweak = json[GETMMIINFO_VERSIOTWEAK.c_str()].GetInt();
        if (tweak < 0)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo returned %s with invalid value %d, assuming 0", path.c_str(), GETMMIINFO_VERSIOTWEAK.c_str(), tweak);
            tweak = 0;
        }
        info.version.Tweak = static_cast<unsigned int>(tweak);
    }

    if (json.HasMember(GETMMIINFO_VERSIONINFO.c_str()) && json[GETMMIINFO_VERSIONINFO.c_str()].IsString())
    {
        info.versionInfo = json[GETMMIINFO_VERSIONINFO.c_str()].GetString();
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_VERSIONINFO.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_VERSIONINFO.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not string", path.c_str(), GETMMIINFO_VERSIONINFO.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_COMPONENTS.c_str()) && json[GETMMIINFO_COMPONENTS.c_str()].IsArray())
    {
        const rapidjson::Value &val = json[GETMMIINFO_COMPONENTS.c_str()];
        std::unordered_set<std::string> uniqueComponents;
        for (auto &v : val.GetArray())
        {
            if (uniqueComponents.find(v.GetString()) != uniqueComponents.end())
            {
                OsConfigLogError(ModulesManagerLog::Get(), "Module %s contains multiple components with the same name %s", path.c_str(), v.GetString());
                isValid = false;
            }
            info.components.push_back(v.GetString());
            uniqueComponents.insert(v.GetString());
        }

        if (info.components.empty())
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s contains no component", path.c_str());
            isValid = false;
        }
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_COMPONENTS.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_COMPONENTS.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not array", path.c_str(), GETMMIINFO_COMPONENTS.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_LIFETIME.c_str()) && json[GETMMIINFO_LIFETIME.c_str()].IsInt())
    {
        info.lifetime = (Lifetime)json[GETMMIINFO_LIFETIME.c_str()].GetInt();
        if (info.lifetime < 0 || info.lifetime > 2)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo returned %s with invalid value %d", path.c_str(), GETMMIINFO_LIFETIME.c_str(), info.lifetime);
            isValid = false;
        }
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_LIFETIME.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_LIFETIME.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not integer", path.c_str(), GETMMIINFO_LIFETIME.c_str());
        }
        isValid = false;
    }

    if (json.HasMember(GETMMIINFO_LICENSEURI.c_str()) && json[GETMMIINFO_LICENSEURI.c_str()].IsString())
    {
        info.licenseUri = json[GETMMIINFO_LICENSEURI.c_str()].GetString();
    }

    if (json.HasMember(GETMMIINFO_PROJECTURI.c_str()) && json[GETMMIINFO_PROJECTURI.c_str()].IsString())
    {
        info.projectUri = json[GETMMIINFO_PROJECTURI.c_str()].GetString();
    }

    if (json.HasMember(GETMMIINFO_USERACCOUNT.c_str()) && json[GETMMIINFO_USERACCOUNT.c_str()].IsUint())
    {
        info.userAccount = json[GETMMIINFO_USERACCOUNT.c_str()].GetUint();
    }
    else
    {
        if (!json.HasMember(GETMMIINFO_USERACCOUNT.c_str()))
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo payload missing required field %s", path.c_str(), GETMMIINFO_USERACCOUNT.c_str());
        }
        else
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Module %s MmiGetInfo required JSON field %s type not unsigned integer", path.c_str(), GETMMIINFO_USERACCOUNT.c_str());
        }
        isValid = false;
    }
}

ManagementModule::~ManagementModule()
{
    UnloadModule();
}

ManagementModule::Lifetime ManagementModule::GetLifetime() const
{
    return info.lifetime;
}

void ManagementModule::UnloadModule()
{
    if (nullptr != mmiHandle)
    {
        mmiClose(mmiHandle);
        OsConfigLogInfo(ModulesManagerLog::Get(), "MmiClose(%p) to %s", mmiHandle, modulePath.c_str());
        mmiHandle = nullptr;
    }
    if (nullptr != handle)
    {
        dlclose(handle);
        handle = nullptr;
    }
}

void ManagementModule::LoadModule()
{
    if (nullptr == handle)
    {
        handle = dlopen(modulePath.c_str(), RTLD_LAZY);

        if (nullptr == handle)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Failed to load %s", modulePath.c_str());
            return;
        }

        mmiOpen = reinterpret_cast<Mmi_Open>(dlsym(handle, MmiFuncMmiOpen.c_str()));
        mmiGetInfo = reinterpret_cast<Mmi_GetInfo>(dlsym(handle, MmiFuncMmiGetInfo.c_str()));
        mmiClose = reinterpret_cast<Mmi_Close>(dlsym(handle, MmiFuncMmiClose.c_str()));
        mmiSet = reinterpret_cast<Mmi_Set>(dlsym(handle, MmiFuncMmiSet.c_str()));
        mmiGet = reinterpret_cast<Mmi_Get>(dlsym(handle, MmiFuncMmiGet.c_str()));
        mmiFree = reinterpret_cast<Mmi_Free>(dlsym(handle, MmiFuncMmiFree.c_str()));

        if ((nullptr == mmiOpen) || (nullptr == mmiGetInfo) || (nullptr == mmiClose) || (nullptr == mmiSet) || (nullptr == mmiGet) || (nullptr == mmiFree))
        {
            dlclose(handle);
            isValid = false;
            handle = nullptr;
            return;
        }

        ScopeGuard sg{[&]()
        {
            if (mmiHandle)
            {
                OsConfigLogInfo(ModulesManagerLog::Get(), "MmiOpen(%s, %d) to %s returned %p", clientName.c_str(), maxPayloadSizeBytes, modulePath.c_str(), mmiHandle);
            }
            else
            {
                OsConfigLogError(ModulesManagerLog::Get(), "MmiOpen(%s, %d) to %s failed", clientName.c_str(), maxPayloadSizeBytes, modulePath.c_str());
            }
        }};

        mmiHandle = mmiOpen(clientName.c_str(), maxPayloadSizeBytes);
        isValid = true;
    }
}

bool ManagementModule::IsValid() const
{
    return isValid;
}

bool ManagementModule::IsLoaded() const
{
    return mmiHandle ? true : false;
}

const std::vector<std::string> ManagementModule::GetSupportedComponents() const
{
    return info.components;
}

const std::vector<std::string> ManagementModule::GetReportedObjects(const std::string& componentName) const
{
    std::vector<std::string> objects;
    if (reportedObjects.find(componentName) != reportedObjects.end())
    {
        objects = reportedObjects.at(componentName);
    }
    return objects;
}

void ManagementModule::AddReportedObject(const std::string& component, const std::string& object)
{
    if (reportedObjects.find(component) == reportedObjects.end())
    {
        reportedObjects[component] = std::vector<std::string>();
    }

    if (std::find(reportedObjects[component].begin(), reportedObjects[component].end(), object) == reportedObjects[component].end())
    {
        reportedObjects[component].push_back(object);
    }
}

bool ManagementModule::IsExportingMmi(const std::string path)
{
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (nullptr == handle)
    {
        return false;
    }

    // Make sure all function exports are present
    const std::vector<std::string> mmi_funcs = {MmiFuncMmiOpen, MmiFuncMmiClose, MmiFuncMmiSet, MmiFuncMmiGet, MmiFuncMmiGetInfo, MmiFuncMmiFree};
    for (auto &f : mmi_funcs)
    {
        mmi_t funcPtr = (mmi_t)dlsym(handle, f.c_str());
        if (nullptr == funcPtr)
        {
            OsConfigLogError(ModulesManagerLog::Get(), "Unable to call %s on %s", f.c_str(), path.c_str());
            dlclose(handle);
            return false;
        }
    }
    dlclose(handle);

    return true;
}

int ManagementModule::CallMmiSet(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;

    // Validate payload before calling MmiSet
    if (IsValidMimObjectPayload(payload, payloadSizeBytes, ModulesManagerLog::Get()))
    {
        LoadModule();
        status = mmiSet(mmiHandle, componentName, objectName, payload, payloadSizeBytes);
    }
    else
    {
        status = EINVAL;
    }

    return status;
}

int ManagementModule::CallMmiGet(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    LoadModule();
    status = mmiGet(mmiHandle, componentName, objectName, payload, payloadSizeBytes);

    if (MMI_OK == status)
    {
        // Validate payload from MmiGet
        status = IsValidMimObjectPayload(*payload, *payloadSizeBytes, ModulesManagerLog::Get()) ? MMI_OK : EINVAL;
    }

    return status;
}

const ManagementModule::Version ManagementModule::GetVersion() const
{
    return info.version;
}

const std::string ManagementModule::GetName() const
{
    return info.name;
}

const std::string ManagementModule::GetModulePath() const
{
    return modulePath;
}