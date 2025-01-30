// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MmiClient.h>

#define AZURE_OSCONFIG "Azure OSConfig"
#define MODULE_EXT ".so"

static const char* g_modelVersion = "ModelVersion";
static const char* g_reportedObjectType = "Reported";
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
static REPORTED_OBJECT* g_reported = NULL;
static int g_reportedTotal = 0;

OSCONFIG_LOG_HANDLE g_platformLog = NULL;

OSCONFIG_LOG_HANDLE GetPlatformLog(void)
{
    return g_platformLog;
}

static void LoadModules(const char* directory, const char* configJson)
{
    MODULE* module = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    char* clientName = NULL;
    int version = 0;
    JSON_Value* config = NULL;
    JSON_Object* configObject = NULL;
    JSON_Array* reportedArray = NULL;
    JSON_Object* reportedObject = NULL;
    REPORTED_OBJECT* reported = NULL;
    int reportedCount = 0;
    int reportedTotal = 0;
    int i = 0;
    int loaded = 0;
    ssize_t clientNameSize = 0;
    ssize_t reportedSize = 0;
    ssize_t pathSize = 0;

    if ((NULL == directory) || (NULL == configJson))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModules(%p, %p) called with invalid arguments", directory, configJson);
    }
    else if (NULL == (config = json_parse_file(configJson)))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModules: failed to parse configuration JSON '%s'", configJson);
    }
    else if (NULL == (configObject = json_value_get_object(config)))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModules: failed to get config object");
    }
    else if (0 == (version = json_object_get_number(configObject, g_modelVersion)))
    {
        OsConfigLogError(GetPlatformLog(), "LoadModules: failed to get model version from configuration JSON '%s'", configJson);
    }
    else
    {
        OsConfigLogInfo(GetPlatformLog(), "LoadModules: loading modules from '%s'", directory);

        // "Azure OSConfig <version>;<osconfig version>" + null-terminator
        clientNameSize = strlen(AZURE_OSCONFIG) + strlen(OSCONFIG_VERSION) + 5;

        if (NULL == (clientName = (char*)malloc(clientNameSize)))
        {
            OsConfigLogError(GetPlatformLog(), "LoadModules: failed to allocate memory for client name");
        }
        else
        {
            memset(clientName, 0, clientNameSize);
            snprintf(clientName, clientNameSize, "%s %d;%s", AZURE_OSCONFIG, version, OSCONFIG_VERSION);
        }

        OsConfigLogInfo(GetPlatformLog(), "LoadModules: client name '%s'", clientName);
        errno = 0;

        if (NULL != (dir = opendir(directory)))
        {
            while (NULL != (entry = readdir(dir)))
            {
                if ((DT_REG != entry->d_type) ||
                    ((strcmp(entry->d_name, "") == 0) || (strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) ||
                    (NULL == strstr(entry->d_name, MODULE_EXT)))
                {
                    continue;
                }

                // <directory>/<module .so> + null-terminator
                pathSize = strlen(directory) + strlen(entry->d_name) + 2;

                if (NULL == (path = malloc(pathSize)))
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModules: failed to allocate memory for path");
                    continue;
                }

                memset(path, 0, pathSize);
                snprintf(path, pathSize, "%s/%s", directory, entry->d_name);

                if (NULL != (module = LoadModule(clientName, path)))
                {
                    module->next = g_modules;
                    g_modules = module;
                    loaded++;
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModules: failed to load module '%s'", entry->d_name);
                }

                FREE_MEMORY(path);
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "LoadModules: failed to open module directory '%s'", directory);
        }

        if (0 != errno)
        {
            OsConfigLogError(GetPlatformLog(), "LoadModules: failed during readdir() (%d)", errno);
        }

        if (loaded > 0)
        {
            OsConfigLogInfo(GetPlatformLog(), "Loaded %d modules from '%s'", loaded, directory);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "No modules found in '%s'", directory);
        }
    }

    if (config && configObject)
    {
        if (NULL != (reportedArray = json_object_get_array(configObject, g_reportedObjectType)))
        {
            reportedCount = (int)json_array_get_count(reportedArray);

            if (0 < reportedCount)
            {
                reportedSize = sizeof(REPORTED_OBJECT) * reportedCount;

                if (NULL != (reported = (REPORTED_OBJECT*)malloc(reportedSize)))
                {
                    memset(reported, 0, reportedSize);

                    for (i = 0; i < reportedCount; i++)
                    {
                        if (NULL == (reportedObject = json_array_get_object(reportedArray, i)))
                        {
                            OsConfigLogError(GetPlatformLog(), "LoadModules: array element at index %d is not an object", i);
                        }
                        else if (NULL == (reported[i].component = (char*)json_object_get_string(reportedObject, g_componentName)))
                        {
                            OsConfigLogError(GetPlatformLog(), "LoadModules: object at index %d is missing '%s'", i, g_componentName);
                        }
                        else if (NULL == (reported[i].component = strdup(reported[i].component)))
                        {
                            OsConfigLogError(GetPlatformLog(), "LoadModules: failed to allocate memory for component name");
                        }
                        else if (NULL == (reported[i].object = (char*)json_object_get_string(reportedObject, g_objectName)))
                        {
                            OsConfigLogError(GetPlatformLog(), "LoadModules: object at index %d is missing '%s'", i, g_objectName);
                        }
                        else if (NULL == (reported[i].object = strdup(reported[i].object)))
                        {
                            OsConfigLogError(GetPlatformLog(), "LoadModules: failed to allocate memory for object name");
                        }
                        else
                        {
                            if (IsFullLoggingEnabled())
                            {
                                OsConfigLogInfo(GetPlatformLog(), "LoadModules: found reported property (%s.%s)", reported[i].component, reported[i].object);
                            }

                            reportedTotal++;
                        }
                    }

                    OsConfigLogInfo(GetPlatformLog(), "LoadModules: found %d reported objects in '%s'", reportedTotal, configJson);

                    g_reported = reported;
                    g_reportedTotal = reportedTotal;
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "LoadModules: failed to allocate memory for reported objects");
                }
            }
        }
    }

    if (config)
    {
        json_value_free(config);
    }
}

