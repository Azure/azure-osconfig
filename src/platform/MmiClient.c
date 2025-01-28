// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MmiClient.h>

static const char* g_mmiOpenFunction = "MmiOpen";
static const char* g_mmiCloseFunction = "MmiClose";
static const char* g_mmiGetFunction = "MmiGet";
static const char* g_mmiSetFunction = "MmiSet";
static const char* g_mmiGetInfoFunction = "MmiGetInfo";
static const char* g_mmiFreeFunction = "MmiFree";

// Required module info fields
static const char* g_infoName = "Name";
static const char* g_infoDescription = "Description";
static const char* g_infoManufacturer = "Manufacturer";
static const char* g_infoVersionMajor = "VersionMajor";
static const char* g_infoVersionMinor = "VersionMinor";
static const char* g_infoVersionInfo = "VersionInfo";
static const char* g_infoComponents = "Components";
static const char* g_infoLifetime = "Lifetime";

// Optional module info fields
static const char* g_infoVersionPatch = "VersionPatch";
static const char* g_infoVersionTweak = "VersionTweak";
static const char* g_infoLicenseUri = "LicenseUri";
static const char* g_infoProjectUri = "ProjectUri";
static const char* g_infoUserAccount = "UserAccount";

static void FreeModuleInfo(MODULE_INFO* info)
{
    int i = 0;

    if (info)
    {
        FREE_MEMORY(info->name);
        FREE_MEMORY(info->description);
        FREE_MEMORY(info->manufacturer);
        FREE_MEMORY(info->versionInfo);
        FREE_MEMORY(info->licenseUri);
        FREE_MEMORY(info->projectUri);

        if (info->components)
        {
            for (i = 0; i < (int)info->componentCount; i++)
            {
                if (NULL != info->components[i])
                {
                    FREE_MEMORY(info->components[i]);
                }
            }

            FREE_MEMORY(info->components);
        }

        FREE_MEMORY(info);
    }
}

