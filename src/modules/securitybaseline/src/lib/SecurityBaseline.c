// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "SecurityBaseline.h"

static const char* g_securityBaselineModuleName = "OSConfig SecurityBaseline module";
static const char* g_securityBaselineComponentName = "SecurityBaseline";

static const char* g_auditSecurityBaselineObject = "AuditSecurityBaseline";
static const char* g_auditEnsurePermissionsOnEtcIssueObject = "AuditEnsurePermissionsOnEtcIssue";
static const char* g_auditEnsurePermissionsOnEtcIssueNetObject = "AuditEnsurePermissionsOnEtcIssueNet";
static const char* g_auditEnsurePermissionsOnEtcHostsAllowObject = "AuditEnsurePermissionsOnEtcHostsAllow";
static const char* g_auditEnsurePermissionsOnEtcHostsDenyObject = "AuditEnsurePermissionsOnEtcHostsDeny";

static const char* g_remediateSecurityBaselineObject = "RemediateSecurityBaseline";
static const char* g_remediateEnsurePermissionsOnEtcIssueObject = "RemediateEnsurePermissionsOnEtcIssue";
static const char* g_remediateEnsurePermissionsOnEtcIssueNetObject = "RemediateEnsurePermissionsOnEtcIssueNet";
static const char* g_remediateEnsurePermissionsOnEtcHostsAllowObject = "RemediateEnsurePermissionsOnEtcHostsAllow";
static const char* g_remediateEnsurePermissionsOnEtcHostsDenyObject = "RemediateEnsurePermissionsOnEtcHostsDeny";

static const char* g_securityBaselineLogFile = "/var/log/osconfig_securitybaseline.log";
static const char* g_securityBaselineRolledLogFile = "/var/log/osconfig_securitybaseline.bak";

static const char* g_compliant = "Compliant";
static const char* g_incompliant = "Incompliant";

static const char* g_securityBaselineModuleInfo = "{\"Name\": \"SecurityBaseline\","
    "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Zinc\","
    "\"Components\": [\"SecurityBaseline\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_securityBaselineLogFile = "/var/log/osconfig_securitybaseline.log";
static const char* g_securityBaselineRolledLogFile = "/var/log/osconfig_securitybaseline.bak";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static char* g_auditSecurityBaseline = NULL;
static char* g_auditEnsurePermissionsOnEtcIssue = NULL;
static char* g_auditEnsurePermissionsOnEtcIssueNet = NULL;
static char* g_auditEnsurePermissionsOnEtcHostsAllow = NULL;
static char* g_auditEnsurePermissionsOnEtcHostsDeny = NULL;

static char* g_remediateSecurityBaseline = NULL;
static char* g_remediateEnsurePermissionsOnEtcIssue = NULL;
static char* g_remediateEnsurePermissionsOnEtcIssueNet = NULL;
static char* g_remediateEnsurePermissionsOnEtcHostsAllow = NULL;
static char* g_remediateEnsurePermissionsOnEtcHostsDeny = NULL;

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE SecurityBaselineGetLog(void)
{
    return g_log;
}

void SecurityBaselineInitialize(void)
{
    g_log = OpenLog(g_securityBaselineLogFile, g_securityBaselineRolledLogFile);

    OsConfigLogInfo(SecurityBaselineGetLog(), "%s initialized", g_securityBaselineModuleName);
}

void SecurityBaselineShutdown(void)
{
    OsConfigLogInfo(SecurityBaselineGetLog(), "%s shutting down", g_securityBaselineModuleName);

    FREE_MEMORY(g_gitBranch);

    FREE_MEMORY(g_auditSecurityBaseline);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcIssue);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcIssueNet);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcHostsAllow);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcHostsDeny);

    FREE_MEMORY(g_remediateSecurityBaseline);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcIssue);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcIssueNet);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcHostsAllow);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcHostsDeny);
    
    CloseLog(&g_log);
}

static int AuditEnsurePermissionsOnFile(const char* fileName, uid_t expectedUserId, gid_t expectedGroupId, mode_t expectedFileMode)
{
    struct statStruct = {0};
    int result = ENOENT;

    if (fileName && FileExists(fileName))
    {
        if (0 == (result = stat(fileName, &statStruct)))
        {
            if ((expectedUserId == statStruct.st_uid) && (expectedGroupId == statStruct.st_gid) && (expectedFileMode == statStruct.st_mode))
            {
                result = 0;
            }
        }
    }

    return result;
}

