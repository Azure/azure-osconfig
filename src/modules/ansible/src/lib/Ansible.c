// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>

#include "Ansible.h"

typedef struct MIM_ANSIBLE_DATA_MAPPING {
    const char mimComponentName[64];
    const char mimObjectName[64];
    const bool mimDesired;
    const char ansibleModuleName[64];
    const char ansibleDataTransformation[512];
} MIM_ANSIBLE_DATA_MAPPING;

static const MIM_ANSIBLE_DATA_MAPPING g_dataMappings[] = {
    {"Service", "rcctl", false, "ansible.builtin.service_facts", ".ansible_facts.services | map(select(.source==\"rcctl\" and .state==\"running\").name)"},
    {"Service", "systemd", false, "ansible.builtin.service_facts", ".ansible_facts.services | map(select(.source==\"systemd\" and .state==\"running\").name)"},
    {"Service", "sysv", false, "ansible.builtin.service_facts", ".ansible_facts.services | map(select(.source==\"sysv\" and .state==\"running\").name)"},
    {"Service", "upstart", false, "ansible.builtin.service_facts", ".ansible_facts.services | map(select(.source==\"upstart\" and .state==\"running\").name)"},
    {"Service", "src", false, "ansible.builtin.service_facts", ".ansible_facts.services | map(select(.source==\"src\" and .state==\"running\").name)"},
    {"Service", "desiredServices", true, "ansible.builtin.service", ".[] | \"name=\\(.name) state=\\(.state)\""}};
static const int g_dataMappingsCount = ARRAY_SIZE(g_dataMappings);

static const char* g_ansibleModuleInfo = "{\"Name\": \"Ansible\","
    "\"Description\": \"Provides functionality to observe and configure configuration\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"Service\", \"Docker\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_ansibleModuleName = "Ansible module";

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static const char* g_ansibleLogFile = "/var/log/osconfig_ansible.log";
static const char* g_ansibleRolledLogFile = "/var/log/osconfig_ansible.bak";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE AnsibleGetLog()
{
    return g_log;
}

void AnsibleInitialize()
{
    g_log = OpenLog(g_ansibleLogFile, g_ansibleRolledLogFile);

    /*
     * Install ansible-core on the system, preferably in an isolated environment (e.g., venv):
     * curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
     * python3 get-pip.py --user
     * python3 -m pip install --user ansible-core
     * 
     * Install external an Ansible module on the system:
     * /root/.local/bin/ansible-galaxy collection install community.docker
     * 
     * Install jq on the system:
     * sudo apt install jq
     * 
     */ 
        
    OsConfigLogInfo(AnsibleGetLog(), "%s initialized", g_ansibleModuleName);
}

void AnsibleShutdown(void)
{
    OsConfigLogInfo(AnsibleGetLog(), "%s shutting down", g_ansibleModuleName);

    CloseLog(&g_log);
}

MMI_HANDLE AnsibleMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_ansibleModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(AnsibleGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_ansibleModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void AnsibleMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(AnsibleGetLog(), "MmiClose(%p)", clientSession);
    }
    else 
    {
        OsConfigLogError(AnsibleGetLog(), "MmiClose() called outside of a valid session");
    }
}

int AnsibleMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(AnsibleGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return status;
    }
    
    *payloadSizeBytes = (int)strlen(g_ansibleModuleInfo);
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memset(*payload, 0, *payloadSizeBytes);
        memcpy(*payload, g_ansibleModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(AnsibleGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(AnsibleGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int AnsibleMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    const char* commandFormat = "%s localhost -m %s -o 2> /dev/null | grep -o '{.*' | jq -c '%s' | tr '\n' ' '";

    int status = MMI_OK;
    char* result = NULL;

    char* commandBuffer = NULL;
    int commandBufferSizeBytes = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(AnsibleGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(AnsibleGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }

    if (MMI_OK == status)
    {
        for (int i = 0; i < g_dataMappingsCount; i++)
        {
            if ((0 == strcmp(g_dataMappings[i].mimComponentName, componentName)) && 
                (0 == strcmp(g_dataMappings[i].mimObjectName, objectName)) &&
                (!g_dataMappings[i].mimDesired))
            {
                commandBufferSizeBytes = snprintf(NULL, 0, commandFormat, "/root/.local/bin/ansible", g_dataMappings[i].ansibleModuleName, g_dataMappings[i].ansibleDataTransformation);
                commandBuffer = malloc(commandBufferSizeBytes + 1);

                if (NULL != commandBuffer)
                {
                    memset(commandBuffer, 0, commandBufferSizeBytes + 1);
                    snprintf(commandBuffer, commandBufferSizeBytes + 1, commandFormat, "/root/.local/bin/ansible", g_dataMappings[i].ansibleModuleName, g_dataMappings[i].ansibleDataTransformation);

                    if ((0 != ExecuteCommand(NULL, commandBuffer, false, false, 0, 0, &result, NULL, AnsibleGetLog())))
                    {
                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogError(AnsibleGetLog(), "MmiGet failed to execute command '%s'", commandBuffer);
                        }
                        status = EINVAL;
                    }
                }
                else 
                {
                    AnsibleGetLog(AnsibleGetLog(), "MmiGet: failed to allocate %d bytes", commandBufferSizeBytes + 1);
                    status = ENOMEM;
                }

                break;
            }

            // TODO: Output error for invalid component name or object name.
        }
    }

    if ((MMI_OK == status) && (NULL != result))
    {
        *payloadSizeBytes = strlen(result);
        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(AnsibleGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated", componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);
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
            OsConfigLogError(AnsibleGetLog(), "MmiGet failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }
    else 
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(AnsibleGetLog(), "MmiGet failed to serialize JSON array");
        }
        status = EINVAL;
    }

    FREE_MEMORY(commandBuffer);
    FREE_MEMORY(result);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(AnsibleGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int AnsibleMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    const char* commandFormat = "echo '%s' | jq -c '%s' | xargs -L1 %s localhost -m %s -a";

    int status = MMI_OK;
    char* buffer = NULL;

    char* commandBuffer = NULL;
    int commandBufferSizeBytes = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (payloadSizeBytes <= 0))
    {
        OsConfigLogError(AnsibleGetLog(), "MmiSet(%s, %s, %p, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (!IsValidSession(clientSession))
    {
        OsConfigLogError(AnsibleGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
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

            for (int i = 0; i < g_dataMappingsCount; i++)
            {
                if ((0 == strcmp(g_dataMappings[i].mimComponentName, componentName)) && 
                    (0 == strcmp(g_dataMappings[i].mimObjectName, objectName)) &&
                    (g_dataMappings[i].mimDesired))
                {
                    commandBufferSizeBytes = snprintf(NULL, 0, commandFormat, buffer, g_dataMappings[i].ansibleDataTransformation, "/root/.local/bin/ansible", g_dataMappings[i].ansibleModuleName);
                    commandBuffer = malloc(commandBufferSizeBytes + 1);

                    if (NULL != commandBuffer)
                    {
                        memset(commandBuffer, 0, commandBufferSizeBytes + 1);
                        snprintf(commandBuffer, commandBufferSizeBytes + 1, commandFormat, buffer, g_dataMappings[i].ansibleDataTransformation, "/root/.local/bin/ansible", g_dataMappings[i].ansibleModuleName);

                        if ((0 != ExecuteCommand(NULL, commandBuffer, false, false, 0, 0, NULL, NULL, AnsibleGetLog())))
                        {
                            if (IsFullLoggingEnabled())
                            {
                                OsConfigLogError(AnsibleGetLog(), "MmiSet failed to execute command '%s'", commandBuffer);
                            }
                            status = EINVAL;
                        }
                    }
                    else 
                    {
                        AnsibleGetLog(AnsibleGetLog(), "MmiSet: failed to allocate %d bytes", commandBufferSizeBytes + 1);
                        status = ENOMEM;
                    }

                    break;
                }

                // TODO: Output error for invalid component name or object name.
            }
        }
        else 
        {
            OsConfigLogError(AnsibleGetLog(), "MmiSet failed to allocate %d bytes", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }

    FREE_MEMORY(buffer);

    OsConfigLogInfo(AnsibleGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
    
    return status;
}

void AnsibleMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