static int ParseModuleInfo(const JSON_Value* value, MODULE_INFO** moduleInfo)
{
    MODULE_INFO* info = NULL;
    JSON_Object* object = NULL;
    JSON_Array* components = NULL;
    int componentsCount = 0;
    int status = 0;
    char* component = NULL;
    int i = 0;

    if ((NULL == value) || (NULL == moduleInfo))
    {
        OsConfigLogError(GetPlatformLog(), "ParseModuleInfo(%p, %p) called with invalid arguments", value, moduleInfo);
        status = EINVAL;
    }
    else if (NULL == (object = json_value_get_object(value)))
    {
        OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: JSON value is not an object");
        status = EINVAL;
    }
    else if (NULL == (info = (MODULE_INFO*)malloc(sizeof(MODULE_INFO))))
    {
        OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for module info");
        status = ENOMEM;
    }
    else
    {
        memset(info, 0, sizeof(MODULE_INFO));

        if (NULL == (info->name = (char*)json_object_get_string(object, g_infoName)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info is missing required field '%s'", g_infoName);
            status = EINVAL;
        }
        else if (NULL == (info->name = strdup(info->name)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for module name");
            status = errno;
        }
        else if (NULL == (info->description = (char*)json_object_get_string(object, g_infoDescription)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info is missing required field '%s'", g_infoDescription);
            status = EINVAL;
        }
        else if (NULL == (info->description = strdup(info->description)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for module description");
            status = errno;
        }
        else if (NULL == (info->manufacturer = (char*)json_object_get_string(object, g_infoManufacturer)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info is missing required field '%s'", g_infoManufacturer);
            status = EINVAL;
        }
        else if (NULL == (info->manufacturer = strdup(info->manufacturer)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for module manufacturer");
            status = errno;
        }
        else if (NULL == (info->versionInfo = (char*)json_object_get_string(object, g_infoVersionInfo)))
        {
            OsConfigLogError(GetPlatformLog(), "Module info is missing required field '%s'", g_infoVersionInfo);
            status = EINVAL;
        }
        else if (NULL == (info->versionInfo = strdup(info->versionInfo)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for module version info");
            status = errno;
        }
        else if (NULL == (components = json_object_get_array(object, g_infoComponents)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info is missing required field '%s'", g_infoComponents);
            status = EINVAL;
        }
        else if (0 == (componentsCount = json_array_get_count(components)))
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info has no components");
            status = EINVAL;
        }
        else if (json_value_get_type(json_object_get_value(object, g_infoLifetime)) != JSONNumber)
        {
            OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info has invalid lifetime type");
            status = EINVAL;
        }
        else
        {
            info->licenseUri = (char*)json_object_get_string(object, g_infoLicenseUri);
            info->projectUri = (char*)json_object_get_string(object, g_infoProjectUri);
            info->userAccount = json_object_get_number(object, g_infoUserAccount);
            info->version.major = json_object_get_number(object, g_infoVersionMajor);
            info->version.minor = json_object_get_number(object, g_infoVersionMinor);
            info->version.patch = json_object_get_number(object, g_infoVersionPatch);
            info->version.tweak = json_object_get_number(object, g_infoVersionTweak);

            if (json_object_has_value_of_type(object, g_infoLifetime, JSONNumber))
            {
                info->lifetime = json_object_get_number(object, g_infoLifetime);

                if ((info->lifetime < 0) || (2 < info->lifetime))
                {
                    OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info has invalid lifetime (%d)", info->lifetime);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: module info is missing required field '%s'", g_infoLifetime);
            }

            if ((NULL != info->licenseUri) && (NULL == (info->licenseUri = strdup(info->licenseUri))))
            {
                OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for license URI");
                status = errno;
            }

            if ((NULL != info->projectUri) && (NULL == (info->projectUri = strdup(info->projectUri))))
            {
                OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for project URI");
                status = errno;
            }

            if (NULL == (info->components = (char**)malloc(componentsCount * sizeof(char*))))
            {
                OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to allocate memory for components");
                status = ENOMEM;
            }
            else
            {
                info->componentCount = componentsCount;

                for (i = 0; i < componentsCount; i++)
                {
                    if (NULL == (component = (char*)json_array_get_string(components, i)))
                    {
                        OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to get component name at index %d", i);
                        status = EINVAL;
                        break;
                    }
                    else if (NULL == (info->components[i] = strdup(component)))
                    {
                        OsConfigLogError(GetPlatformLog(), "ParseModuleInfo: failed to copy component name at index %d", i);
                        status = errno;
                        break;
                    }
                }
            }

            if (0 == status)
            {
                *moduleInfo = info;
            }
            else
            {
                FreeModuleInfo(info);
            }
        }
    }

    return status;
}

MODULE* LoadModule(const char* client, const char* path)
{
    int status = 0;
    MODULE* module = NULL;
    MODULE_INFO* info = NULL;
    JSON_Value* value = NULL;
    MMI_JSON_STRING payload = NULL;
    int payloadSize = 0;

    if ((NULL == client) || (NULL == path))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModule(%p, %p) called with invalid arguments", client, path);
        status = EINVAL;
    }
    else if (NULL == (module = (MODULE*)malloc(sizeof(MODULE))))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModule: failed to allocate memory for module");
        status = ENOMEM;
    }
    else
    {
        OsConfigLogInfo(GetPlatformLog(), "Loading module '%s'", path);

        memset(module, 0, sizeof(MODULE));

        if (NULL == (module->path = strdup(path)))
        {
            OsConfigLogError(GetPlatformLog(), "LoadModule: failed to allocate memory for module name");
            status = errno;
        }
        else if (NULL == (module->handle = dlopen(path, RTLD_NOW)))
        {
            OsConfigLogError(GetPlatformLog(), "LoadModule: failed to load module %s: ", dlerror());
            status = ENOENT;
        }
        else
        {
            if (NULL == (module->getInfo = (MMI_GETINFO)dlsym(module->handle, g_mmiGetInfoFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiGetInfoFunction, path);
                status = ENOENT;
            }

            if (NULL == (module->open = (MMI_OPEN)dlsym(module->handle, g_mmiOpenFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiOpenFunction, path);
                status = ENOENT;
            }

            if (NULL == (module->close = (MMI_CLOSE)dlsym(module->handle, g_mmiCloseFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiCloseFunction, path);
                status = ENOENT;
            }

            if (NULL == (module->get = (MMI_GET)dlsym(module->handle, g_mmiGetFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiGetFunction, path);
                status = ENOENT;
            }

            if (NULL == (module->set = (MMI_SET)dlsym(module->handle, g_mmiSetFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiSetFunction, path);
                status = ENOENT;
            }

            if (NULL == (module->free = (MMI_FREE)dlsym(module->handle, g_mmiFreeFunction)))
            {
                OsConfigLogError(GetPlatformLog(), "LoadModule: function '%s()' not implmenented by '%s'", g_mmiFreeFunction, path);
                status = ENOENT;
            }

            if (0 == status)
            {
                if (MMI_OK != (module->getInfo(client, &payload, &payloadSize)))
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModule: failed to get module info '%s'", path);
                    status = ENOENT;
                }
                else if (NULL == (value = json_parse_string(payload)))
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModule: failed to parse module info '%s'", path);
                    status = ENOENT;
                }
                else if (0 != (status = ParseModuleInfo(value, &info)))
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModule: failed to parse module info '%s'", path);
                    status = ENOENT;
                }
                else
                {
                    OsConfigLogInfo(GetPlatformLog(), "Module loaded '%s' (v%d.%d.%d)", info->name, info->version.major, info->version.minor, info->version.patch);
                    module->info = info;
                }
            }
        }
    }

    if (0 != status)
    {
        UnloadModule(module);
        module = NULL;
    }

    return module;
}

void UnloadModule(MODULE* module)
{
    if (module)
    {
        OsConfigLogInfo(GetPlatformLog(), "Unloading module (%p)", module);

        if (NULL != module->handle)
        {
            dlclose(module->handle);
            module->handle = NULL;
        }

        module->getInfo = NULL;
        module->open = NULL;
        module->close = NULL;
        module->get = NULL;
        module->set = NULL;
        module->free = NULL;

        FreeModuleInfo(module->info);

        FREE_MEMORY(module->path);
        FREE_MEMORY(module);
    }
}
