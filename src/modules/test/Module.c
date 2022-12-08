// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Module.h"

#define MMI_OPEN_FUNCTION "MmiOpen"
#define MMI_CLOSE_FUNCTION "MmiClose"
#define MMI_GET_FUNCTION "MmiGet"
#define MMI_SET_FUNCTION "MmiSet"
#define MMI_GETINFO_FUNCTION "MmiGetInfo"
#define MMI_FREE_FUNCTION "MmiFree"

// Required module info fields
#define INFO_NAME "Name"
#define INFO_DESCRIPTION "Description"
#define INFO_MANUFACTURER "Manufacturer"
#define INFO_VESRION_MAJOR "VersionMajor"
#define INFO_VERSION_MINOR "VersionMinor"
#define INFO_VERSION_INFO "VersionInfo"
#define INFO_COMPONENTS "Components"
#define INFO_LIFETIME "Lifetime"

// Optional module info fields
#define INFO_VERSION_PATCH "VersionPatch"
#define INFO_VERSION_TWEAK "VersionTweak"
#define INFO_LICENSE_URI "LicenseUri"
#define INFO_PROJECT_URI "ProjectUri"
#define INFO_USER_ACCOUNT_URI "UserAccount"

#define DEFAULT_MAX_PAYLOAD_SIZE 0

void FreeModuleInfo(MODULE_INFO* info)
{
    FREE_MEMORY(info->name);
    FREE_MEMORY(info->description);
    FREE_MEMORY(info->manufacturer);
    FREE_MEMORY(info->versionInfo);
    FREE_MEMORY(info->licenseUri);
    FREE_MEMORY(info->projectUri);

    if (NULL != info->components)
    {
        for (unsigned int i = 0; i < info->componentCount; i++)
        {
            if (NULL != info->components[i])
            {
                free(info->components[i]);
            }
        }

        free(info->components);
    }

    free(info);

}

void FreeModule(MANAGEMENT_MODULE* module)
{
    if (NULL != module)
    {
        FREE_MEMORY(module->name);

        if (NULL != module->info)
        {
            FreeModuleInfo(module->info);
        }

        free(module);
    }
}

