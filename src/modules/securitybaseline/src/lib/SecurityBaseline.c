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

static const char* g_auditSecurityBaselineObject = "auditSecurityBaseline";
static const char* g_auditEnsurePermissionsOnEtcIssueObject = "auditEnsurePermissionsOnEtcIssue";
static const char* g_auditEnsurePermissionsOnEtcIssueNetObject = "auditEnsurePermissionsOnEtcIssueNet";
static const char* g_auditEnsurePermissionsOnEtcHostsAllowObject = "auditEnsurePermissionsOnEtcHostsAllow";
static const char* g_auditEnsurePermissionsOnEtcHostsDenyObject = "auditEnsurePermissionsOnEtcHostsDeny";
static const char* g_auditEnsurePermissionsOnEtcSshSshdConfigObject = "auditEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_auditEnsurePermissionsOnEtcShadowObject = "auditEnsurePermissionsOnEtcShadow";
static const char* g_auditEnsurePermissionsOnEtcShadowDashObject = "auditEnsurePermissionsOnEtcShadowDash";
static const char* g_auditEnsurePermissionsOnEtcGShadowObject = "auditEnsurePermissionsOnEtcGShadow";
static const char* g_auditEnsurePermissionsOnEtcGShadowDashObject = "auditEnsurePermissionsOnEtcGShadowDash";
static const char* g_auditEnsurePermissionsOnEtcPasswdObject = "auditEnsurePermissionsOnEtcPasswd";
static const char* g_auditEnsurePermissionsOnEtcPasswdDashObject = "auditEnsurePermissionsOnEtcPasswdDash";
static const char* g_auditEnsurePermissionsOnEtcGroupObject = "auditEnsurePermissionsOnEtcGroup";
static const char* g_auditEnsurePermissionsOnEtcGroupDashObject = "auditEnsurePermissionsOnEtcGroupDash";
static const char* g_auditEnsurePermissionsOnEtcAnaCronTabObject = "auditEnsurePermissionsOnEtcAnaCronTab";
static const char* g_auditEnsurePermissionsOnEtcCronDObject = "auditEnsurePermissionsOnEtcCronD";
static const char* g_auditEnsurePermissionsOnEtcCronDailyObject = "auditEnsurePermissionsOnEtcCronDaily";
static const char* g_auditEnsurePermissionsOnEtcCronHourlyObject = "auditEnsurePermissionsOnEtcCronHourly";
static const char* g_auditEnsurePermissionsOnEtcCronMonthlyObject = "auditEnsurePermissionsOnEtcCronMonthly";
static const char* g_auditEnsurePermissionsOnEtcCronWeeklyObject = "auditEnsurePermissionsOnEtcCronWeekly";

static const char* g_remediateSecurityBaselineObject = "remediateSecurityBaseline";
static const char* g_remediateEnsurePermissionsOnEtcIssueObject = "remediateEnsurePermissionsOnEtcIssue";
static const char* g_remediateEnsurePermissionsOnEtcIssueNetObject = "remediateEnsurePermissionsOnEtcIssueNet";
static const char* g_remediateEnsurePermissionsOnEtcHostsAllowObject = "remediateEnsurePermissionsOnEtcHostsAllow";
static const char* g_remediateEnsurePermissionsOnEtcHostsDenyObject = "remediateEnsurePermissionsOnEtcHostsDeny";
static const char* g_remediateEnsurePermissionsOnEtcSshSshdConfigObject = "remediateEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_remediateEnsurePermissionsOnEtcShadowObject = "remediateEnsurePermissionsOnEtcShadow";
static const char* g_remediateEnsurePermissionsOnEtcShadowDashObject = "remediateEnsurePermissionsOnEtcShadowDash";
static const char* g_remediateEnsurePermissionsOnEtcGShadowObject = "remediateEnsurePermissionsOnEtcGShadow";
static const char* g_remediateEnsurePermissionsOnEtcGShadowDashObject = "remediateEnsurePermissionsOnEtcGShadowDash";
static const char* g_remediateEnsurePermissionsOnEtcPasswdObject = "remediateEnsurePermissionsOnEtcPasswd";
static const char* g_remediateEnsurePermissionsOnEtcPasswdDashObject = "remediateEnsurePermissionsOnEtcPasswdDash";
static const char* g_remediateEnsurePermissionsOnEtcGroupObject = "remediateEnsurePermissionsOnEtcGroup";
static const char* g_remediateEnsurePermissionsOnEtcGroupDashObject = "remediateEnsurePermissionsOnEtcGroupDash";
static const char* g_remediateEnsurePermissionsOnEtcAnaCronTabObject = "remediateEnsurePermissionsOnEtcAnaCronTab";
static const char* g_remediateEnsurePermissionsOnEtcCronDObject = "remediateEnsurePermissionsOnEtcCronD";
static const char* g_remediateEnsurePermissionsOnEtcCronDailyObject = "remediateEnsurePermissionsOnEtcCronDaily";
static const char* g_remediateEnsurePermissionsOnEtcCronHourlyObject = "remediateEnsurePermissionsOnEtcCronHourly";
static const char* g_remediateEnsurePermissionsOnEtcCronMonthlyObject = "remediateEnsurePermissionsOnEtcCronMonthly";
static const char* g_remediateEnsurePermissionsOnEtcCronWeeklyObject = "remediateEnsurePermissionsOnEtcCronWeekly";

