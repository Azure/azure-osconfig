// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
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

static mode_t GetFileAccessFlags(mode_t mode)
{
    // From: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_stat.h.html
    // ------------------------------------------------------------
    // Name       Value    Description
    // ------------------------------------------------------------
    // S_IRWXU    0700     Read, write, execute/search by owner.
    // S_IRUSR    0400     Read permission, owner.
    // S_IWUSR    0200     Write permission, owner.
    // S_IXUSR    0100     Execute/search permission, owner.
    // S_IRWXG     070     Read, write, execute/search by group.
    // S_IRGRP     040     Read permission, group.
    // S_IWGRP     020     Write permission, group.
    // S_IXGRP     010     Execute/search permission, group.
    // S_IRWXO      07     Read, write, execute/search by others.
    // S_IROTH      04     Read permission, others.
    // S_IWOTH      02     Write permission, others.
    // S_IXOTH      01     Execute/search permission, others.
    // S_ISUID   04000     Set-user-ID on execution.
    // S_ISGID   02000     Set-group-ID on execution.
    // S_ISVTX   01000     On directories, restricted deletion flag.
    //------------------------------------------------------------

    mode_t flags = 0;

    if (mode & S_IRWXU)
    {
        flags |= S_IRWXU;
    }
    else
    {
        if (mode & S_IRUSR)
        {
            flags |= S_IRUSR;
        }

        if (mode & S_IWUSR)
        {
            flags |= S_IWUSR;
        }

        if (mode & S_IXUSR)
        {
            flags |= S_IXUSR;
        }
    }
    
    if (mode & S_IRWXG)
    {
        flags |= S_IRWXG;
    }
    else
    {
        if (mode & S_IRGRP)
        {
            flags |= S_IRGRP;
        }

        if (mode & S_IWGRP)
        {
            flags |= S_IWGRP;
        }

        if (mode & S_IXGRP)
        {
            flags |= S_IXGRP;
        }
    }
    
    if (mode & S_IRWXO)
    {
        flags |= S_IRWXO;
    }
    else
    {
        if (mode & S_IROTH)
        {
            flags |= S_IROTH;
        }

        if (mode & S_IWOTH)
        {
            flags |= S_IWOTH;
        }

        if (mode & S_IXOTH)
        {
            flags |= S_IXOTH;
        }
    }
    
    if (mode & S_ISUID)
    {
        flags |= S_ISUID;
    }

    if (mode & S_ISGID)
    {
        flags |= S_ISGID;
    }

    if (mode & S_ISVTX)
    {
        flags |= S_ISVTX;
    }

    return flags;
}

static int CheckFileAccess(const char* fileName, uid_t desiredUserId, gid_t desiredGroupId, mode_t desiredFileAccess)
{
    struct stat statStruct = {0};
    mode_t currentMode = 0; 
    mode_t desiredMode = 0;
    int result = ENOENT;

    if (fileName && FileExists(fileName))
    {
        if (0 == (result = stat(fileName, &statStruct)))
        {
            if ((desiredUserId == statStruct.st_uid) && (desiredGroupId == statStruct.st_gid))
            {
                currentMode = GetFileAccessFlags(statStruct.st_mode);
                desiredMode = GetFileAccessFlags(desiredFileAccess);

                if ((((desiredMode & S_IRWXU) == (currentMode & S_IRWXU)) || (0 == (desiredMode & S_IRWXU))) &&
                    (((desiredMode & S_IRWXG) == (currentMode & S_IRWXG)) || (0 == (desiredMode & S_IRWXG))) &&
                    (((desiredMode & S_IRWXO) == (currentMode & S_IRWXO)) || (0 == (desiredMode & S_IRWXO))))
                {
                    OsConfigLogInfo(SecurityBaselineGetLog(), "File %s (%d, %d, %d-%d) matches expected (%d, %d, %d-%d)", 
                        fileName, statStruct.st_uid, statStruct.st_gid, statStruct.st_mode, currentMode, 
                        desiredUserId, desiredGroupId, desiredFileAccess, desiredMode);
                    
                    result = 0;
                }
                else
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "No matching access permissions for %s (%d-%d) versus expected (%d-%d)", 
                        fileName, statStruct.st_mode, currentMode, desiredFileAccess, desiredMode);
                }
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "No matching ownership for %s (user: %d, group: %d) versus expected (user: %d, group: %d)", 
                    fileName, statStruct.st_uid, statStruct.st_gid, desiredUserId, desiredGroupId);
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

static int SetFileAccess(const char* fileName, uid_t desiredUserId, gid_t desiredGroupId, mode_t desiredFileAccess)
{
    int result = ENOENT;

    if (fileName && FileExists(fileName))
    {
        if (0 == (result = CheckFileAccess(fileName, desiredUserId, desiredGroupId, desiredFileAccess)))
        {
            OsConfigLogInfo(SecurityBaselineGetLog(), "Desired %s ownership (user %d, group %d with access %d) already set",
                fileName, desiredUserId, desiredGroupId, desiredFileAccess);
            result = 0;
        }
        else
        {
            if (0 == (result = chown(fileName, desiredUserId, desiredGroupId)))
            {
                OsConfigLogInfo(SecurityBaselineGetLog(), "Successfully set %s ownership to user %d, group %d", fileName, desiredUserId, desiredGroupId);

                if (0 == (result = chmod(fileName, desiredFileAccess)))
                {
                    OsConfigLogInfo(SecurityBaselineGetLog(), "Successfully set %s access to %d", fileName, desiredFileAccess);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(SecurityBaselineGetLog(), "chmod(%s, %d) failed with %d", fileName, desiredFileAccess, errno);
                }
            }
            else
            {
                OsConfigLogError(SecurityBaselineGetLog(), "chown(%s, %d, %d) failed with %d", fileName, desiredUserId, desiredGroupId, errno);
            }

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
    char* buffer = NULL;
    const char* result = NULL;
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
            result = AuditEnsurePermissionsOnEtcHostsDeny() ? g_fail : g_pass;
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

    FREE_MEMORY(buffer);

    return status;
}

int SecurityBaselineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
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
