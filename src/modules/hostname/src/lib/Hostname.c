// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>

#include "HostName.h"

static const char* g_hostnameModuleName = "HostName";
static const char* g_component = "HostName";
static const char* g_desiredName = "desiredName";
static const char* g_desiredHosts = "desiredHosts";
static const char* g_name = "name";
static const char* g_hosts = "hosts";

static const char* g_hostnameLogFile = "/var/log/osconfig_hostname.log";
static const char* g_hostnameRolledLogFile = "/var/log/osconfig_hostname.bak";

static const char* g_hostnameModuleInfo = "{\"Name\": \"HostName\","
"\"Description\": \"Provides functionality to observe and configure network hostname and hosts\","
"\"Manufacturer\": \"Microsoft\","
"\"VersionMajor\": 1,"
"\"VersionMinor\": 0,"
"\"VersionInfo\": \"Nickel\","
"\"Components\": [\"HostName\"],"
"\"Lifetime\": 2,"
"\"UserAccount\": 0}";

typedef struct Handle
{
    unsigned int maxPayloadSizeBytes;
} Handle;

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE GetHostnameLog(void)
{
    return g_log;
}

void HostNameInitialize(void)
{
    g_log = OpenLog(g_hostnameLogFile, g_hostnameRolledLogFile);

    OsConfigLogInfo(GetHostnameLog(), "%s initialized", g_hostnameModuleName);
}

void HostNameShutdown(void)
{
    OsConfigLogInfo(GetHostnameLog(), "%s shutting down", g_hostnameModuleName);

    CloseLog(&g_log);
}

MMI_HANDLE HostNameMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    Handle* handle = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(GetHostnameLog(), "MmiOpen() called with NULL clientName");
        return NULL;
    }

    handle = (Handle*)malloc(sizeof(Handle));
    if (NULL == handle)
    {
        OsConfigLogError(GetHostnameLog(), "MmiOpen() failed to allocate memory for handle");
    }
    else
    {
        handle->maxPayloadSizeBytes = maxPayloadSizeBytes;
        OsConfigLogInfo(GetHostnameLog(), "MmiOpen(%s, %u) = %p", clientName, maxPayloadSizeBytes, handle);
    }

    return (MMI_HANDLE)handle;
}

void HostNameMmiClose(MMI_HANDLE clientSession)
{
    Handle* handle = (Handle*)clientSession;

    if (NULL == handle)
    {
        OsConfigLogError(GetHostnameLog(), "MmiClose() called with NULL handle");
        return;
    }

    OsConfigLogInfo(GetHostnameLog(), "MmiClose(%p)", handle);

    free(handle);
}

int HostNameMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return EINVAL;
    }

    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_hostnameModuleInfo);

    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_hostnameModuleInfo, *payloadSizeBytes);
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetHostnameLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int GetName(JSON_Value** value)
{
    const char* command = "cat /etc/hostname";

    int status = 0;
    char* textResult = NULL;

    if ((0 == ExecuteCommand(NULL, command, false, false, 0, 0, &textResult, NULL, GetHostnameLog())))
    {
        if (textResult)
        {
            *value = json_value_init_string(textResult);
            if (NULL == *value)
            {
                OsConfigLogError(GetHostnameLog(), "GetName: failed to create JSON value for %s", textResult);
                status = ENOMEM;
            }
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "GetName: failed to get hostname");
            status = ENOENT;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "GetName: failed to execute %s", command);
        status = ENOENT;
    }

    FREE_MEMORY(textResult);

    return status;
}

int GetHosts(JSON_Value** value)
{
    const char* command = "cat /etc/hosts";

    int status = 0;
    char* textResult = NULL;

    if ((0 == ExecuteCommand(NULL, command, false, false, 0, 0, &textResult, NULL, GetHostnameLog())))
    {
        if (textResult)
        {
            // TODO: parse the textResult and create the hosts string
            UNUSED(value);
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "GetHosts: failed to get hosts");
            status = ENOENT;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "GetHosts: failed to execute %s", command);
        status = ENOENT;
    }

    FREE_MEMORY(textResult);

    return status;
}