static const char* g_securityBaselineLogFile = "/var/log/osconfig_securitybaseline.log";
static const char* g_securityBaselineRolledLogFile = "/var/log/osconfig_securitybaseline.bak";

static const char* g_securityBaselineModuleInfo = "{\"Name\": \"SecurityBaseline\","
    "\"Description\": \"Provides functionality to audit and remediate Security Baseline policies on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Zinc\","
    "\"Components\": [\"SecurityBaseline\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_etcIssue = "/etc/issue";
static const char* g_etcIssueNet = "/etc/issue.net";
static const char* g_etcHostsAllow = "/etc/hosts.allow";
static const char* g_etcHostsDeny = "/etc/hosts.deny";
static const char* g_etcSshSshdConfig = "/etc/ssh/sshd_config";
static const char* g_etcShadow = "/etc/shadow";
static const char* g_etcShadowDash = "/etc/shadow-";
static const char* g_etcGShadow = "/etc/gshadow";
static const char* g_etcGShadowDash = "/etc/gshadow-";
static const char* g_etcPasswd = "/etc/passwd";
static const char* g_etcPasswdDash = "/etc/passwd-";
static const char* g_etcGroup = "/etc/group";
static const char* g_etcGroupDash = "/etc/group-";
static const char* g_etcAnaCronTab = "/etc/anacrontab";
static const char* g_etcCronD = "/etc/cron.d";
static const char* g_etcCronDaily = "/etc/cron.daily";
static const char* g_etcCronHourly = "/etc/cron.hourly";
static const char* g_etcCronMonthly = "/etc/cron.monthly";
static const char* g_etcCronWeekly = "/etc/cron.weekly";

static const char* g_pass = "\"PASS\"";
static const char* g_fail = "\"FAIL\"";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static char* g_auditSecurityBaseline = NULL;
static char* g_auditEnsurePermissionsOnEtcIssue = NULL;
static char* g_auditEnsurePermissionsOnEtcIssueNet = NULL;
static char* g_auditEnsurePermissionsOnEtcHostsAllow = NULL;
static char* g_auditEnsurePermissionsOnEtcHostsDeny = NULL;
static char* g_auditEnsurePermissionsOnEtcSshSshdConfig = NULL;
static char* g_auditEnsurePermissionsOnEtcShadow = NULL;
static char* g_auditEnsurePermissionsOnEtcShadowDash = NULL;
static char* g_auditEnsurePermissionsOnEtcGShadow = NULL;
static char* g_auditEnsurePermissionsOnEtcGShadowDash = NULL;
static char* g_auditEnsurePermissionsOnEtcPasswd = NULL;
static char* g_auditEnsurePermissionsOnEtcPasswdDash = NULL;
static char* g_auditEnsurePermissionsOnEtcGroup = NULL;
static char* g_auditEnsurePermissionsOnEtcGroupDash = NULL;
static char* g_auditEnsurePermissionsOnEtcAnaCronTab = NULL;
static char* g_auditEnsurePermissionsOnEtcCronD = NULL;
static char* g_auditEnsurePermissionsOnEtcCronDaily = NULL;
static char* g_auditEnsurePermissionsOnEtcCronHourly = NULL;
static char* g_auditEnsurePermissionsOnEtcCronMonthly = NULL;
static char* g_auditEnsurePermissionsOnEtcCronWeekly = NULL;

