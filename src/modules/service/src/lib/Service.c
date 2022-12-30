// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>

#include "Service.h"

static const char* g_serviceModuleInfo = "{\"Name\": \"Service\","
    "\"Description\": \"Provides functionality to observe and configure services\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"Service\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_serviceModuleName = "Service module";
static const char* g_serviceComponentName = "Service";

static const char* g_objectNames[] = {"rcctl", "systemd", "sysv", "upstart", "src"};
static const int g_objectNamesCount = ARRAY_SIZE(g_objectNames);

// static const char* g_reportedOptInObjectName = "optIn";
// static const char* g_desiredOptInObjectName = "desiredOptIn";

// static const char* g_serviceConfigFileFormat = "Permission = \"%s\"\n";
// static const char* g_permissionConfigPattern = "\\bPermission\\s*=\\s*([\\\"'])([A-Za-z0-9]*)\\1";
// static const char* g_permissionConfigName = "Permission";
// static const char* g_permissionConfigMapKeys[] = {"None", "Required", "Optional"};
// static const char* g_permissionConfigMapValues[] = {"0", "1", "2"};
// static const unsigned int g_permissionConfigMapCount = ARRAY_SIZE(g_permissionConfigMapKeys);

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static const char* g_serviceConfigFile = NULL;
static const char* g_serviceLogFile = "/var/log/osconfig_service.log";
static const char* g_serviceRolledLogFile = "/var/log/osconfig_service.bak";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE ServiceGetLog()
{
    return g_log;
}

void ServiceInitialize(const char* configFile)
{
    g_serviceConfigFile = configFile;
    g_log = OpenLog(g_serviceLogFile, g_serviceRolledLogFile);
        
    OsConfigLogInfo(ServiceGetLog(), "%s initialized", g_serviceModuleName);
}

void ServiceShutdown(void)
{
    OsConfigLogInfo(ServiceGetLog(), "%s shutting down", g_serviceModuleName);

    g_serviceConfigFile = NULL;
    CloseLog(&g_log);
}

MMI_HANDLE ServiceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_serviceModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(ServiceGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_serviceModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void ServiceMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(ServiceGetLog(), "MmiClose(%p)", clientSession);
    }
    else 
    {
        OsConfigLogError(ServiceGetLog(), "MmiClose() called outside of a valid session");
    }
}

int ServiceMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(ServiceGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return status;
    }
    
    *payloadSizeBytes = (int)strlen(g_serviceModuleInfo);
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memset(*payload, 0, *payloadSizeBytes);
        memcpy(*payload, g_serviceModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(ServiceGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ServiceGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int ServiceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    const char* command = "/root/.local/bin/ansible localhost -m ansible.builtin.service_facts -o 2> /dev/null | grep -o '{.*'";

    int status = MMI_OK;
    char* result = NULL;

    JSON_Value* rootValue = NULL; 
    JSON_Object* rootObject = NULL;
    JSON_Object* servicesObject = NULL;
    JSON_Value* serviceValue = NULL;
    JSON_Object* serviceObject = NULL;
    JSON_Value* resultValue = NULL;
    JSON_Array* resultArray = NULL;

    const char* name = NULL;
    const char* source = NULL;
    const char* state = NULL;
    int count = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(ServiceGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(ServiceGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_serviceComponentName))
    {
        OsConfigLogError(ServiceGetLog(), "MmiGet called for an unsupported component name '%s'", componentName);
        status = EINVAL;
    }
    else
    {
        status = EINVAL;

        for (int i = 0; i < g_objectNamesCount; i++)
        {
            if (0 == strcmp(objectName, g_objectNames[i]))
            {
                status = MMI_OK;
                break;
            }
        }

        if (MMI_OK != status)
        {
            OsConfigLogError(ServiceGetLog(), "MmiGet called for an unsupported object name '%s'", objectName);
        }
    }

    if (MMI_OK == status)
    {
        resultValue = json_value_init_array();
        resultArray = json_value_get_array(resultValue);

        if ((0 == ExecuteCommand(NULL, command, false, false, 0, 0, &result, NULL, ServiceGetLog())))
        {
            rootValue = json_parse_string(result);

            if (json_value_get_type(rootValue) == JSONObject)
            {
                rootObject = json_value_get_object(rootValue);

                if (json_object_dothas_value_of_type(rootObject, "ansible_facts.services", JSONObject))
                {
                    servicesObject = json_object_dotget_object(rootObject, "ansible_facts.services");
                    count = json_object_get_count(servicesObject);

                    for (int i = 0; i < count; i++)
                    {
                        name = json_object_get_name(servicesObject, i);

                        if (json_object_has_value_of_type(servicesObject, name, JSONObject))
                        {
                            serviceValue = json_object_get_value(servicesObject, name);

                            if (json_value_get_type(serviceValue) == JSONObject)
                            {
                                serviceObject = json_value_get_object(serviceValue);

                                if ((json_object_has_value_of_type(serviceObject, "source", JSONString)) &&
                                    (json_object_has_value_of_type(serviceObject, "state", JSONString)))
                                {
                                    source = json_object_get_string(serviceObject, "source");
                                    state = json_object_get_string(serviceObject, "state");

                                    if ((source != NULL) && (0 == strcmp(source, objectName)) && 
                                        (state != NULL) && (0 == strcmp(state, "running")))
                                    {
                                        json_array_append_string(resultArray, name);
                                    }
                                }
                            }
                        }
                    }
                }
                else 
                {
                    if (IsFullLoggingEnabled())
                    {
                        OsConfigLogError(ServiceGetLog(), "MmiGet failed to find JSON property '%s'", "ansible_facts.services");
                    }
                    status = EINVAL;
                }
            }
            else 
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(ServiceGetLog(), "MmiGet failed to parse JSON string '%s'", result);
                }
                status = EINVAL;
            }
        }
        else 
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ServiceGetLog(), "MmiGet failed to execute command '%s'", command);
            }
            status = EINVAL;
        }

        FREE_MEMORY(result);

        result = json_serialize_to_string(resultValue);

        if (NULL != result)
        {
            *payloadSizeBytes = strlen(result);
            if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
            {
                OsConfigLogError(ServiceGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated", componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);
                *payloadSizeBytes = g_maxPayloadSizeBytes;
            }

            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
            if (NULL != *payload)
            {
                memset(*payload, 0, *payloadSizeBytes);
                memcpy(*payload, result, *payloadSizeBytes);
            }
            else 
            {
                OsConfigLogError(ServiceGetLog(), "MmiGet failed to allocate %d bytes", *payloadSizeBytes + 1);
                *payloadSizeBytes = 0;
                status = ENOMEM;
            }

            json_free_serialized_string(result);
        }
        else 
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ServiceGetLog(), "MmiGet failed to serialize JSON array");
            }
            status = EINVAL;
        }
    }

    if (NULL != rootValue)
    {
        json_value_free(rootValue);
    }

    if (NULL != resultValue)
    {
        json_value_free(resultValue);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ServiceGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int ServiceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    const char* command = "/root/.local/bin/ansible localhost -m ansible.builtin.service -a 'name=%s state=%s' -o 2> /dev/null | grep -o '{.*'";

    int status = MMI_OK;
    char* buffer = NULL;

    JSON_Value* rootValue = NULL;
    JSON_Array* rootArray = NULL;
    JSON_Value* serviceValue = NULL;
    JSON_Object* serviceObject = NULL;

    int count = 0;
    const char* name = NULL;
    const char* state = NULL;

    char* commandBuffer = NULL;
    int commandBufferSizeBytes = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (payloadSizeBytes <= 0))
    {
        OsConfigLogError(ServiceGetLog(), "MmiSet(%s, %s, %p, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (!IsValidSession(clientSession))
    {
        OsConfigLogError(ServiceGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_serviceComponentName))
    {
        OsConfigLogError(ServiceGetLog(), "MmiSet called for an unsupported component name '%s'", componentName);
        status = EINVAL;
    }
    else if (0 != strcmp(objectName, "desiredServices"))
    {
        OsConfigLogError(ServiceGetLog(), "MmiSet called for an unsupported object name '%s'", objectName);
        status = EINVAL;
    }
    else 
    {
        // Copy payload to local buffer with null terminator for json_parse_string.
        buffer = malloc(payloadSizeBytes + 1);

        if (NULL != buffer)
        {
            memset(buffer, 0, payloadSizeBytes + 1);
            memcpy(buffer, payload, payloadSizeBytes);

            rootValue = json_parse_string(buffer);
            if (json_value_get_type(rootValue) == JSONArray)
            {
                rootArray = json_value_get_array(rootValue);
                count = json_array_get_count(rootArray);

                for (int i = 0; i < count; i++)
                {
                    serviceValue = json_array_get_value(rootArray, i);

                    if (json_value_get_type(serviceValue) == JSONObject)
                    {
                        serviceObject = json_value_get_object(serviceValue);

                        name = NULL;
                        state = "started";

                        if (json_object_has_value_of_type(serviceObject, "name", JSONString))
                        {
                            name = json_object_get_string(serviceObject, "name");
                        }
                        
                        if (json_object_has_value_of_type(serviceObject, "state", JSONString))
                        {
                            state = json_object_get_string(serviceObject, "state");
                        }

                        if ((NULL != name) && (NULL != state))
                        {
                            commandBufferSizeBytes = snprintf(NULL, 0, command, name, state);
                            commandBuffer = malloc(commandBufferSizeBytes + 1);

                            if (NULL != commandBuffer)
                            {
                                memset(commandBuffer, 0, commandBufferSizeBytes + 1);
                                snprintf(commandBuffer, commandBufferSizeBytes + 1, command, name, state);

                                if ((0 == ExecuteCommand(NULL, commandBuffer, false, false, 0, 0, NULL, NULL, ServiceGetLog())))
                                {
                                    // Ignored.
                                }
                                else 
                                {
                                    if (IsFullLoggingEnabled())
                                    {
                                        OsConfigLogError(ServiceGetLog(), "MmiSet failed to execute command '%s'", command);
                                    }
                                    status = EINVAL;
                                }

                                FREE_MEMORY(commandBuffer);
                            }
                            else 
                            {
                                ServiceGetLog(ServiceGetLog(), "MmiSet: failed to allocate %d bytes", commandBufferSizeBytes + 1);
                                status = ENOMEM;
                            }
                        }
                    }
                    else 
                    {
                        // Ignore malformed object.

                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogError(ServiceGetLog(), "MmiSet failed to process mailformed JSON object");
                        }
                    }
                }
            }
            else
            {
                OsConfigLogError(ServiceGetLog(), "MmiSet failed to parse JSON '%.*s'", payloadSizeBytes, payload);
                status = EINVAL;
            }

            json_value_free(rootValue);
        }
        else 
        {
            OsConfigLogError(ServiceGetLog(), "MmiSet failed to allocate %d bytes", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }

    FREE_MEMORY(buffer);

    OsConfigLogInfo(ServiceGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
    
    return status;
}

void ServiceMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