static int AuditEnsurePermissionsOnEtcIssue(void)
{
    return AuditEnsurePermissionsOnFile("/etc/issue", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcIssueNet(void)
{
    return AuditEnsurePermissionsOnFile("/etc/issue.net", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    return AuditEnsurePermissionsOnFile("/etc/hosts.allow", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    return AuditEnsurePermissionsOnFile("/etc/hosts.deny", 0, 0, 644);
};

int AuditSecurityBaseline(void)
{
    return ((0 == AuditEnsurePermissionsOnEtcIssue()) && (0 == AuditEnsurePermissionsOnEtcIssueNet()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsAllow()) && (0 == AuditEnsurePermissionsOnEtcHostsDeny())) ? 0 : ENOENT;
}

int RemediateEnsurePermissionsOnEtcIssue(void)
{

};

int RemediateEnsurePermissionsOnEtcIssueNet(void)
{

};

int RemediateEnsurePermissionsOnEtcHostsAllow(void)
{

}

int RemediateEnsurePermissionsOnEtcHostsDeny(void)
{

};

int RemediateSecurityBaseline(void)
{

};


static int UpdateSecurityBaselineFile(void)
{
    const char* commandLoggingEnabledName = "CommandLogging";
    const char* fullLoggingEnabledName = "FullLogging";
    const char* localManagementEnabledName = "LocalManagement";
    const char* modelVersionName = "ModelVersion";
    const char* iotHubProtocolName = "IotHubProtocol";
    const char* refreshIntervalName = "ReportingIntervalSeconds";
    const char* gitManagementEnabledName = "GitManagement";
    const char* gitBranchName = "GitBranch";
    
    int status = MMI_OK;

    JSON_Value* jsonValue = NULL;
    JSON_Object* jsonObject = NULL;

    int modelVersion = g_modelVersion;
    int refreshInterval = g_refreshInterval;
    bool localManagementEnabled = g_localManagementEnabled;
    bool fullLoggingEnabled = g_fullLoggingEnabled;
    bool commandLoggingEnabled = g_commandLoggingEnabled;
    int iotHubProtocol = g_iotHubProtocol;
    bool gitManagementEnabled = g_gitManagementEnabled;
    char* gitBranch = DuplicateString(g_gitBranch);

    char* existingSecurityBaseline = LoadSecurityBaselineFromFile(g_securityBaselineFile);
    char* newSecurityBaseline = NULL;

    if (!existingSecurityBaseline)
    {
        OsConfigLogError(SecurityBaselineGetLog(), "No securityBaseline file, cannot update securityBaseline");
        return ENOENT;
    }

    if ((modelVersion != g_modelVersion) || (refreshInterval != g_refreshInterval) || (localManagementEnabled != g_localManagementEnabled) || 
        (fullLoggingEnabled != g_fullLoggingEnabled) || (commandLoggingEnabled != g_commandLoggingEnabled) || (iotHubProtocol != g_iotHubProtocol) ||
        (gitManagementEnabled != g_gitManagementEnabled) || strcmp(gitBranch, g_gitBranch))
    {
        if (NULL == (jsonValue = json_parse_string(existingSecurityBaseline)))
        {
            OsConfigLogError(SecurityBaselineGetLog(), "json_parse_string(%s) failed, UpdateSecurityBaselineFile failed", existingSecurityBaseline);
            status = EINVAL;
        }
        else if (NULL == (jsonObject = json_value_get_object(jsonValue)))
        {
            OsConfigLogError(SecurityBaselineGetLog(), "json_value_get_object(%s) failed, UpdateSecurityBaselineFile failed", existingSecurityBaseline);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if (JSONSuccess == json_object_set_number(jsonObject, modelVersionName, (double)modelVersion))
            {
                g_modelVersion = modelVersion;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_number(%s, %d) failed", g_modelVersionObject, modelVersion);
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, refreshIntervalName, (double)refreshInterval))
            {
                g_refreshInterval = refreshInterval;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_number(%s, %d) failed", g_refreshIntervalObject, refreshInterval);
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, localManagementEnabledName, (double)(localManagementEnabled ? 1 : 0)))
            {
                g_localManagementEnabled = localManagementEnabled;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_boolean(%s, %s) failed", g_localManagementEnabledObject, localManagementEnabled ? "true" : "false");
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, fullLoggingEnabledName, (double)(fullLoggingEnabled ? 1: 0)))
            {
                g_fullLoggingEnabled = fullLoggingEnabled;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_boolean(%s, %s) failed", g_fullLoggingEnabledObject, fullLoggingEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, commandLoggingEnabledName, (double)(commandLoggingEnabled ? 1 : 0)))
            {
                g_commandLoggingEnabled = commandLoggingEnabled;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_boolean(%s, %s) failed", g_commandLoggingEnabledObject, commandLoggingEnabled ? "true" : "false");
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, iotHubProtocolName, (double)iotHubProtocol))
            {
                g_iotHubProtocol = iotHubProtocol;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_number(%s, %d) failed", g_iotHubProtocolObject, iotHubProtocol);
            }

            if (JSONSuccess == json_object_set_number(jsonObject, gitManagementEnabledName, (double)(gitManagementEnabled ? 1 : 0)))
            {
                g_gitManagementEnabled = gitManagementEnabled;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_boolean(%s, %s) failed", g_gitManagementEnabledObject, gitManagementEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_string(jsonObject, gitBranchName, gitBranch))
            {
                FREE_MEMORY(g_gitBranch);
                g_gitBranch = DuplicateString(gitBranch);
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_object_set_string(%s, %s) failed", g_gitBranchObject, gitBranch);
            }
        }

        if (MMI_OK == status)
        {
            if (NULL != (newSecurityBaseline = json_serialize_to_string_pretty(jsonValue)))
            {
                if (false == SavePayloadToFile(g_securityBaselineFile, newSecurityBaseline, strlen(newSecurityBaseline), SecurityBaselineGetLog()))
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "Failed saving securityBaseline to %s", g_osConfigSecurityBaselineFile);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "json_serialize_to_string_pretty failed");
                status = EIO;
            }
        }
    }

    if (MMI_OK == status)
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "New securityBaseline successfully applied: %s", IsFullLoggingEnabled() ? newSecurityBaseline : "-");
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "Failed to apply new securityBaseline: %s", IsFullLoggingEnabled() ? newSecurityBaseline : "-");
    }
    
    if (jsonValue)
    {
        json_value_free(jsonValue);
    }

    if (newSecurityBaseline)
    {
        json_free_serialized_string(newSecurityBaseline);
    }

    FREE_MEMORY(gitBranch);
    FREE_MEMORY(existingSecurityBaseline);

    return status;
}

