// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MmiClient.h>

#define AZURE_OSCONFIG "Azure OSConfig"
#define MODULE_EXT ".so"

#define UUID_LENGTH 36

static const char* g_modelVersion = "ModelVersion";
static const char* g_reported = "Reported";
static const char* g_componentName = "ComponentName";
static const char* g_objectName = "ObjectName";

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
    char* component;
    char* object;
} REPORTED_OBJECT;

static SESSION* g_sessions = NULL;
static MODULE* g_modules = NULL;
static REPORTED_OBJECT* g_reportedObjects = NULL;
static int g_numReportedObjects = 0;

bool AreModulesLoaded()
{
    return NULL != g_modules;
}

void LoadModules(const char* directory, const char* configJson)
{
    MODULE* module = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    char* client = NULL;
    int version = 0;
    int reportedCount = 0;
    JSON_Value* config = NULL;
    JSON_Object* configObject = NULL;
    JSON_Array* reportedArray = NULL;
    JSON_Object* reportedObject = NULL;
    REPORTED_OBJECT* reported = NULL;

    if (NULL == (dir = opendir(directory)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to open module directory: %s", directory);
    }
    else if (NULL == (config = json_parse_file(configJson)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to parse configuration JSON (%s)", configJson);
    }
    else if (NULL == (configObject = json_value_get_object(config)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to get config object");
    }
    else if (0 == (version = json_object_get_number(configObject, g_modelVersion)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to get model version from configuration JSON (%s)", configJson);
    }
    else
    {
        // Allocate memory for client name "Azure OSConfig <version>;<osconfig version>" + null terminator
        if (NULL == (client = (char*)malloc(strlen(AZURE_OSCONFIG) + strlen(OSCONFIG_VERSION) + 5)))
        {
            OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for client name");
        }
        else
        {
            memset(client, 0, strlen(AZURE_OSCONFIG) + strlen(OSCONFIG_VERSION) + 5);
            sprintf(client, "%s %d;%s", AZURE_OSCONFIG, version, OSCONFIG_VERSION);
        }

        while ((entry = readdir(dir)))
        {
            if (NULL == entry->d_name)
            {
                continue;
            }

            if ((strcmp(entry->d_name, "") == 0) || (strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
            {
                continue;
            }

            if (NULL == strstr(entry->d_name, MODULE_EXT))
            {
                continue;
            }

            if (NULL == (path = malloc(strlen(directory) + strlen(entry->d_name) + 2)))
            {
                OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for module path");
                continue;
            }

            memset(path, 0, strlen(directory) + strlen(entry->d_name) + 2);
            sprintf(path, "%s/%s", directory, entry->d_name);

            if (NULL != (module = LoadModule(client, path)))
            {
                module->next = g_modules;
                g_modules = module;
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "Failed to load module: %s", entry->d_name);
            }

            FREE_MEMORY(path);
        }
    }

    if (config && configObject)
    {
        if (NULL != (reportedArray = json_object_get_array(configObject, g_reported)))
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
                        OsConfigLogError(GetPlatformLog(), "Array element at index %d is not an object", i);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (reported->component = (char*)json_object_get_string(reportedObject, g_componentName)))
                    {
                        OsConfigLogError(GetPlatformLog(), "Object at index %d is missing '%s'", i, g_componentName);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (reported->component = strdup(reported->component)))
                    {
                        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for component name");
                        g_numReportedObjects--;
                    }
                    else if (NULL == (reported->object = (char*)json_object_get_string(reportedObject, g_objectName)))
                    {
                        OsConfigLogError(GetPlatformLog(), "Object at index %d is missing '%s'", i, g_objectName);
                        g_numReportedObjects--;
                    }
                    else if (NULL == (reported->object = strdup(reported->object)))
                    {
                        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for object name");
                        g_numReportedObjects--;
                    }
                }
            }
        }
    }

    if (config)
    {
        json_value_free(config);
    }
}

static void FreeModules(MODULE* modules)
{
    MODULE* curr = modules;
    MODULE* next = NULL;

    while (curr)
    {
        next = curr->next;
        UnloadModule(curr);
        curr = next;
    }

    modules = NULL;
}

static void FreeSessions(SESSION* sessions)
{
    SESSION* curr = sessions;
    SESSION* next = NULL;
    MODULE_SESSION* moduleSession = NULL;

    while (curr)
    {
        next = curr->next;

        while (curr->modules)
        {
            moduleSession = curr->modules;
            curr->modules = moduleSession->next;
            FREE_MEMORY(moduleSession);
        }

        FREE_MEMORY(curr->uuid);
        FREE_MEMORY(curr->client);
        FREE_MEMORY(curr);

        curr = next;
    }

    sessions = NULL;
}

static void FreeReportedObjects(REPORTED_OBJECT* reportedObjects, int numReportedObjects)
{
    for (int i = 0; i < numReportedObjects; i++)
    {
        FREE_MEMORY(reportedObjects[i].component);
        FREE_MEMORY(reportedObjects[i].object);
    }

    FREE_MEMORY(reportedObjects);
}

void UnloadModules(void)
{
    FreeSessions(g_sessions);
    FreeModules(g_modules);
    FreeReportedObjects(g_reportedObjects, g_numReportedObjects);

    g_numReportedObjects = 0;
}

static char* GenerateUuid(void)
{
    char* uuid = NULL;
    static const char uuidTemplate[] = "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx";
    const char* hex = "0123456789ABCDEF-";

    if (NULL == (uuid = (char*)malloc(UUID_LENGTH + 1)))
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
    char* uuid = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(GetPlatformLog(), "Invalid (null) client name");
        return NULL;
    }

    if (NULL != (session = (SESSION*)malloc(sizeof(SESSION))))
    {
        if (NULL == (session->client = strdup(clientName)))
        {
            OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for client name");
            FREE_MEMORY(session);
        }
        else if (NULL == (session->uuid = GenerateUuid()))
        {
            OsConfigLogError(GetPlatformLog(), "Failed to generate UUID");
            FREE_MEMORY(session->client);
            FREE_MEMORY(session);
        }
        else
        {
            while (module)
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
                    OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for module session");
                }

                module = module->next;
            }

            session->next = g_sessions;
            g_sessions = session;
        }

    }
    else
    {
        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for session");
    }

    if (session && (NULL == (uuid = strdup(session->uuid))))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for UUID");
    }

    return (MPI_HANDLE)uuid;
}