static char* g_remediateSecurityBaseline = NULL;
static char* g_remediateEnsurePermissionsOnEtcIssue = NULL;
static char* g_remediateEnsurePermissionsOnEtcIssueNet = NULL;
static char* g_remediateEnsurePermissionsOnEtcHostsAllow = NULL;
static char* g_remediateEnsurePermissionsOnEtcHostsDeny = NULL;
static char* g_remediateEnsurePermissionsOnEtcSshSshdConfig = NULL;
static char* g_remediateEnsurePermissionsOnEtcShadow = NULL;
static char* g_remediateEnsurePermissionsOnEtcShadowDash = NULL;
static char* g_remediateEnsurePermissionsOnEtcGShadow = NULL;
static char* g_remediateEnsurePermissionsOnEtcGShadowDash = NULL;
static char* g_remediateEnsurePermissionsOnEtcPasswd = NULL;
static char* g_remediateEnsurePermissionsOnEtcPasswdDash = NULL;
static char* g_remediateEnsurePermissionsOnEtcGroup = NULL;
static char* g_remediateEnsurePermissionsOnEtcGroupDash = NULL;
static char* g_remediateEnsurePermissionsOnEtcAnaCronTab = NULL;
static char* g_remediateEnsurePermissionsOnEtcCronD = NULL;
static char* g_remediateEnsurePermissionsOnEtcCronDaily = NULL;
static char* g_remediateEnsurePermissionsOnEtcCronHourly = NULL;
static char* g_remediateEnsurePermissionsOnEtcCronMonthly = NULL;
static char* g_remediateEnsurePermissionsOnEtcCronWeekly = NULL;

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
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcSshSshdConfig);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcShadow);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcShadowDash);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcGShadow);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcGShadowDash);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcPasswd);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcPasswdDash);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcGroup);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcGroupDash);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcAnaCronTab);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcCronD);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcCronDaily);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcCronHourly);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcCronMonthly);
    FREE_MEMORY(g_auditEnsurePermissionsOnEtcCronWeekly);

    FREE_MEMORY(g_remediateSecurityBaseline);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcIssue);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcIssueNet);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcHostsAllow);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcHostsDeny);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcSshSshdConfig);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcShadow);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcShadowDash);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcGShadow);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcGShadowDash);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcPasswd);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcPasswdDash);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcGroup);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcGroupDash);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcAnaCronTab);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcCronD);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcCronDaily);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcCronHourly);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcCronMonthly);
    FREE_MEMORY(g_remediateEnsurePermissionsOnEtcCronWeekly);
    
    CloseLog(&g_log);
}

