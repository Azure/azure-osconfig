// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Platform.h>
#include <Module.h>
#include <Log.h>

#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <parson.h>

#define MODULE_EXT ".so"

#define AZURE_OSCONFIG "Azure OSConfig"
#define CONFIG_JSON "/etc/osconfig/osconfig.json"

#define UUID_LENGTH 36

typedef struct MODULE_SESSION
{
    MODULE* module;
    MMI_HANDLE handle;

    struct MODULE_SESSION* next;
} MODULE_SESSION;

typedef struct SESSION
{
    char* uuid;
    char* client;
    MODULE_SESSION* modules;

    struct SESSION* next;
} SESSION;

static SESSION* g_sessions = NULL;
static MODULE* g_modules = NULL;

char* GetClientName(void)
{
    char* client = NULL;
    int version = 0;
    JSON_Value* config = NULL;
    JSON_Object* configObject = NULL;

    if (NULL == (config = json_parse_file(CONFIG_JSON)))
    {
        LOG_ERROR("Failed to parse %s\n", CONFIG_JSON);
    }
    else if (NULL == (configObject = json_value_get_object(config)))
    {
        LOG_ERROR("Failed to get config object\n");
    }
    else if (0 == (version = json_object_get_number(configObject, "ModelVersion")))
    {
        LOG_ERROR("Failed to get model version\n");
    }
    else
    {
        client = (char*)calloc(strlen(AZURE_OSCONFIG) + strlen(OSCONFIG_VERSION) + 5, sizeof(char));
        if (NULL == client)
        {
            LOG_ERROR("Failed to allocate memory for client name\n");
        }
        else
        {
            sprintf(client, "%s %d;%s", AZURE_OSCONFIG, version, OSCONFIG_VERSION);
        }
    }

    if (config != NULL)
    {
        json_value_free(config);
    }

    return client;
}

void LoadModules(const char* directory)
{
    MODULE* module = NULL;
    char* path = NULL;
    char* client = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    if (g_modules != NULL)
    {
        return;
    }

    if (NULL == (dir = opendir(directory)))
    {
        LOG_ERROR("Failed to open module directory: %s", directory);
    }
    else if (NULL == (client = GetClientName()))
    {
        LOG_ERROR("Failed to get client name");
    }
    else
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            if (NULL == strstr(entry->d_name, MODULE_EXT))
            {
                continue;
            }

            if (NULL == (path = calloc(strlen(directory) + strlen(entry->d_name) + 2, sizeof(char))))
            {
                LOG_ERROR("Failed to allocate memory for module path");
                continue;
            }

            sprintf(path, "%s/%s", directory, entry->d_name);

            if (NULL != (module = LoadModule(client, path)))
            {
                module->next = g_modules;
                g_modules = module;
            }
            else
            {
                LOG_ERROR("Failed to load module: %s", entry->d_name);
            }

            free(path);
        }
    }
}

void UnloadModules(void)
{
    MODULE* module = g_modules;
    MODULE* next = NULL;

    while (module != NULL)
    {
        next = module->next;
        UnloadModule(module);
        module = next;
    }
}

static char* GenerateUuid(void)
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

MPI_HANDLE MpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    SESSION* session = NULL;
    MODULE* module = g_modules;
    MODULE_SESSION* moduleSession = NULL;

    if (NULL == clientName)
    {
        LOG_ERROR("Invalid (null) client name");
        return NULL;
    }

    if (NULL != (session = (SESSION*)malloc(sizeof(SESSION))))
    {
        session->client = strdup(clientName);
        session->uuid = GenerateUuid();

        while (module != NULL)
        {
            if (NULL != (moduleSession = (MODULE_SESSION*)malloc(sizeof(MODULE_SESSION))))
            {
                moduleSession->module = module;
                moduleSession->handle = module->open(clientName, maxPayloadSizeBytes);
                moduleSession->next = session->modules;
                session->modules = moduleSession;
            }
            else
            {
                LOG_ERROR("Failed to allocate memory for module session");
            }

            module = module->next;
        }

        session->next = g_sessions;
        g_sessions = session;
    }
    else
    {
        LOG_ERROR("Failed to allocate memory for session");
    }

    return (session) ? (MPI_HANDLE)session->uuid : NULL;
}

SESSION* FindSession(const char* uuid)
{
    SESSION* session = g_sessions;

    while (session != NULL)
    {
        if (0 == strcmp(session->uuid, uuid))
        {
            break;
        }
        session = session->next;
    }

    return session;
}

void MpiClose(MPI_HANDLE handle)
{
    SESSION* session = NULL;
    MODULE_SESSION* moduleSession = NULL;
    MODULE_SESSION* next = NULL;

    if (NULL == handle)
    {
        LOG_ERROR("Invalid (null) handle");
    }
    else if (NULL == (session = FindSession((const char*)handle)))
    {
        LOG_ERROR("Failed to find session");
    }
    else if (session->modules != NULL)
    {
        moduleSession = session->modules;
        next = NULL;

        while (moduleSession != NULL)
        {
            next = moduleSession->next;

            moduleSession->module->close(moduleSession->handle);
            moduleSession->handle = NULL;
            moduleSession->module = NULL;

            FREE_MEMORY(moduleSession);

            moduleSession = next;
        }
    }
}