static SESSION* FindSession(const char* uuid)
{
    SESSION* session = g_sessions;

    while (session)
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
    SESSION* prev = NULL;

    if (NULL == handle)
    {
        OsConfigLogError(GetPlatformLog(), "Invalid (null) handle");
    }
    else if (NULL == (session = FindSession(handle)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to find session for handle (%s)", (char*)handle);
    }
    else
    {
        // Remove the session from the linked list
        if (session == g_sessions)
        {
            g_sessions = session->next;
        }
        else
        {
            prev = g_sessions;
            while (prev->next != session)
            {
                prev = prev->next;
            }
            prev->next = session->next;
        }

        FREE_MEMORY(session->uuid);
        FREE_MEMORY(session->client);
        FREE_MEMORY(session);
    }
}

static bool ComponentExists(MODULE* module, const char* component)
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

static MODULE_SESSION* FindModuleSession(MODULE_SESSION* modules, const char* component)
{
    MODULE_SESSION* current = modules;

    while (current)
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

    if (!handle || !component || !object || !payload || (payloadSizeBytes <= 0))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet(%p, %s, %s, %p, %d) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "No session exists with uuid: '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        OsConfigLogError(GetPlatformLog(), "No module exists with component: %s", component);
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

    if (!handle || !component || !object || !payload || !payloadSizeBytes)
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet(%p, %s, %s, %p, %p) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "No session exists with uuid: '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        OsConfigLogError(GetPlatformLog(), "No module exists with component: %s", component);
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
        OsConfigLogError(GetPlatformLog(), "MpiSet(%p, %p, %d) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (json = (char*)malloc(payloadSizeBytes + 1)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for json");
        status = ENOMEM;
    }
    else
    {
        memcpy(json, payload, payloadSizeBytes + 1);

        if (NULL == (rootValue = json_parse_string(json)))
        {
            OsConfigLogError(GetPlatformLog(), "Failed to parse json");
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
                    OsConfigLogError(GetPlatformLog(), "No module exists with component: %s", component);
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
                            OsConfigLogError(GetPlatformLog(), "Failed to serialize json");
                            status = EINVAL;
                        }
                        else
                        {
                            status = moduleSession->module->set(moduleSession->handle, component, object, objectJson, (int)strlen(objectJson));
                            FREE_MEMORY(objectJson);
                        }
                    }
                }
            }

            json_value_free(rootValue);
        }

        FREE_MEMORY(json);
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
    int mmiPayloadSizeBytes = 0;
    int mmiStatus = MMI_OK;
    char* payloadJson = NULL;

    if ((NULL == handle) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported(%p, %p, %p) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "No session exists with uuid: %s", uuid);
        status = EINVAL;
    }
    else if (NULL == (rootValue = json_value_init_object()))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to initialize json object");
        status = ENOMEM;
    }
    else if (NULL == (rootObject = json_value_get_object(rootValue)))
    {
        OsConfigLogError(GetPlatformLog(), "Failed to get root json object from value");
        status = ENOMEM;
    }
    else
    {
        for (int i = 0; i < g_numReportedObjects; i++)
        {
            reported = &g_reportedObjects[i];

            if (NULL == (moduleSession = FindModuleSession(session->modules, reported->component)))
            {
                OsConfigLogError(GetPlatformLog(), "No module exists with component: %s", reported->component);
            }
            else
            {
                mmiStatus = moduleSession->module->get(moduleSession->handle, reported->component, reported->object, &mmiPayload, &mmiPayloadSizeBytes);

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(GetPlatformLog(), "MmiGet(%s, %s) returned %d (%.*s)", reported->component, reported->object, mmiStatus, mmiPayloadSizeBytes, mmiPayload);
                }

                if (MMI_OK != mmiStatus)
                {
                    OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s), returned %d", reported->component, reported->object, mmiStatus);
                }
                else if (NULL == (payloadJson = (char*)malloc(mmiPayloadSizeBytes + 1)))
                {
                    OsConfigLogError(GetPlatformLog(), "Failed to allocate memory for json");
                }
                else
                {
                    memcpy(payloadJson, mmiPayload, mmiPayloadSizeBytes + 1);

                    if (NULL == (objectValue = json_parse_string(payloadJson)))
                    {
                        OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s) returned an invalid payload: %s", reported->component, reported->object, payloadJson);
                    }
                    else
                    {
                        if (NULL == (componentValue = json_object_get_value(rootObject, reported->component)))
                        {
                            componentValue = json_value_init_object();
                            json_object_set_value(rootObject, reported->component, componentValue);
                        }

                        if (NULL == (componentObject = json_value_get_object(componentValue)))
                        {
                            OsConfigLogError(GetPlatformLog(), "Failed to get JSON object for component: %s", reported->component);
                        }
                        else
                        {
                            json_object_set_value(componentObject, reported->object, objectValue);
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