static int AuditEnsurePermissionsOnEtcIssue(void)
{
    return CheckFileAccess(g_etcIssue, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcIssueNet(void)
{
    return CheckFileAccess(g_etcIssueNet, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcHostsAllow(void)
{
    return CheckFileAccess(g_etcHostsAllow, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcHostsDeny(void)
{
    return CheckFileAccess(g_etcHostsDeny, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcSshSshdConfig(void)
{
    // The Security Baseline asks for /etc/ssh/sshd_config. 
    // On Ubuntu /etc/ssh/sshd_config is not present unless the openssh-server package is installed. 
    // etc/ssh/sshd_config is installed with the ssh client and is inbox in the basic Ubuntu distro.
    return FileExists(g_etcSshSshdConfig) ? CheckFileAccess(g_etcSshSshdConfig, 0, 0, 600, SecurityBaselineGetLog()) : 0;
};

static int AuditEnsurePermissionsOnEtcShadow(void)
{
    return CheckFileAccess(g_etcShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcShadowDash(void)
{
    return CheckFileAccess(g_etcShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGShadow(void)
{
    return CheckFileAccess(g_etcGShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGShadowDash(void)
{
    return CheckFileAccess(g_etcGShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcPasswd(void)
{
    return CheckFileAccess(g_etcPasswd, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcPasswdDash(void)
{
    return CheckFileAccess(g_etcPasswdDash, 0, 0, 600, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGroup(void)
{
    return CheckFileAccess(g_etcGroup, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcGroupDash(void)
{
    return CheckFileAccess(g_etcGroupDash, 0, 0, 644, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcAnaCronTab(void)
{
    return CheckFileAccess(g_etcAnaCronTab, 0, 0, 600, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronD(void)
{
    return CheckFileAccess(g_etcCronD, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronDaily(void)
{
    return CheckFileAccess(g_etcCronDaily, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronHourly(void)
{
    return CheckFileAccess(g_etcCronHourly, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronMonthly(void)
{
    return CheckFileAccess(g_etcCronMonthly, 0, 0, 700, SecurityBaselineGetLog());
};

static int AuditEnsurePermissionsOnEtcCronWeekly(void)
{
    return CheckFileAccess(g_etcCronWeekly, 0, 0, 700, SecurityBaselineGetLog());
};

int AuditSecurityBaseline(void)
{
    return ((0 == AuditEnsurePermissionsOnEtcIssue()) && 
        (0 == AuditEnsurePermissionsOnEtcIssueNet()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsAllow()) && 
        (0 == AuditEnsurePermissionsOnEtcHostsDeny()) &&
        (0 == AuditEnsurePermissionsOnEtcSshSshdConfig()) &&
        (0 == AuditEnsurePermissionsOnEtcShadow()) &&
        (0 == AuditEnsurePermissionsOnEtcShadowDash()) &&
        (0 == AuditEnsurePermissionsOnEtcGShadow()) &&
        (0 == AuditEnsurePermissionsOnEtcGShadowDash()) &&
        (0 == AuditEnsurePermissionsOnEtcPasswd()) &&
        (0 == AuditEnsurePermissionsOnEtcPasswdDash()) &&
        (0 == AuditEnsurePermissionsOnEtcGroup()) &&
        (0 == AuditEnsurePermissionsOnEtcGroupDash()) &&
        (0 == AuditEnsurePermissionsOnEtcAnaCronTab()) &&
        (0 == AuditEnsurePermissionsOnEtcCronD()) &&
        (0 == AuditEnsurePermissionsOnEtcCronDaily()) &&
        (0 == AuditEnsurePermissionsOnEtcCronHourly()) &&
        (0 == AuditEnsurePermissionsOnEtcCronMonthly()) &&
        (0 == AuditEnsurePermissionsOnEtcCronWeekly())) ? 0 : ENOENT;
}

static int RemediateEnsurePermissionsOnEtcIssue(void)
{
    return SetFileAccess(g_etcIssue, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcIssueNet(void)
{
    return SetFileAccess(g_etcIssueNet, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsAllow(void)
{
    return SetFileAccess(g_etcHostsAllow, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcHostsDeny(void)
{
    return SetFileAccess(g_etcHostsDeny, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcSshSshdConfig(void)
{
    // The Security Baseline asks for /etc/ssh/sshd_config. 
    // On Ubuntu /etc/ssh/sshd_config is not present unless the openssh-server package is installed. 
    // etc/ssh/sshd_config is installed with the ssh client and is inbox in the basic Ubuntu distro.
    return FileExists(g_etcSshSshdConfig) ? SetFileAccess(g_etcSshSshdConfig, 0, 0, 600, SecurityBaselineGetLog()) : 0;
};

static int RemediateEnsurePermissionsOnEtcShadow(void)
{
    return SetFileAccess(g_etcShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcShadowDash(void)
{
    return SetFileAccess(g_etcShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadow(void)
{
    return SetFileAccess(g_etcGShadow, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGShadowDash(void)
{
    return SetFileAccess(g_etcGShadowDash, 0, 42, 400, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswd(void)
{
    return SetFileAccess(g_etcPasswd, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcPasswdDash(void)
{
    return SetFileAccess(g_etcPasswdDash, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroup(void)
{
    return SetFileAccess(g_etcGroup, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcGroupDash(void)
{
    return SetFileAccess(g_etcGroupDash, 0, 0, 644, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcAnaCronTab(void)
{
    return SetFileAccess(g_etcAnaCronTab, 0, 0, 600, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronD(void)
{
    return SetFileAccess(g_etcCronD, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronDaily(void)
{
    return SetFileAccess(g_etcCronDaily, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronHourly(void)
{
    return SetFileAccess(g_etcCronHourly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronMonthly(void)
{
    return SetFileAccess(g_etcCronMonthly, 0, 0, 700, SecurityBaselineGetLog());
};

static int RemediateEnsurePermissionsOnEtcCronWeekly(void)
{
    return SetFileAccess(g_etcCronWeekly, 0, 0, 700, SecurityBaselineGetLog());
};

int RemediateSecurityBaseline(void)
{
    return ((0 == RemediateEnsurePermissionsOnEtcIssue()) && 
        (0 == RemediateEnsurePermissionsOnEtcIssueNet()) &&
        (0 == RemediateEnsurePermissionsOnEtcHostsAllow()) && 
        (0 == RemediateEnsurePermissionsOnEtcHostsDeny()) &&
        (0 == RemediateEnsurePermissionsOnEtcSshSshdConfig()) &&
        (0 == RemediateEnsurePermissionsOnEtcShadow()) &&
        (0 == RemediateEnsurePermissionsOnEtcShadowDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcGShadow()) &&
        (0 == RemediateEnsurePermissionsOnEtcGShadowDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcPasswd()) &&
        (0 == RemediateEnsurePermissionsOnEtcPasswdDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcGroup()) &&
        (0 == RemediateEnsurePermissionsOnEtcGroupDash()) &&
        (0 == RemediateEnsurePermissionsOnEtcAnaCronTab()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronD()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronDaily()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronHourly()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronMonthly()) &&
        (0 == RemediateEnsurePermissionsOnEtcCronWeekly())) ? 0 : ENOENT;
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
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            result = AuditEnsurePermissionsOnEtcSshSshdConfig() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcShadow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcShadowDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadow() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGShadowDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGShadowDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswd() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcPasswdDashObject))
        {
            result = AuditEnsurePermissionsOnEtcPasswdDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupObject))
        {
            result = AuditEnsurePermissionsOnEtcGroup() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcGroupDashObject))
        {
            result = AuditEnsurePermissionsOnEtcGroupDash() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcAnaCronTabObject))
        {
            result = AuditEnsurePermissionsOnEtcAnaCronTab() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDObject))
        {
            result = AuditEnsurePermissionsOnEtcCronD() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronDailyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronDaily() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronHourlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronHourly() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronMonthlyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronMonthly() ? g_fail : g_pass;
        }
        else if (0 == strcmp(objectName, g_auditEnsurePermissionsOnEtcCronWeeklyObject))
        {
            result = AuditEnsurePermissionsOnEtcCronWeekly() ? g_fail : g_pass;
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

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);

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
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject))
        {
            status = RemediateEnsurePermissionsOnEtcSshSshdConfig();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcShadowDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadow();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGShadowDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGShadowDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswd();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcPasswdDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcPasswdDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroup();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcGroupDashObject))
        {
            status = RemediateEnsurePermissionsOnEtcGroupDash();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcAnaCronTabObject))
        {
            status = RemediateEnsurePermissionsOnEtcAnaCronTab();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronD();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronDailyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronDaily();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronHourlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronHourly();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronMonthlyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronMonthly();
        }
        else if (0 == strcmp(objectName, g_remediateEnsurePermissionsOnEtcCronWeeklyObject))
        {
            status = RemediateEnsurePermissionsOnEtcCronWeekly();
        }
        else
        {
            OsConfigLogError(SecurityBaselineGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
            status = EINVAL;
        }
    }

    FREE_MEMORY(payloadString);

    OsConfigLogInfo(SecurityBaselineGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    return status;
}

void SecurityBaselineMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