MMI_HANDLE SecurityBaselineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_securityBaselineModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_securityBaselineModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void SecurityBaselineMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiClose() called outside of a valid session");
    }
}

int SecurityBaselineMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return status;
    }
    
    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_securityBaselineModuleInfo);
    
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_securityBaselineModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int SecurityBaselineMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    char* buffer = NULL;
    char* securityBaseline = NULL;
    const size_t minimumLength = 20;
    size_t branchLength = 0;
    size_t maximumLength = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    branchLength = g_gitBranch ? strlen(g_gitBranch) : 0;
    maximumLength = branchLength + 1;

    if (maximumLength < minimumLength)
    {
        maximumLength = minimumLength;
    }

    if (NULL == (buffer = malloc(maximumLength)))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) failed due to out of memory condition", componentName, objectName);
        status = ENOMEM;
        return status;
    }

    memset(buffer, 0, maximumLength);

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    else if (NULL == (securityBaseline = LoadSecurityBaselineFromFile(g_securityBaselineFile)))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "Cannot load securityBaseline from %s, MmiGet failed", g_securityBaselineFile);
        status = ENOENT;
    }
    else
    {
        if (0 == strcmp(objectName, g_modelVersionObject))
        {
            snprintf(buffer, maximumLength, "%u", g_modelVersion);
        }
        else if (0 == strcmp(objectName, g_refreshIntervalObject))
        {
            snprintf(buffer, maximumLength, "%u", g_refreshInterval);
        }
        else if (0 == strcmp(objectName, g_localManagementEnabledObject))
        {
            snprintf(buffer, maximumLength, "%s", g_localManagementEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_fullLoggingEnabledObject))
        {
            snprintf(buffer, maximumLength, "%s", g_fullLoggingEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_commandLoggingEnabledObject))
        {
            snprintf(buffer, maximumLength, "%s", g_commandLoggingEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_iotHubProtocolObject))
        {
            snprintf(buffer, maximumLength, "%u", g_iotHubProtocol);
            switch (g_iotHubProtocol)
            {
                case 1:
                    snprintf(buffer, maximumLength, "%s", g_mqtt);
                    break;

                case 2:
                    snprintf(buffer, maximumLength, "%s", g_mqttWebSocket);
                    break;

                case 0:
                default:
                    snprintf(buffer, maximumLength, "%s", g_auto);

            }
        }
        else if (0 == strcmp(objectName, g_gitManagementEnabledObject))
        {
            snprintf(buffer, maximumLength, "%s", g_gitManagementEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_gitBranchObject))
        {
            snprintf(buffer, maximumLength, "%s", g_gitBranch);
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        *payloadSizeBytes = strlen(buffer);

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported buffer will be truncated",
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
        if (*payload)
        {
            memcpy(*payload, buffer, *payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }    

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    FREE_MEMORY(securityBaseline);
    FREE_MEMORY(buffer);

    return status;
}

int SecurityBaselineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    const char* stringTrue = "true";
    const char* stringFalse = "false";
    JSON_Value* jsonValue = NULL;
    const char* jsonString = NULL;
    char* payloadString = NULL;

    int status = MMI_OK;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet(%s, %s, %s, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    } 
    else if (0 != strcmp(componentName, g_securityBaselineComponentName))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if (MMI_OK == status)
    {
        if (NULL != (payloadString = malloc(payloadSizeBytes + 1)))
        {
            memset(payloadString, 0, payloadSizeBytes + 1);
            memcpy(payloadString, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "Failed to allocate %d bytes of memory, MmiSet failed", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }
    
    if (MMI_OK == status)
    {
        if (0 == strcmp(objectName, g_desiredRefreshIntervalObject))
        {
            g_refreshInterval = atoi(payloadString);
        }
        else if (0 == strcmp(objectName, g_desiredLocalManagementEnabledObject))
        {
            if (0 == strcmp(stringTrue, payloadString))
            {
                g_localManagementEnabled = true;
            }
            else if (0 == strcmp(stringFalse, payloadString))
            {
                g_localManagementEnabled = false;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Unsupported %s value: %s", g_desiredLocalManagementEnabledObject, payloadString);
                status = EINVAL;
            }
        }
        else if (0 == strcmp(objectName, g_desiredFullLoggingEnabledObject))
        {
            if (0 == strcmp(stringTrue, payloadString))
            {
                g_fullLoggingEnabled = true;
            }
            else if (0 == strcmp(stringFalse, payloadString))
            {
                g_fullLoggingEnabled = false;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Unsupported %s value: %s", g_desiredFullLoggingEnabledObject, payloadString);
                status = EINVAL;
            }
        }
        else if (0 == strcmp(objectName, g_desiredCommandLoggingEnabledObject))
        {
            if (0 == strcmp(stringTrue, payloadString))
            {
                g_commandLoggingEnabled = true;
            }
            else if (0 == strcmp(stringFalse, payloadString))
            {
                g_commandLoggingEnabled = false;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Unsupported %s value: %s", g_desiredCommandLoggingEnabledObject, payloadString);
                status = EINVAL;
            }
        }
        else if (0 == strcmp(objectName, g_desiredIotHubProtocolObject))
        {
            if (0 == strcmp(g_auto, payloadString))
            {
                g_iotHubProtocol = 0;
            }
            else if (0 == strcmp(g_mqtt, payloadString))
            {
                g_iotHubProtocol = 1;
            }
            else if (0 == strcmp(g_mqttWebSocket, payloadString))
            {
                g_iotHubProtocol = 2;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Unsupported %s value: %s", g_desiredIotHubProtocolObject, payloadString);
                status = EINVAL;
            }
        }
        else if (0 == strcmp(objectName, g_desiredGitManagementEnabledObject))
        {
            if (0 == strcmp(stringTrue, payloadString))
            {
                g_gitManagementEnabled = true;
            }
            else if (0 == strcmp(stringFalse, payloadString))
            {
                g_gitManagementEnabled = false;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Unsupported %s value: %s", g_gitManagementEnabledObject, payloadString);
                status = EINVAL;
            }
        }
        else if (0 == strcmp(objectName, g_desiredGitBranchObject))
        {
            if (NULL != (jsonValue = json_parse_string(payloadString)))
            {
                jsonString = json_value_get_string(jsonValue);
                if (jsonString)
                {
                    FREE_MEMORY(g_gitBranch);
                    g_gitBranch = DuplicateString(payloadString);
                }
                else
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "Bad string value for %s (json_value_get_string failed)", g_desiredGitBranchObject);
                    status = EINVAL;
                }
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "Bad string value for %s (json_parse_string failed)", g_desiredGitBranchObject);
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        status = UpdateSecurityBaselineFile();
    }

    FREE_MEMORY(payloadString);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogInfo(SecurityBaselineGetLog(), "MmiSet(%p, %s, %s) returning %d", clientSession, componentName, objectName, status);
    }

    return status;
}

void SecurityBaselineMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