int HostNameMmiGet(MMI_HANDLE clientSession, const char* component, const char* object, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    Handle* handle = (Handle*)clientSession;
    JSON_Value* value = NULL;
    char* json = NULL;

    if ((NULL == handle) || (NULL == component) || (NULL == object) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(GetHostnameLog(), "MmiGet(%p, %s, %s, %p, %p) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        return EINVAL;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (0 == strcmp(component, g_component))
    {
        if (0 == strcmp(object, g_name))
        {
            status = GetName(&value);
        }
        else if (0 == strcmp(object, g_hosts))
        {
            status = GetHosts(&value);
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "MmiGet called for an invalid object name (%s)", object);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "MmiGet called for an invalid component name (%s)", component);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (NULL != value))
    {
        json = json_serialize_to_string(value);

        if (NULL == json)
        {
            OsConfigLogError(GetHostnameLog(), "Failed to serialize JSON object");
            status = ENOMEM;
        }
        else if ((handle->maxPayloadSizeBytes > 0) && (handle->maxPayloadSizeBytes < strlen(json)))
        {
            OsConfigLogError(GetHostnameLog(), "Payload size exceeds maximum size");
            status = E2BIG;
        }
        else
        {
            *payloadSizeBytes = strlen(json);
            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);

            if (NULL == *payload)
            {
                OsConfigLogError(GetHostnameLog(), "Failed to allocate memory for payload");
                status = ENOMEM;
            }
            else
            {
                memcpy(*payload, json, *payloadSizeBytes);
            }
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetHostnameLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, component, object, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int SetName(const JSON_Value* value)
{
    const char* template = "hostnamectl set-hostname --static \"%s\"";

    int status = 0;
    const char* name = NULL;
    char* command = NULL;
    char* textResult = NULL;
    JSON_Object* object = json_value_get_object(value);

    if (json_object_has_value_of_type(object, g_name, JSONString))
    {
        name = json_object_get_string(object, g_name);

        if (NULL != (command = (char*)malloc(strlen(template) + strlen(name) + 1)))
        {
            sprintf(command, template, name);

            if (0 != ExecuteCommand(NULL, command, false, false, 0, 0, &textResult, NULL, GetHostnameLog()))
            {
                OsConfigLogError(GetHostnameLog(), "Failed to set the hostname: '%s'", name);
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "Failed to allocate memory for command");
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "Failed to get name from JSON object");
        status = ENOENT;
    }

    return status;
}

int SetHosts(const JSON_Value* value)
{
    const char* template = "echo \"%s\" > /etc/hosts";

    int status = 0;
    const char* hosts = NULL;
    char* command = NULL;
    char* textResult = NULL;
    JSON_Object* object = json_value_get_object(value);

    if (json_object_has_value_of_type(object, g_hosts, JSONString))
    {
        hosts = json_object_get_string(object, g_hosts);

        // TODO: parse this string and apply the hosts properly

        if (NULL != (command = (char*)malloc(strlen(template) + strlen(hosts) + 1)))
        {
            sprintf(command, template, hosts);

            if (0 != ExecuteCommand(NULL, command, false, false, 0, 0, &textResult, NULL, GetHostnameLog()))
            {
                OsConfigLogError(GetHostnameLog(), "Failed to set the hosts: '%s'", hosts);
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "Failed to allocate memory for command");
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "Failed to get hosts from JSON object");
        status = ENOENT;
    }

    return status;
}

int HostNameMmiSet(MMI_HANDLE clientSession, const char* component, const char* object, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    Handle* handle = (Handle*)clientSession;
    JSON_Value* value = NULL;
    char* json = NULL;

    if ((NULL == handle) || (NULL == component) || (NULL == object) || (NULL == payload))
    {
        OsConfigLogError(GetHostnameLog(), "MmiGet(%p, %s, %s, %p, %d) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        return EINVAL;
    }

    if (NULL != (json = (char*)malloc(payloadSizeBytes + 1)))
    {
        memcpy(json, payload, payloadSizeBytes);
        json[payloadSizeBytes] = '\0';

        if (NULL == (value = json_parse_string(json)))
        {
            OsConfigLogError(GetHostnameLog(), "Failed to parse JSON object");
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "Failed to allocate memory for payload");
        status = ENOMEM;
    }

    if ((status == MMI_OK) && (0 == strcmp(component, g_component)))
    {
        if (0 == strcmp(object, g_desiredName))
        {
            status = SetName(value);
        }
        else if (0 == strcmp(object, g_desiredHosts))
        {
            status = SetHosts(value);
        }
        else
        {
            OsConfigLogError(GetHostnameLog(), "MmiSet called for an invalid object name (%s)", object);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetHostnameLog(), "MmiSet called for an invalid component name (%s)", component);
        status = EINVAL;
    }

    return status;
}

void HostNameMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