bool ComponentExists(MODULE* module, const char* component)
{
    bool exists = false;

    for (int i = 0; i < (int)module->info->componentCount; i++)
    {
        if (0 == strcmp(module->info->components[i], component))
        {
            exists = true;
            break;
        }
    }

    return exists;
}

MODULE_SESSION* FindModuleSession(MODULE_SESSION* session, const char* component)
{
    while (session != NULL)
    {
        if (ComponentExists(session->module, component))
        {
            break;
        }
        session = session->next;
    }

    return session;
}

int MpiSet(MPI_HANDLE handle, const char* component, const char* object, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MPI_OK;
    SESSION* session = NULL;
    MODULE_SESSION* moduleSession = NULL;
    char* uuid = (char*)handle;

    if ((NULL == handle) || (NULL == component) || (NULL == object) || (NULL == payload) || (payloadSizeBytes <= 0))
    {
        LOG_ERROR("MpiSet(%p, %s, %s, %p, %d) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        LOG_ERROR("No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        LOG_ERROR("No module exists with component: %s", component);
        status = EINVAL;
    }
    else
    {
        status = moduleSession->module->set(moduleSession->handle, component, object, payload, payloadSizeBytes);
    }

    return status;
}

int MpiGet(MPI_HANDLE handle, const char* component, const char* object, MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MPI_OK;
    SESSION* session = NULL;
    MODULE_SESSION* moduleSession = NULL;
    char* uuid = (char*)handle;

    if ((NULL == handle) || (NULL == component) || (NULL == object) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        LOG_ERROR("MpiGet(%p, %s, %s, %p, %p) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        LOG_ERROR("No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        LOG_ERROR("No module exists with component: %s", component);
        status = EINVAL;
    }
    else
    {
        status = moduleSession->module->get(moduleSession->handle, component, object, payload, payloadSizeBytes);
    }

    return status;
}

int MpiSetDesired(MPI_HANDLE handle, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MPI_OK;
    // TODO: here and elsewhere - UUID type
    const char* uuid = (const char*)handle;
    char* json = NULL;
    const char* component = NULL;
    const char* object = NULL;
    char* objectJson = NULL;
    int componentCount = 0;
    int objectCount = 0;
    SESSION* session = NULL;
    MODULE_SESSION* moduleSession = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Object* componentObject = NULL;
    JSON_Value* objectValue = NULL;

    if ((NULL == handle) || (NULL == payload) || (payloadSizeBytes <= 0))
    {
        LOG_ERROR("MpiSet(%p, %p, %d) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        LOG_ERROR("No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (json = (char*)malloc(payloadSizeBytes + 1)))
    {
        LOG_ERROR("Failed to allocate memory for json");
        status = ENOMEM;
    }
    else
    {
        memcpy(json, payload, payloadSizeBytes);
        json[payloadSizeBytes] = '\0';

        if (NULL == (rootValue = json_parse_string(json)))
        {
            LOG_ERROR("Failed to parse json");
            status = EINVAL;
        }
        else
        {
            // Iterate over the "keys" in the root object
            rootObject = json_value_get_object(rootValue);
            componentCount = (int)json_object_get_count(rootObject);

            for (int i = 0; i < componentCount; i++)
            {
                component = json_object_get_name(rootObject, i);
                componentObject = json_object_get_object(rootObject, component);
                moduleSession = FindModuleSession(session->modules, component);

                if (NULL == moduleSession)
                {
                    LOG_ERROR("No module exists with component: %s", component);
                    status = EINVAL;
                }
                else
                {
                    // Iterate over the "keys" in the component object
                    objectCount = (int)json_object_get_count(componentObject);

                    for (int j = 0; j < objectCount; j++)
                    {
                        object = json_object_get_name(componentObject, j);
                        objectValue = json_object_get_value(componentObject, object);
                        objectJson = json_serialize_to_string(objectValue);

                        if (NULL == objectJson)
                        {
                            LOG_ERROR("Failed to serialize json");
                            status = EINVAL;
                        }
                        else
                        {
                            status = moduleSession->module->set(moduleSession->handle, component, object, objectJson, (int)strlen(objectJson));
                            free(objectJson);
                        }
                    }
                }
            }

            json_value_free(rootValue);
        }

        free(json);
    }

    return status;
}

int MpiGetReported(MPI_HANDLE handle, MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MPI_OK;
    const char* uuid = (const char*)handle;
    SESSION* session = NULL;
    MODULE_SESSION* moduleSession = NULL;
    JSON_Value* rootValue = NULL;

    if ((NULL == handle) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        LOG_ERROR("MpiGetReported(%p, %p, %p) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        LOG_ERROR("No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (rootValue = json_value_init_object()))
    {
        LOG_ERROR("Failed to initialize json object");
        status = ENOMEM;
    }
    else
    {
        // TODO:
        // Parse the osconfig.json file and iterate over the objects in the "Reported" array
        // For each component-object pair, call the module's get function and add the result to the json object

        UNUSED(session);
        UNUSED(moduleSession);

        *payload = json_serialize_to_string(rootValue);
        *payloadSizeBytes = (int)strlen(*payload);
        json_value_free(rootValue);
    }


    return status;
}