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

static const char* g_pass = "PASS";
static const char* g_fail = "FAIL";

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

static int CheckFileAccess(const char* fileName, uid_t expectedUserId, gid_t expectedGroupId, mode_t maxPermissions)
{
    struct statStruct = {0};
    char fileMode[10] = {0};
    int result = ENOENT;

    if (fileName && FileExists(fileName))
    {
        if (0 == (result = stat(fileName, &statStruct)))
        {
            if ((expectedUserId == statStruct.st_uid) && (expectedGroupId == statStruct.st_gid))
            {
                // Used as masks to compare actual versus expected access values considering smaller value means less access:
                // S_IRWXU (00700) owner has read, write, and execute permission
                // S_IRWXG (00070) group has read, write, and execute permission
                // S_IRWXO (00007) others (not in group) have read, write, and execute permission
                if (((maxPermissions & S_IRWXU) >= (statStruct.st_mode & S_IRWXU)) && 
                    ((maxPermissions & S_IRWXG) >= (statStruct.st_mode & S_IRWXG)) && 
                    ((maxPermissions & S_IRWXO) >= (statStruct.st_mode & S_IRWXO)))
                {
                    OsConfigLogInfo(SecurityBaselineGetLog(), "File %s (%d, %d, 0x%X) matches expected (%d, %d, 0x%X)", 
                        fileName, statStruct.st_uid, statStruct.st_gid, statStruct.st_mode, expectedUserId, expectedGroupId, maxPermissions);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "No matching access permissions for file %s (0x%X) versus expected (0x%X)", 
                        fileName, statStruct.st_mode, maxPermissions);
                }
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "No matching ownership for file %s (user: %d, group: %d) versus expected (user: %d, group: %d)", 
                    fileName, statStruct.st_uid, statStruct.st_gid, expectedUserId, expectedGroupId);
            }
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "stat(%s) failed with %d", fileName, errno);
        }
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "CheckFileAccess called with an invalid file name argument (%s)", fileName);
    }

    return result;
}

static int SetFileAccess(const char* fileName, uid_t expectedUserId, gid_t expectedGroupId, mode_t maxPermissions)
{
    int result = ENOENT;

    if (fileName && FileExists(fileName))
    {
        if (0 != (result = CheckFileAccess(fileName, expectedUserId, expectedGroupId, maxPermissions)))
        {
            if (0 == (result = chmod(fileName, maxPermissions)))
            {
                OsConfigLogInfo(SecurityBaselineGetLog(), "Successfully set file ownership to user %d, group %d with access 0x%X", 
                    fileName, expectedUserId, expectedGroupId, maxPermissions);
                result = 0;
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "chmod(%s, 0x%X) failed with %d", fileName, maxPermissions, errno);
            }
        }
        else
        {
            OsConfigLogInfo(SecurityBaselineGetLog(), "Desired file ownership (user %d, group %d with access 0x%X) already set",
                fileName, expectedUserId, expectedGroupId, maxPermissions);
            result = 0;
        }
    }
    else
    {
        OsConfigLogError(SecurityBaselineGetLog(), "SetFileAccess called with an invalid file name argument (%s)", fileName);
    }

    return result;
}

static int AuditEnsurePermissionsOnEtcIssue(void)
{
    return CheckFileAccess("/etc/issue", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcIssueNet(void)
{
    return CheckFileAccess("/etc/issue.net", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    return CheckFileAccess("/etc/hosts.allow", 0, 0, 644);
};

static int AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    return CheckFileAccess("/etc/hosts.deny", 0, 0, 644);
};

int AuditSecurityBaseline(void)
{
    return ((0 == AuditEnsurePermissionsOnEtcIssue()) && (0 == AuditEnsurePermissionsOnEtcIssueNet()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsAllow()) && (0 == AuditEnsurePermissionsOnEtcHostsDeny())) ? 0 : ENOENT;
}

static int RemediateEnsurePermissionsOnEtcIssue(void)
{
    return SetFileAccess("/etc/issue", 0, 0, 644);
};

static int RemediateEnsurePermissionsOnEtcIssueNet(void)
{
    return SetFileAccess("/etc/issue.net", 0, 0, 644);
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(void)
{
    return SetFileAccess("/etc/hosts.allow", 0, 0, 644);
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(void)
{
    return SetFileAccess("/etc/hosts.deny", 0, 0, 644);
};

int RemediateSecurityBaseline(void)
{
    return ((0 == RemediateEnsurePermissionsOnEtcIssue()) && (0 == RemediateEnsurePermissionsOnEtcIssueNet()) &&
        (0 == RemediateEnsurePermissionsOnEtcHostsAllow()) && (0 == RemediateEnsurePermissionsOnEtcHostsDeny())) ? 0 : ENOENT;
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
    char* result = NULL;
    char* buffer = NULL;
    const size_t length = strlen(g_pass) + 1;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (NULL == (buffer = malloc(length)))
    {
        OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) failed due to out of memory condition", componentName, objectName);
        status = ENOMEM;
        return status;
    }

    memset(buffer, 0, length);

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
    else
    {
        if (0 == strcmp(objectName, g_auditSecurityBaselineObject))
        {
            result = AuditSecurityBaseline() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueObject))
        {
            result = AuditEnsurePermissionsOnEtcIssue() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcIssueNetObject))
        {
            result = AuditEnsurePermissionsOnEtcIssueNet() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsAllowObject))
        {
            result = AuditEnsurePermissionsOnEtcHostsAllow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcHostsDenyObject))
        {
            result = AudittEnsurePermissionsOnEtcHostsDeny() ? g_fail : g_pass;
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        *payloadSizeBytes = strlen(result);

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported buffer will be truncated",
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
        if (*payload)
        {
            memcpy(*payload, result, *payloadSizeBytes);
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
        if (0 == strcmp(objectName, g_remediateSecurityBaselineObject))
        {
            status = RemediateSecurityBaseline();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssue();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcIssueNetObject))
        {
            status = RemediateEnsurePermissionsOnEtcIssueNet();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsAllowObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsAllow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcHostsDenyObject))
        {
            status = RemediateEnsurePermissionsOnEtcHostsDeny();
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
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