int ParseModuleInfo(const JSON_Value* value, MODULE_INFO** moduleInfo)
{
    MODULE_INFO* info = NULL;
    JSON_Object* object = NULL;
    JSON_Array* components = NULL;
    int componentsCount = 0;
    int status = 0;

    const char* name = NULL;
    const char* description = NULL;
    const char* component = NULL;
    const char* versionInfo = NULL;
    const char* manufacturer = NULL;
    const char* licenseUri = NULL;
    const char* projectUri = NULL;
    int lifetime = 0;

    if (value == NULL)
    {
        LOG_ERROR("Invalid (null) JSON value");
        status = EINVAL;
    }
    else if (moduleInfo == NULL)
    {
        LOG_ERROR("Invalid (null) module info");
        status = EINVAL;
    }
    else if (NULL == (object = json_value_get_object(value)))
    {
        LOG_ERROR("JSON value is not an object");
        status = EINVAL;
    }
    else if (NULL == (info = (MODULE_INFO*)calloc(1, sizeof(MODULE_INFO))))
    {
        LOG_ERROR("Failed to allocate memory for module info");
        status = ENOMEM;
    }
    else
    {
        if (NULL == (name = json_object_get_string(object, INFO_NAME)))
        {
            LOG_ERROR("Module info is missing required field '%s'", INFO_NAME);
            status = EINVAL;
        }
        else if (NULL == (info->name = strdup(name)))
        {
            LOG_ERROR("Failed to allocate memory for module name");
            status = ENOMEM;
        }
        else if (NULL == (description = json_object_get_string(object, INFO_DESCRIPTION)))
        {
            LOG_ERROR("Module info is missing required field '%s'", INFO_DESCRIPTION);
            status = EINVAL;
        }
        else if (NULL == (info->description = strdup(description)))
        {
            LOG_ERROR("Failed to allocate memory for module description");
            status = ENOMEM;
        }
        else if (NULL == (manufacturer = json_object_get_string(object, INFO_MANUFACTURER)))
        {
            LOG_ERROR("Module info is missing required field '%s'", INFO_MANUFACTURER);
            status = EINVAL;
        }
        else if (NULL == (info->manufacturer = strdup(manufacturer)))
        {
            LOG_ERROR("Failed to allocate memory for module manufacturer");
            status = ENOMEM;
        }
        else if (NULL == (versionInfo = json_object_get_string(object, INFO_VERSION_INFO)))
        {
            LOG_ERROR("Module info is missing required field '%s'", INFO_VERSION_INFO);
            status = EINVAL;
        }
        else if (NULL == (info->versionInfo = strdup(versionInfo)))
        {
            LOG_ERROR("Failed to allocate memory for module version info");
            status = ENOMEM;
        }
        else if (NULL == (components = json_object_get_array(object, INFO_COMPONENTS)))
        {
            LOG_ERROR("Module info is missing required field '%s'", INFO_COMPONENTS);
            status = EINVAL;
        }
        else if (0 == (componentsCount = json_array_get_count(components)))
        {
            LOG_ERROR("Module info has no components");
            status = EINVAL;
        }
        else if (json_value_get_type(json_object_get_value(object, INFO_LIFETIME)) != JSONNumber)
        {
            LOG_ERROR("Module info has invalid lifetime type");
            status = EINVAL;
        }
        else
        {
            lifetime = json_object_get_number(object, INFO_LIFETIME);
            licenseUri = json_object_get_string(object, INFO_LICENSE_URI);
            projectUri = json_object_get_string(object, INFO_PROJECT_URI);
            info->userAccount = json_object_get_number(object, INFO_USER_ACCOUNT_URI);
            info->version.major = json_object_get_number(object, INFO_VESRION_MAJOR);
            info->version.minor = json_object_get_number(object, INFO_VERSION_MINOR);
            info->version.patch = json_object_get_number(object, INFO_VERSION_PATCH);
            info->version.tweak = json_object_get_number(object, INFO_VERSION_TWEAK);

            if ((lifetime < 0) || (2 < lifetime))
            {
                LOG_ERROR("Module info has invalid lifetime: %d", lifetime);
                status = EINVAL;
            }
            else
            {
                info->lifetime = lifetime;
            }

            if ((NULL != licenseUri) && (NULL == (info->licenseUri = strdup(licenseUri))))
            {
                LOG_ERROR("Failed to allocate memory for module license URI");
                status = ENOMEM;
            }

            if ((NULL != projectUri) && (NULL == (info->projectUri = strdup(projectUri))))
            {
                LOG_ERROR("Failed to allocate memory for module project URI");
                status = ENOMEM;
            }

            if (NULL == (info->components = (char**)calloc(componentsCount, sizeof(char*))))
            {
                LOG_ERROR("Failed to allocate memory for module components");
                status = ENOMEM;
            }
            else
            {
                info->componentCount = componentsCount;

                for (int i = 0; i < componentsCount; i++)
                {
                    if (NULL == (component = (char*)json_array_get_string(components, i)))
                    {
                        LOG_ERROR("Failed to get component name at index: %d", i);
                        status = EINVAL;
                        break;
                    }
                    else if (NULL == (info->components[i] = strdup(component)))
                    {
                        LOG_ERROR("Failed to copy component name at index: %d", i);
                        status = ENOMEM;
                        break;
                    }
                }
            }

            if (0 == status)
            {
                *moduleInfo = info;
            }
            else {
                FreeModuleInfo(info);
                info = NULL;
            }

        }
    }

    return status;
}