void AreModulesLoadedAndLoadIfNot(const char* directory, const char* configJson)
{
    if (NULL == g_modules)
    {
        LoadModules(directory, configJson);
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
    int i = 0;

    for (i = 0; i < numReportedObjects; i++)
    {
        FREE_MEMORY(reportedObjects[i].component);
        FREE_MEMORY(reportedObjects[i].object);
    }

    FREE_MEMORY(reportedObjects);
}

void UnloadModules(void)
{
    FreeModules(g_modules);
    FreeSessions(g_sessions);
    FreeReportedObjects(g_reported, g_reportedTotal);

    g_reportedTotal = 0;
}

static char* GenerateUuid(void)
{
    char* uuid = NULL;
    static const char uuidTemplate[] = "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx";
    const char* hex = "0123456789ABCDEF-";
    int random = 0;
    char c = ' ';
    int i = 0;
    ssize_t size = ARRAY_SIZE(uuidTemplate);

    if (NULL == (uuid = (char*)malloc(size)))
    {
        return NULL;
    }

    memset(uuid, 0, size);
    srand(clock());

    for (i = 0; i < size - 1; i++)
    {
        random = rand() % 16;
        c = ' ';

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
        }
        uuid[i] = c;
    }

    return uuid;
}

MPI_HANDLE MpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    SESSION* session = NULL;
    MODULE* module = NULL;
    MODULE_SESSION* moduleSession = NULL;
    char* uuid = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen: invalid (null) client name");
    }
    else if (NULL == g_modules)
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen: modules are not loaded, cannot open a session");
    }
    else if (NULL == (uuid = GenerateUuid()))
    {
        OsConfigLogError(GetPlatformLog(), "MpiOpen: failed to generate UUID");
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiOpen: creating session with UUID '%s'", uuid);
        }

        if (NULL != (session = (SESSION*)malloc(sizeof(SESSION))))
        {
            memset(session, 0, sizeof(MODULE_SESSION));

            if (NULL != (session->client = strdup(clientName)))
            {
                if (NULL != (session->uuid = strdup(uuid)))
                {
                    module = g_modules;

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
                            OsConfigLogError(GetPlatformLog(), "MpiOpen: failed to allocate memory for module session");
                        }

                        module = module->next;
                    }

                    session->next = g_sessions;
                    g_sessions = session;
                }
                else
                {
                    OsConfigLogError(GetPlatformLog(), "MpiOpen: failed to allocate memory for session '%s'", uuid);
                    FREE_MEMORY(session->client);
                    FREE_MEMORY(session);
                }
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "MpiOpen: failed to allocate memory for client name");
                FREE_MEMORY(session);
            }
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiOpen: failed to allocate memory for session '%s'", uuid);
        }
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
        OsConfigLogError(GetPlatformLog(), "MpiClose: invalid (null) handle");
    }
    else if (NULL == (session = FindSession(handle)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiClose: failed to find session for handle (%s)", (char*)handle);
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiClose: closing session with UUID '%s'", session->uuid);
        }

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
    int i = 0;

    for (i = 0; i < (int)module->info->componentCount; i++)
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

    if ((NULL == handle) || (NULL == component) || (NULL == object))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet(%p, %s, %s, %p, %d) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet: no session exists with UUID '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet: no module exists with component '%s'", component);
        status = EINVAL;
    }
    else if (NULL == moduleSession->module)
    {
        OsConfigLogError(GetPlatformLog(), "MpiSet: no module is loaded for session '%s'", uuid);
        status = EINVAL;
    }
    else
    {
        if (MMI_OK == (status = moduleSession->module->set(moduleSession->handle, component, object, payload, payloadSizeBytes)))
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiSet(%p, %s, %s, %p, %d) succeeded", moduleSession->handle, component, object, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiSet(%p, %s, %s, %p, %d) failed with %d", moduleSession->handle, component, object, payload, payloadSizeBytes, status);
        }
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
        OsConfigLogError(GetPlatformLog(), "MpiGet(%p, %s, %s, %p, %p) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet: no session exists with UUID '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (moduleSession = FindModuleSession(session->modules, component)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet: no module exists with component '%s'", component);
        status = EINVAL;
    }
    else if (NULL == moduleSession->module)
    {
        OsConfigLogError(GetPlatformLog(), "MpiGet: no module is loaded for session '%s'", uuid);
        status = EINVAL;
    }
    else
    {
        status = moduleSession->module->get(moduleSession->handle, component, object, payload, payloadSizeBytes);

        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(GetPlatformLog(), "MpiGet(%p, %s, %s, %p, %p) succeeded", handle, component, object, payload, payloadSizeBytes);
            }
            else
            {
                OsConfigLogError(GetPlatformLog(), "MpiGet(%p, %s, %s, %p, %p) failed with %d", handle, component, object, payload, payloadSizeBytes, status);
            }
        }
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
    int i = 0;
    int j = 0;

    if ((NULL == handle) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired(%p, %p, %d) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired: no session exists with UUID '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (json = (char*)malloc(payloadSizeBytes + 1)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiSetDesired: failed to allocate memory for JSON");
        status = ENOMEM;
    }
    else
    {
        memcpy(json, payload, payloadSizeBytes + 1);

        if (NULL == (rootValue = json_parse_string(json)))
        {
            OsConfigLogError(GetPlatformLog(), "MpiSetDesired: failed to parse JSON");
            status = EINVAL;
        }
        else
        {
            // Iterate over the "keys" in the root object
            rootObject = json_value_get_object(rootValue);
            componentCount = (int)json_object_get_count(rootObject);

            for (i = 0; i < componentCount; i++)
            {
                component = json_object_get_name(rootObject, i);
                componentObject = json_object_get_object(rootObject, component);
                moduleSession = FindModuleSession(session->modules, component);

                if (NULL == moduleSession)
                {
                    OsConfigLogError(GetPlatformLog(), "MpiSetDesired: no module exists with component '%s'", component);
                    status = EINVAL;
                }
                else
                {
                    // Iterate over the "keys" in the component object
                    objectCount = (int)json_object_get_count(componentObject);

                    for (j = 0; j < objectCount; j++)
                    {
                        object = json_object_get_name(componentObject, j);
                        objectValue = json_object_get_value(componentObject, object);
                        objectJson = json_serialize_to_string(objectValue);

                        if (NULL == objectJson)
                        {
                            OsConfigLogError(GetPlatformLog(), "MpiSetDesired: failed to serialize JSON");
                        }
                        else if (NULL == moduleSession->module)
                        {
                            OsConfigLogError(GetPlatformLog(), "MpiSetDesired: no module is loaded for session '%s'", uuid);
                        }
                        else
                        {
                            if (MMI_OK != (status = moduleSession->module->set(moduleSession->handle, component, object, objectJson, (int)strlen(objectJson))))
                            {
                                OsConfigLogError(GetPlatformLog(), "MpiSetDesired: MmiSet(%p, %s, %s) failed with %d", moduleSession->handle, component, object, status);
                            }

                            FREE_MEMORY(objectJson);
                        }
                    }
                }
            }

            json_value_free(rootValue);
        }

        FREE_MEMORY(json);
    }

    if (IsFullLoggingEnabled())
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiSetDesired(%p, %p, %d) succeeded", handle, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiSetDesired(%p, %p, %d) failed with %d", handle, payload, payloadSizeBytes, status);
        }
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
    MMI_JSON_STRING mmiPayload = NULL;
    int mmiPayloadSizeBytes = 0;
    int mmiStatus = MMI_OK;
    char* payloadJson = NULL;
    int i = 0;

    if ((NULL == handle) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported(%p, %p, %p) called with invalid arguments", handle, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (NULL == (session = FindSession(uuid)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported: no session exists with UUID '%s'", uuid);
        status = EINVAL;
    }
    else if (NULL == (rootValue = json_value_init_object()))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported: failed to initialize json object");
        status = ENOMEM;
    }
    else if (NULL == (rootObject = json_value_get_object(rootValue)))
    {
        OsConfigLogError(GetPlatformLog(), "MpiGetReported: failed to get root json object from value");
        status = ENOMEM;
    }
    else
    {
        for (i = 0; i < g_reportedTotal; i++)
        {
            if (NULL == (moduleSession = FindModuleSession(session->modules, g_reported[i].component)))
            {
                OsConfigLogError(GetPlatformLog(), "MpiGetReported: no module exists with component '%s'", g_reported[i].component);
            }
            else if (NULL == moduleSession->module)
            {
                OsConfigLogError(GetPlatformLog(), "MpiGetReported: no module is loaded for session '%s'", uuid);
            }
            else
            {
                mmiStatus = moduleSession->module->get(moduleSession->handle, g_reported[i].component, g_reported[i].object, &mmiPayload, &mmiPayloadSizeBytes);

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(GetPlatformLog(), "MmiGet(%s, %s) returned %d (%.*s)", g_reported[i].component, g_reported[i].object, mmiStatus, mmiPayloadSizeBytes, mmiPayload);
                }

                if (MMI_OK != mmiStatus)
                {
                    OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s), returned %d", g_reported[i].component, g_reported[i].object, mmiStatus);
                }
                else if (NULL == (payloadJson = (char*)malloc(mmiPayloadSizeBytes + 1)))
                {
                    OsConfigLogError(GetPlatformLog(), "MpiGetReported: failed to allocate memory for JSON");
                }
                else
                {
                    memcpy(payloadJson, mmiPayload, mmiPayloadSizeBytes + 1);

                    if (NULL == (objectValue = json_parse_string(payloadJson)))
                    {
                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s) returned an invalid payload '%s'", g_reported[i].component, g_reported[i].object, payloadJson);
                        }
                        else
                        {
                            OsConfigLogError(GetPlatformLog(), "MmiGet(%s, %s) returned an invalid payload", g_reported[i].component, g_reported[i].object);
                        }
                    }
                    else
                    {
                        if (NULL == (componentValue = json_object_get_value(rootObject, g_reported[i].component)))
                        {
                            componentValue = json_value_init_object();
                            json_object_set_value(rootObject, g_reported[i].component, componentValue);
                        }

                        if (NULL == (componentObject = json_value_get_object(componentValue)))
                        {
                            OsConfigLogError(GetPlatformLog(), "MpiGetReported: failed to get JSON object for component '%s'", g_reported[i].component);
                        }
                        else
                        {
                            json_object_set_value(componentObject, g_reported[i].object, objectValue);
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

    if (IsFullLoggingEnabled())
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(GetPlatformLog(), "MpiGetDesired(%p, %p) succeeded", handle, payload);
        }
        else
        {
            OsConfigLogError(GetPlatformLog(), "MpiGetDesired(%p, %p) failed with %d", handle, payload, status);
        }
    }

    return status;
}
