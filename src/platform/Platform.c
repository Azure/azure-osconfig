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

#define MODEL_VERSION "ModelVersion"
#define REPORTED "Reported"
#define COMPONENT_NAME "ComponentName"
#define OBJECT_NAME "ObjectName"

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

typedef struct REPORTED_OBJECT
{
    const char* component;
    const char* object;
} REPORTED_OBJECT;

static SESSION* g_sessions = NULL;
static MODULE* g_modules = NULL;
static REPORTED_OBJECT* g_reportedObjects = NULL;
static int g_numReportedObjects = 0;

void LoadModules(const char* directory)
{
    MODULE* module = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    char* client = NULL;
    int version = 0;
    const char* component = NULL;
    const char* object = NULL;
    int reportedCount = 0;
    JSON_Value* config = NULL;
    JSON_Object* configObject = NULL;
    JSON_Array* reportedArray = NULL;
    JSON_Object* reportedObject = NULL;
    REPORTED_OBJECT* reported = NULL;

    if (g_modules != NULL)
    {
        return;
    }

    if (NULL == (dir = opendir(directory)))
    {
        LOG_ERROR("Failed to open module directory: %s", directory);
    }
    else if (NULL == (config = json_parse_file(CONFIG_JSON)))
    {
        LOG_ERROR("Failed to parse %s\n", CONFIG_JSON);
    }
    else if (NULL == (configObject = json_value_get_object(config)))
    {
        LOG_ERROR("Failed to get config object\n");
    }
    else if (0 == (version = json_object_get_number(configObject, MODEL_VERSION)))
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


    if ((NULL != config) && (NULL != configObject))
    {
        if (NULL != (reportedArray = json_object_get_array(configObject, REPORTED)))
        {
            reportedCount = (int)json_array_get_count(reportedArray);
            g_numReportedObjects = reportedCount;

            if ((reportedCount > 0) && (NULL != (g_reportedObjects = (REPORTED_OBJECT*)malloc(sizeof(REPORTED_OBJECT) * reportedCount))))
            {
                for (int i = 0; i < reportedCount; i++)
                {
                    reported = &g_reportedObjects[i];

                    if (NULL == (reportedObject = json_array_get_object(reportedArray, i)))
                    {
                        LOG_ERROR("Array element at index %d is not an object", i);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (component = json_object_get_string(reportedObject, COMPONENT_NAME)))
                    {
                        LOG_ERROR("Object at index %d is missing '%s'", i, COMPONENT_NAME);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (component = strdup(component)))
                    {
                        LOG_ERROR("Failed to allocate memory for component name");
                        g_numReportedObjects--;
                    }
                    else if (NULL == (object = json_object_get_string(reportedObject, OBJECT_NAME)))
                    {
                        LOG_ERROR("Object at index %d is missing '%s'", i, OBJECT_NAME);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (object = strdup(object)))
                    {
                        LOG_ERROR("Failed to allocate memory for object name");
                        g_numReportedObjects--;
                    }
                    else
                    {
                        reported->component = component;
                        reported->object = object;
                    }
                }
            }
        }
    }

    if (config != NULL)
    {
        json_value_free(config);
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

    FREE_MEMORY(g_reportedObjects);
    g_numReportedObjects = 0;
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

    return (session) ? (MPI_HANDLE)strdup(session->uuid) : NULL;
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

MODULE_SESSION* FindModuleSession(MODULE_SESSION* modules, const char* component)
{
    MODULE_SESSION* current = modules;

    while (current != NULL)
    {
        if (ComponentExists(current->module, component))
        {
            break;
        }
        current = current->next;
    }

    return current;
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
    JSON_Object* rootObject = NULL;
    JSON_Value* componentValue = NULL;
    JSON_Object* componentObject = NULL;
    JSON_Value* objectValue = NULL;
    REPORTED_OBJECT* reported = NULL;
    MMI_JSON_STRING mmiPayload = NULL;
    const char* componentName = NULL;
    const char* objectName = NULL;
    int mmiPayloadSizeBytes = 0;
    int mmiStatus = MMI_OK;
    char* payloadJson = NULL;

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
    else if (NULL == (rootObject = json_value_get_object(rootValue)))
    {
        LOG_ERROR("Failed to get root json object from value");
        status = ENOMEM;
    }
    else
    {
        for (int i = 0; i < g_numReportedObjects; i++)
        {
            reported = &g_reportedObjects[i];
            componentName = reported->component;
            objectName = reported->object;

            if (NULL == (moduleSession = FindModuleSession(session->modules, componentName)))
            {
                LOG_ERROR("No module exists with component: %s", componentName);
            }
            else
            {
                mmiStatus = moduleSession->module->get(moduleSession->handle, componentName, objectName, &mmiPayload, &mmiPayloadSizeBytes);

                LOG_TRACE("MmiGet(%s, %s) returned %d (%.*s)", componentName, objectName, mmiStatus, mmiPayloadSizeBytes, mmiPayload);

                if (MMI_OK != mmiStatus)
                {
                    LOG_ERROR("MmiGet(%s, %s), returned %d", componentName, objectName, mmiStatus);
                }
                else if (NULL == (payloadJson = (char*)malloc(mmiPayloadSizeBytes + 1)))
                {
                    LOG_ERROR("Failed to allocate memory for json");
                }
                else
                {
                    memcpy(payloadJson, mmiPayload, mmiPayloadSizeBytes);
                    payloadJson[mmiPayloadSizeBytes] = '\0';

                    if (NULL == (objectValue = json_parse_string(payloadJson)))
                    {
                        LOG_ERROR("MmiGet(%s, %s) returned an invalid payload: %s", componentName, objectName, payloadJson);
                    }
                    else
                    {
                        if (NULL == (componentValue = json_object_get_value(rootObject, componentName)))
                        {
                            componentValue = json_value_init_object();
                            json_object_set_value(rootObject, componentName, componentValue);
                        }

                        if (NULL == (componentObject = json_value_get_object(componentValue)))
                        {
                            LOG_ERROR("Failed to get JSON object for component: %s", componentName);
                        }
                        else
                        {
                            json_object_set_value(componentObject, objectName, objectValue);
                        }
                    }

                    FREE_MEMORY(payloadJson);
                }

                FREE_MEMORY(mmiPayload);
            }
        }

        *payload = json_serialize_to_string_pretty(rootValue);
        *payloadSizeBytes = (int)strlen(*payload);
        json_value_free(rootValue);
    }

    return status;
}