int LoadModule(const char* client, const char* bin, const char* name, MANAGEMENT_MODULE* module)
{
    int status = 0;
    char* path = NULL;
    MODULE_INFO* info = NULL;
    size_t pathLength = 0;
    JSON_Value* value = NULL;
    MMI_JSON_STRING payload = NULL;
    int payloadSize = 0;

    if (NULL == client)
    {
        LOG_ERROR("Invalid (null) client");
        status = EINVAL;
    }
    else if (NULL == bin)
    {
        LOG_ERROR("Invalid (null) bin");
        status = EINVAL;
    }
    else if (NULL == name)
    {
        LOG_ERROR("Invalid (null) name");
        status = EINVAL;
    }
    else if (0 == (pathLength = strlen(bin) + strlen(name) + 5))
    {
        LOG_ERROR("Failed to calculate path length");
        status = EINVAL;
    }
    else if (NULL == (module->name = strdup(name)))
    {
        LOG_ERROR("Failed to allocate memory for module name");
        status = ENOMEM;
    }
    else if (NULL == (path = (char*)calloc(pathLength, sizeof(char))))
    {
        LOG_ERROR("Failed to allocate memory for module path");
        status = ENOMEM;
    }
    else
    {
        snprintf(path, pathLength, "%s/%s.so", bin, name);

        LOG_INFO("Loading module %s", path);

        if (NULL == (module->handle = dlopen(path, RTLD_NOW)))
        {
            LOG_ERROR("Failed to load module %s: %s", name, dlerror());
            status = ENOENT;
        }
        else
        {
            if (NULL == (module->getinfo = (MMI_GETINFO)dlsym(module->handle, MMI_GETINFO_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_GETINFO_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (NULL == (module->open = (MMI_OPEN)dlsym(module->handle, MMI_OPEN_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_OPEN_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (NULL == (module->close = (MMI_CLOSE)dlsym(module->handle, MMI_CLOSE_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_CLOSE_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (NULL == (module->get = (MMI_GET)dlsym(module->handle, MMI_GET_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_GET_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (NULL == (module->set = (MMI_SET)dlsym(module->handle, MMI_SET_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_SET_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (NULL == (module->free = (MMI_FREE)dlsym(module->handle, MMI_FREE_FUNCTION)))
            {
                LOG_ERROR("Function '%s()' is missing from MMI for %s: %s.so", MMI_FREE_FUNCTION, name, dlerror());
                status = ENOENT;
            }

            if (0 == status)
            {
                if (MMI_OK != (module->getinfo(client, &payload, &payloadSize)))
                {
                    LOG_ERROR("Failed to get module info for %s", name);
                    status = ENOENT;
                }
                else if (NULL == (value = json_parse_string(payload)))
                {
                    LOG_ERROR("Failed to parse module info for %s", name);
                    status = ENOENT;
                }
                else if (0 != (status = ParseModuleInfo(value, &info)))
                {
                    LOG_ERROR("Failed to parse module info for %s", name);
                    status = ENOENT;
                }
                else if (NULL == (module->session = module->open(client, DEFAULT_MAX_PAYLOAD_SIZE)))
                {
                    LOG_ERROR("Failed to open module %s", name);
                    status = ENOENT;
                }
                else
                {
                    module->info = info;

                    LOG_INFO("Loaded module: '%s' (v%d.%d.%d)", info->name, info->version.major, info->version.minor, info->version.patch);
                }
            }
        }
    }

    return status;
}

void UnloadModule(MANAGEMENT_MODULE* module)
{
    if (NULL != module)
    {
        LOG_INFO("Closing module session: %s", module->name);
        module->close(module->session);
        module->session = NULL;

        module->getinfo = NULL;
        module->open = NULL;
        module->close = NULL;
        module->get = NULL;
        module->set = NULL;
        module->free = NULL;


        if (NULL != module->handle)
        {
            LOG_INFO("Unloading module: %s", module->name);
            dlclose(module->handle);
            module->handle = NULL;
        }

        FreeModule(module);
    }
}