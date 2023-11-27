// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "SshUtils.h"

static const char* g_sshServerService = "sshd";
static const char* g_sshServerConfiguration = "/etc/ssh/sshd_config";
static const char* g_sshdDashTCommand = "sshd -T";

static const char* g_sshDefaultSshSshdConfigAccess = "600";
static const char* g_sshDefaultSshProtocol = "2";
static const char* g_sshDefaultSshYes = "yes";
static const char* g_sshDefaultSshNo = "no";
static const char* g_sshDefaultSshLogLevel = "Info";
static const char* g_sshDefaultSshMaxAuthTries = "6";
static const char* g_sshDefaultSshAllowUsers = "*@*";
static const char* g_sshDefaultSshDenyUsers = "root";
static const char* g_sshDefaultSshAllowGroups = "*";
static const char* g_sshDefaultSshDenyGroups = "root";
static const char* g_sshDefaultSshClientIntervalCountMax = "0";
static const char* g_sshDefaultSshClientAliveInterval = "3600";
static const char* g_sshDefaultSshLoginGraceTime = "60";
static const char* g_sshDefaultSshMacs = "hmac-sha2-256,hmac-sha2-256-etm@openssh.com,hmac-sha2-512,hmac-sha2-512-etm@openssh.com";
static const char* g_sshDefaultSshCiphers = "aes128-ctr,aes192-ctr,aes256-ctr";
static const char* g_sshDefaultSshBannerText =
    "#######################################################################\n\n"
    "Authorized access only!\n\n"
    "If you are not authorized to access or use this system, disconnect now!\n\n"
    "#######################################################################\n";

static char* g_desiredPermissionsOnEtcSshSshdConfig = NULL;
static char* g_desiredSshBestPracticeProtocol = NULL;
static char* g_desiredSshBestPracticeIgnoreRhosts = NULL;
static char* g_desiredSshLogLevelIsSet = NULL;
static char* g_desiredSshMaxAuthTriesIsSet = NULL;
static char* g_desiredAllowUsersIsConfigured = NULL;
static char* g_desiredDenyUsersIsConfigured = NULL;
static char* g_desiredAllowGroupsIsConfigured = NULL;
static char* g_desiredDenyGroupsConfigured = NULL;
static char* g_desiredSshHostbasedAuthenticationIsDisabled = NULL;
static char* g_desiredSshPermitRootLoginIsDisabled = NULL;
static char* g_desiredSshPermitEmptyPasswordsIsDisabled = NULL;
static char* g_desiredSshClientIntervalCountMaxIsConfigured = NULL;
static char* g_desiredSshClientAliveIntervalIsConfigured = NULL;
static char* g_desiredSshLoginGraceTimeIsSet = NULL;
static char* g_desiredOnlyApprovedMacAlgorithmsAreUsed = NULL;
static char* g_desiredSshWarningBannerIsEnabled = NULL;
static char* g_desiredUsersCannotSetSshEnvironmentOptions = NULL;
static char* g_desiredAppropriateCiphersForSsh = NULL;

static char* GetSshServerState(const char* name, void* log)
{
    const char* commandTemplateForOne = "%s | grep %s";
    char* command = NULL;
    char* textResult = NULL;
    int status = 0;

    if (NULL == name)
    {
        if (0 != (status = ExecuteCommand(NULL, g_sshdDashTCommand, true, false, 0, 0, &textResult, NULL, NULL)))
        {
            OsConfigLogError(log, "GetSshServerState: '%s' failed with %d and '%s'", g_sshdDashTCommand, status, textResult);
            FREE_MEMORY(textResult);
        }
    }
    else
    {
        if (NULL != (command = FormatAllocateString(commandTemplateForOne, g_sshdDashTCommand, name)))
        {
            if (0 != (status = ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, NULL)))
            {
                OsConfigLogError(log, "GetSshServerState: '%s' failed with %d and '%s'", command, status, textResult);
                FREE_MEMORY(textResult);
            }
            else if ((NULL != textResult) && (NULL != strstr(textResult, name)))
            {
                RemovePrefixUpToString(textResult, name);
                RemovePrefixUpTo(textResult, ' ');
                RemovePrefixBlanks(textResult);
                RemoveTrailingBlanks(textResult);
            }
        }
        else
        {
            OsConfigLogError(log, "GetSshServerState: FormatAllocateString failed");
        }

        FREE_MEMORY(command);
    }

    return textResult;
}

static int IsSshServerActive(void* log)
{
    int result = 0;
   
    if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogInfo(log, "IsSshServerActive: the OpenSSH Server configuration file '%s' is not present on this device", g_sshServerConfiguration);
        result = EEXIST;
    }
    else if (false == IsDaemonActive(g_sshServerService, log))
    {
        OsConfigLogInfo(log, "IsSshServerActive: the OpenSSH Server service '%s' is not active on this device", g_sshServerService);
        result = EEXIST;
    }

    return result;
}

static int CheckOnlyApprovedMacAlgorithmsAreUsed(const char* macs, char** reason, void* log)
{
    const char* sshMacs = "macs";
    char* macsValue = NULL;
    char* value = NULL;
    size_t macsValueLength = 0;
    size_t i = 0;
    int status = 0;

    if (NULL == macs)
    {
        OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: invalid arguments");
        return EINVAL;
    }
    else if (0 != IsSshServerActive(log))
    {
        return status;
    }

    if (NULL == (macsValue = GetSshServerState(sshMacs, log)))
    {
        OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: '%s' not found in SSH Server response from 'sshd -T'", sshMacs);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' not found in SSH Server response", sshMacs);
        status = ENOENT;
    }
    else
    {
        macsValueLength = strlen(macsValue);

        for (i = 0; i < macsValueLength; i++)
        {
            if (NULL == (value = DuplicateString(&(macsValue[i]))))
            {
                OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: failed to duplicate string");
                status = ENOMEM;
                break;
            }
            else
            {
                TruncateAtFirst(value, ',');

                if (NULL == strstr(macs, value))
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: unapproved MAC algorithm '%s' found in SSH Server response", value);
                    OsConfigCaptureReason(reason, "Unapproved MAC algorithm '%s' found in SSH Server response", "%s, also MAC algorithm '%s' is unapproved", value);
                }

                i += strlen(value);
                FREE_MEMORY(value);
                continue;
            }
        }
    }

    if ((0 == status) && reason)
    {
        FREE_MEMORY(*reason);
        *reason = FormatAllocateString("%sThe %s service reports that '%s' is set to '%s' (all approved MAC algorithms)", 
            SECURITY_AUDIT_PASS, g_sshServerService, sshMacs, macsValue);
    }

    FREE_MEMORY(macsValue);

    OsConfigLogInfo(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckAppropriateCiphersForSsh(const char* ciphers, char** reason, void* log)
{
    const char* sshCiphers = "ciphers";
    char* ciphersValue = NULL;
    char* value = NULL;
    size_t ciphersValueLength = 0;
    size_t i = 0;
    int status = 0;

    if (NULL == ciphers)
    {
        OsConfigLogError(log, "CheckAppropriateCiphersForSsh: invalid argument");
        return EINVAL;
    }
    else if (0 != IsSshServerActive(log))
    {
        return status;
    }

    if (NULL == (ciphersValue = GetSshServerState(sshCiphers, log)))
    {
        OsConfigLogError(log, "CheckAppropriateCiphersForSsh: '%s' not found in SSH Server response", sshCiphers);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' not found in SSH Server response", sshCiphers);
        status = ENOENT;
    }
    else
    {
        ciphersValueLength = strlen(ciphersValue);

        // Check that no unapproved ciphers are configured
        for (i = 0; i < ciphersValueLength; i++)
        {
            if (NULL == (value = DuplicateString(&(ciphersValue[i]))))
            {
                OsConfigLogError(log, "CheckAppropriateCiphersForSsh: failed to duplicate string");
                status = ENOMEM;
                break;
            }
            else
            {
                TruncateAtFirst(value, ',');

                if (NULL == strstr(ciphers, value))
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckAppropriateCiphersForSsh: unapproved cipher '%s' found in SSH Server response", value);
                    OsConfigCaptureReason(reason, "Unapproved cipher '%s' found in SSH Server response", "%s, also cipher '%s' is unapproved", value);
                }

                i += strlen(value);
                FREE_MEMORY(value);
                continue;
            }
        }

        ciphersValueLength = strlen(ciphers);

        // Check that all required ciphers are configured
        for (i = 0; i < ciphersValueLength; i++)
        {
            if (NULL == (value = DuplicateString(&(ciphers[i]))))
            {
                OsConfigLogError(log, "CheckAppropriateCiphersForSsh: failed to duplicate ciphers string");
                status = ENOMEM;
                break;
            }
            else
            {
                TruncateAtFirst(value, ',');

                if (NULL == strstr(ciphersValue, value))
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckAppropriateCiphersForSsh: required cipher '%s' not found in SSH Server response", &(ciphers[i]));
                    OsConfigCaptureReason(reason, "Required cipher '%s' not found in SSH Server response", "%s, also required cipher '%s' is not found", &(ciphers[i]));
                }

                i += strlen(value);
                FREE_MEMORY(value);
                continue;
            }
        }
    }

    if ((0 == status) && reason)
    {
        FREE_MEMORY(*reason);
        *reason = FormatAllocateString("%sThe %s service reports that '%s' is set to '%s' (only approved ciphers)",
            SECURITY_AUDIT_PASS, g_sshServerService, sshCiphers, ciphersValue);
    }

    FREE_MEMORY(ciphersValue);

    OsConfigLogInfo(log, "CheckAppropriateCiphersForSsh: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshOptionIsSet(const char* option, const char* expectedValue, char** actualValue, char** reason, void* log)
{
    char* value = NULL;
    int status = 0;

    if (NULL == option)
    {
        OsConfigLogError(log, "CheckSshOptionIsSet: invalid argument");
        return EINVAL;
    }

    if (0 != IsSshServerActive(log))
    {
        return status;
    }

    if (NULL != (value = GetSshServerState(option, log)))
    {
        OsConfigLogInfo(log, "CheckSshOptionIsSet: '%s' found in SSH Server response set to '%s'", option, value);

        if ((NULL != expectedValue) && (0 != strcmp(value, expectedValue)))
        {
            OsConfigLogError(log, "CheckSshOptionIsSet: '%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, value);
            OsConfigCaptureReason(reason, "'%s' is not set to '%s' in SSH Server response (but to '%s')",
                "%s, also '%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, value);
            status = ENOENT;
        }

        if (NULL != actualValue)
        {
            *actualValue = DuplicateString(value);
        }

        if ((0 == status) && reason)
        {
            FREE_MEMORY(*reason);
            *reason = FormatAllocateString("%sThe %s service reports that '%s' is set to '%s'", SECURITY_AUDIT_PASS, g_sshServerService, option, value);
        }

        FREE_MEMORY(value);
    }
    else
    {
        OsConfigLogError(log, "CheckSshOptionIsSet: '%s' not found in SSH Server response", option);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' is not found in SSH server response", option);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshOptionIsSet: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshOptionIsSetToInteger(const char* option, int* expectedValue, int* actualValue, char** reason, void* log)
{
    char* actualValueString = NULL;
    char* expectedValueString = expectedValue ? FormatAllocateString("%d", *expectedValue) : NULL;
    int status = CheckSshOptionIsSet(option, expectedValueString, &actualValueString, reason, log);

    if ((0 == status) && (NULL != actualValue))
    {
        *actualValue = (NULL != actualValueString) ? atoi(actualValueString) : -1;
    }

    FREE_MEMORY(actualValueString);
    FREE_MEMORY(expectedValueString);

    return status;
}

static int CheckSshClientAliveInterval(char** reason, void* log)
{
    const char* clientAliveInterval = "clientaliveinterval";
    int actualValue = 0;
    int status = 0; 
    
    if ((0 == (status = CheckSshOptionIsSetToInteger(clientAliveInterval, NULL, &actualValue, reason, log))) && (actualValue <= 0))
    {
        OsConfigLogError(log, "CheckSshClientAliveInterval: 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
        OsConfigCaptureReason(reason, "'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)",
            "%s, also 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
        status = ENOENT;
    }
    else if (reason)
    {
        FREE_MEMORY(*reason);
        *reason = FormatAllocateString("%sThe %s service reports that '%s' is set to '%d' (that is greater than zero)", 
            SECURITY_AUDIT_PASS, g_sshServerService, clientAliveInterval, actualValue);
    }

    OsConfigLogInfo(log, "CheckSshClientAliveInterval: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshLoginGraceTime(const char* value, char** reason, void* log)
{
    const char* loginGraceTime = "logingracetime";
    int targetValue = atoi(value ? value : g_sshDefaultSshLoginGraceTime);
    int actualValue = 0;
    int status = 0; 

    if ((0 == (status = CheckSshOptionIsSetToInteger(loginGraceTime, NULL, &actualValue, reason, log))) && (actualValue > 60))
    {
        OsConfigLogError(log, "CheckSshLoginGraceTime: 'logingracetime' is not set to %d or less in SSH Server response (but to %d)", targetValue, actualValue);
        OsConfigCaptureReason(reason, "'logingracetime' is not set to a value of %d or less in SSH Server response (but to %d)",
            "%s, also 'logingracetime' is not set to a value of 60 or less in SSH Server response (but to %d)", targetValue, actualValue);
        status = ENOENT;
    }
    else if (reason)
    {
        FREE_MEMORY(*reason);
        *reason = FormatAllocateString("%sThe %s service reports that '%s' is set to '%d' (that is %d or less)", 
            SECURITY_AUDIT_PASS, g_sshServerService, loginGraceTime, targetValue, actualValue);
    }

    OsConfigLogInfo(log, "CheckSshLoginGraceTime: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshWarningBanner(const char* bannerFile, const char* bannerText, char** reason, void* log)
{
    const char* banner = "banner";
    char* actualValue = NULL;
    char* contents = NULL;
    int status = 0;

    if ((NULL == bannerFile) || (NULL == bannerText))
    {
        OsConfigLogError(log, "CheckSshWarningBanner: invalid arguments");
        return EINVAL;
    }

    if (0 == (status = CheckSshOptionIsSet(banner, bannerFile, &actualValue, reason, log)))
    {
        if (NULL == (contents = LoadStringFromFile(bannerFile, false, log)))
        {
            OsConfigLogError(log, "CheckSshWarningBanner: cannot read from '%s'", bannerFile);
            OsConfigCaptureReason(reason, "'%s' is set to '%s' but the file cannot be read",
                "%s, also '%s' is set to '%s' but the file cannot be read", banner, actualValue);
            status = ENOENT;
        }
        else  if (0 != strcmp(contents, bannerText))
        {
            OsConfigLogError(log, "CheckSshWarningBanner: banner text is:\n%s instead of:\n%s", contents, bannerText);
            OsConfigCaptureReason(reason, "Banner text from file '%s'is different from not the expected text",
                "%s, also the banner text from '%s' is different from the expected text", bannerFile);
            status = ENOENT;
        }
    }

    if ((0 == status) && reason)
    {
        FREE_MEMORY(*reason);
        *reason = FormatAllocateString("%sThe sshd service reports that '%s' is set to '%s' and this file contains the expected banner text", 
            SECURITY_AUDIT_PASS, banner, actualValue);
    }

    FREE_MEMORY(contents);
    FREE_MEMORY(actualValue);

    return status;
}

static int SetSshOption(const char* option, const char* value, void* log)
{
    // Replaces any instances of 'option foo' with 'option value' and adds 'option value' when 'option' is missing
    const char* commandTemplate = "sed '/^%s /{h;s/ .*/ %s/};${x;/^$/{s//%s %s/;H};x}' %s";

    char* command = NULL;
    char* commandResult = NULL;
    size_t commandLength = 0;
    int status = 0;

    if ((NULL == option) || (NULL == value))
    {
        OsConfigLogError(log, "SetSshOption: invalid arguments (%s, %s)", option, value);
        return EINVAL;
    }
    else if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogError(log, "SetSshOption: the SSH Server configuration file '%s' is not present on this device, no place to set '%s' to '%s'", 
            g_sshServerConfiguration, option, value);
        return status;
    }

    commandLength = strlen(commandTemplate) + (2 * strlen(option)) + (2 * strlen(value)) + strlen(g_sshServerConfiguration) + 1;

    if (NULL != (command = malloc(commandLength)))
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, option, value, option, value, g_sshServerConfiguration);

        if ((0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, &commandResult, NULL, log))) && commandResult)
        {
            if (false == SavePayloadToFile(g_sshServerConfiguration, commandResult, strlen(commandResult), log))
            {
                OsConfigLogError(log, "SetSshOption: failed saving the updated configuration to '%s'", g_sshServerConfiguration);
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogInfo(log, "SetSshOption: failed setting '%s' to '%s' in '%s' (%d)", option, value, g_sshServerConfiguration, status);
        }
    }
    else
    {
        OsConfigLogError(log, "SetSshOption: out of memory");
        status = ENOMEM;
    }

    FREE_MEMORY(commandResult);
    FREE_MEMORY(command);

    OsConfigLogInfo(log, "SetSshOption('%s' to '%s'): %s (%d)", option, value, PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int SetSshWarningBanner(unsigned int desiredBannerFileAccess, const char* bannerText, void* log)
{
    const char* etcAzSec = "/etc/azsec/";
    const char* bannerFile = "/etc/azsec/banner.txt";
    const char* escapedPath = "\\/etc\\/azsec\\/banner.txt";
   
    if (NULL == bannerText)
    {
        OsConfigLogError(log, "SetSshWarningBanner: invalid argument");
        return EINVAL;
    }
    
    if (false == DirectoryExists(etcAzSec))
    {
        mkdir(etcAzSec, desiredBannerFileAccess);
    }

    SavePayloadToFile(bannerFile, bannerText, strlen(bannerText), log);

    return SetSshOption("Banner", escapedPath, log);
}

int InitializeSshAudit(void* log)
{
    int status = 0;

    if ((NULL == (g_desiredPermissionsOnEtcSshSshdConfig = DuplicateString(g_sshDefaultSshSshdConfigAccess))) ||
        (NULL == (g_desiredSshBestPracticeProtocol = DuplicateString(g_sshDefaultSshProtocol))) ||
        (NULL == (g_desiredSshBestPracticeIgnoreRhosts = DuplicateString(g_sshDefaultSshYes))) ||
        (NULL == (g_desiredSshLogLevelIsSet = DuplicateString(g_sshDefaultSshLogLevel))) ||
        (NULL == (g_desiredSshMaxAuthTriesIsSet = DuplicateString(g_sshDefaultSshMaxAuthTries))) ||
        (NULL == (g_desiredAllowUsersIsConfigured = DuplicateString(g_sshDefaultSshAllowUsers))) ||
        (NULL == (g_desiredDenyUsersIsConfigured = DuplicateString(g_sshDefaultSshDenyUsers))) ||
        (NULL == (g_desiredAllowGroupsIsConfigured = DuplicateString(g_sshDefaultSshAllowGroups))) ||
        (NULL == (g_desiredDenyGroupsConfigured = DuplicateString(g_sshDefaultSshDenyGroups))) ||
        (NULL == (g_desiredSshHostbasedAuthenticationIsDisabled = DuplicateString(g_sshDefaultSshNo))) ||
        (NULL == (g_desiredSshPermitRootLoginIsDisabled = DuplicateString(g_sshDefaultSshNo))) ||
        (NULL == (g_desiredSshPermitEmptyPasswordsIsDisabled = DuplicateString(g_sshDefaultSshNo))) ||
        (NULL == (g_desiredSshClientIntervalCountMaxIsConfigured = DuplicateString(g_sshDefaultSshClientIntervalCountMax))) ||
        (NULL == (g_desiredSshClientAliveIntervalIsConfigured = DuplicateString(g_sshDefaultSshClientAliveInterval))) ||
        (NULL == (g_desiredSshLoginGraceTimeIsSet = DuplicateString(g_sshDefaultSshLoginGraceTime))) ||
        (NULL == (g_desiredOnlyApprovedMacAlgorithmsAreUsed = DuplicateString(g_sshDefaultSshMacs))) ||
        (NULL == (g_desiredSshWarningBannerIsEnabled = DuplicateString(g_sshDefaultSshBannerText))) ||
        (NULL == (g_desiredUsersCannotSetSshEnvironmentOptions = DuplicateString(g_sshDefaultSshNo))) ||
        (NULL == (g_desiredAppropriateCiphersForSsh = DuplicateString(g_sshDefaultSshCiphers))))
    {
        OsConfigLogError(log, "InitializeSshAudit: failed to allocate memory");
        status = ENOMEM;
    }

    return status;
}

void SshAuditCleanup(void)
{
    FREE_MEMORY(g_desiredPermissionsOnEtcSshSshdConfig);
    FREE_MEMORY(g_desiredSshBestPracticeProtocol);
    FREE_MEMORY(g_desiredSshBestPracticeIgnoreRhosts);
    FREE_MEMORY(g_desiredSshLogLevelIsSet);
    FREE_MEMORY(g_desiredSshMaxAuthTriesIsSet);
    FREE_MEMORY(g_desiredAllowUsersIsConfigured);
    FREE_MEMORY(g_desiredDenyUsersIsConfigured);
    FREE_MEMORY(g_desiredAllowGroupsIsConfigured);
    FREE_MEMORY(g_desiredDenyGroupsConfigured);
    FREE_MEMORY(g_desiredSshHostbasedAuthenticationIsDisabled);
    FREE_MEMORY(g_desiredSshPermitRootLoginIsDisabled);
    FREE_MEMORY(g_desiredSshPermitEmptyPasswordsIsDisabled);
    FREE_MEMORY(g_desiredSshClientIntervalCountMaxIsConfigured);
    FREE_MEMORY(g_desiredSshClientAliveIntervalIsConfigured);
    FREE_MEMORY(g_desiredSshLoginGraceTimeIsSet);
    FREE_MEMORY(g_desiredOnlyApprovedMacAlgorithmsAreUsed);
    FREE_MEMORY(g_desiredSshWarningBannerIsEnabled);
    FREE_MEMORY(g_desiredUsersCannotSetSshEnvironmentOptions);
    FREE_MEMORY(g_desiredAppropriateCiphersForSsh);
}

char* SshUtilsAuditEnsurePermissionsOnEtcSshSshdConfig(void* log)
{
    char* reason = NULL;
    CheckFileAccess(g_etcSshSshdConfig, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), &reason, log);
    return reason;
};

char* SshUtilsAuditEnsureSshBestPracticeProtocol(void* log)
{
    char* desiredProtocol = FormatAllocateString("Protocol %s", g_desiredSshBestPracticeProtocol ? g_desiredSshBestPracticeProtocol : g_sshDefaultSshProtocol);
    char* reason = ((EEXIST == CheckFileExists(g_etcSshSshdConfig, log)) ||
        (desiredProtocol && (EEXIST == CheckLineNotFoundOrCommentedOut(g_etcSshSshdConfig, '#', desiredProtocol, log)))) ?
        FormatAllocateString("PASS'%s' is found uncommented in %s", desiredProtocol, g_etcSshSshdConfig) :
        FormatAllocateString("'%s' is not found uncommented with '#' in %s", desiredProtocol, g_etcSshSshdConfig);
    FREE_MEMORY(desiredProtocol);
    return reason;
}

char* SshUtilsAuditEnsureSshBestPracticeIgnoreRhosts(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("ignorerhosts", g_desiredSshBestPracticeIgnoreRhosts ? g_desiredSshBestPracticeIgnoreRhosts : g_sshDefaultSshYes, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshLogLevelIsSet(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("loglevel", g_desiredSshLogLevelIsSet ? g_desiredSshLogLevelIsSet : g_sshDefaultSshLogLevel, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshMaxAuthTriesIsSet(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("maxauthtries", g_desiredSshMaxAuthTriesIsSet ? g_desiredSshMaxAuthTriesIsSet : g_sshDefaultSshMaxAuthTries, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureAllowUsersIsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("allowusers", g_desiredAllowUsersIsConfigured ? g_desiredAllowUsersIsConfigured : g_sshDefaultSshAllowUsers, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureDenyUsersIsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("denyusers", g_desiredDenyUsersIsConfigured ? g_desiredDenyUsersIsConfigured : g_sshDefaultSshDenyUsers, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureAllowGroupsIsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("allowgroups", g_desiredAllowGroupsIsConfigured ? g_desiredAllowGroupsIsConfigured : g_sshDefaultSshAllowGroups, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureDenyGroupsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("denygroups", g_desiredDenyGroupsConfigured ? g_desiredDenyGroupsConfigured : g_sshDefaultSshDenyGroups, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshHostbasedAuthenticationIsDisabled(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("hostbasedauthentication", g_desiredSshHostbasedAuthenticationIsDisabled ? g_desiredSshHostbasedAuthenticationIsDisabled : g_sshDefaultSshNo, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshPermitRootLoginIsDisabled(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("permitrootlogin", g_desiredSshPermitRootLoginIsDisabled ? g_desiredSshPermitRootLoginIsDisabled : g_sshDefaultSshNo, NULL, &reason, log);
    return reason;
}

static char* SshUtilsAuditEnsureSshPermitEmptyPasswordsIsDisabled(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("permitemptypasswords", g_desiredSshPermitEmptyPasswordsIsDisabled ? g_desiredSshPermitEmptyPasswordsIsDisabled : g_sshDefaultSshNo, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshClientIntervalCountMaxIsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("clientalivecountmax", g_desiredSshClientIntervalCountMaxIsConfigured ? g_desiredSshClientIntervalCountMaxIsConfigured : g_sshDefaultSshClientIntervalCountMax, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshClientAliveIntervalIsConfigured(void* log)
{
    char* reason = NULL;
    CheckSshClientAliveInterval(&reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshLoginGraceTimeIsSet(void* log)
{
    char* reason = NULL;
    CheckSshLoginGraceTime(g_desiredSshLoginGraceTimeIsSet ? g_desiredSshLoginGraceTimeIsSet : g_sshDefaultSshLoginGraceTime, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void* log)
{
    char* reason = NULL;
    CheckOnlyApprovedMacAlgorithmsAreUsed(g_desiredOnlyApprovedMacAlgorithmsAreUsed ? g_desiredOnlyApprovedMacAlgorithmsAreUsed : g_sshDefaultSshMacs, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureSshWarningBannerIsEnabled(void* log)
{
    const char* bannerFile = "/etc/azsec/banner.txt";
    char* reason = NULL;
    CheckSshWarningBanner(bannerFile, g_desiredSshWarningBannerIsEnabled ? g_desiredSshWarningBannerIsEnabled : g_sshDefaultSshBannerText, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureUsersCannotSetSshEnvironmentOptions(void* log)
{
    char* reason = NULL;
    CheckSshOptionIsSet("permituserenvironment", g_desiredUsersCannotSetSshEnvironmentOptions ? g_desiredUsersCannotSetSshEnvironmentOptions : g_sshDefaultSshNo, NULL, &reason, log);
    return reason;
}

char* SshUtilsAuditEnsureAppropriateCiphersForSsh(void* log)
{
    char* reason = NULL;
    CheckAppropriateCiphersForSsh(g_desiredAppropriateCiphersForSsh ? g_desiredAppropriateCiphersForSsh : g_sshDefaultSshCiphers, &reason, log);
    return reason;
}

int SshUtilsRemediateEnsurePermissionsOnEtcSshSshdConfig(char* value, void* log)
{
    FREE_MEMORY(g_desiredPermissionsOnEtcSshSshdConfig);
    return (NULL != (g_desiredPermissionsOnEtcSshSshdConfig = DuplicateString(value ? value : g_sshDefaultSshSshdConfigAccess))) ?
        SetFileAccess(g_etcSshSshdConfig, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig), log) : ENOMEM;
};

int SshUtilsRemediateEnsureSshBestPracticeProtocol(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshBestPracticeProtocol);
    return (NULL != (g_desiredSshBestPracticeProtocol = DuplicateString(value ? value : g_sshDefaultSshProtocol))) ?
        SetSshOption("Protocol", g_desiredSshBestPracticeProtocol, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshBestPracticeIgnoreRhosts(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshBestPracticeIgnoreRhosts);
    return (NULL != (g_desiredSshBestPracticeIgnoreRhosts = DuplicateString(value ? value : g_sshDefaultSshYes))) ?
        SetSshOption("IgnoreRhosts", value ? value : g_sshDefaultSshYes, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshLogLevelIsSet(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshLogLevelIsSet);
    return (NULL != (g_desiredSshLogLevelIsSet = DuplicateString(value ? value : g_sshDefaultSshLogLevel))) ?
        SetSshOption("LogLevel", g_desiredSshLogLevelIsSet, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshMaxAuthTriesIsSet(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshMaxAuthTriesIsSet);
    return (NULL != (g_desiredSshMaxAuthTriesIsSet = DuplicateString(value ? value : g_sshDefaultSshMaxAuthTries))) ?
        SetSshOption("MaxAuthTries", g_desiredSshMaxAuthTriesIsSet, log) : ENOMEM;
}

int SshUtilsRemediateEnsureAllowUsersIsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredAllowUsersIsConfigured);
    return (NULL != (g_desiredAllowUsersIsConfigured = DuplicateString(value ? value : g_sshDefaultSshAllowUsers))) ?
        SetSshOption("AllowUsers", g_desiredAllowUsersIsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureDenyUsersIsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredDenyUsersIsConfigured);
    return (NULL != (g_desiredDenyUsersIsConfigured = DuplicateString(value ? value : g_sshDefaultSshDenyUsers))) ?
        SetSshOption("DenyUsers", g_desiredDenyUsersIsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureAllowGroupsIsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredAllowGroupsIsConfigured);
    return (NULL != (g_desiredAllowGroupsIsConfigured = DuplicateString(value ? value : g_sshDefaultSshAllowGroups))) ?
        SetSshOption("AllowGroups", g_desiredAllowGroupsIsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureDenyGroupsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredDenyGroupsConfigured);
    return (NULL != (g_desiredDenyGroupsConfigured = DuplicateString(value ? value : g_sshDefaultSshDenyGroups))) ?
        SetSshOption("DenyGroups", g_desiredDenyGroupsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshHostbasedAuthenticationIsDisabled(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshHostbasedAuthenticationIsDisabled);
    return (NULL != (g_desiredSshHostbasedAuthenticationIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ?
        SetSshOption("HostBasedAuthentication", g_desiredSshHostbasedAuthenticationIsDisabled, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshPermitRootLoginIsDisabled(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshPermitRootLoginIsDisabled);
    return (NULL != (g_desiredSshPermitRootLoginIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ?
        SetSshOption("PermitRootLogin", g_desiredSshPermitRootLoginIsDisabled, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshPermitEmptyPasswordsIsDisabled(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshPermitEmptyPasswordsIsDisabled);
    return (NULL != (g_desiredSshPermitEmptyPasswordsIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ?
        SetSshOption("PermitEmptyPasswords", g_desiredSshPermitEmptyPasswordsIsDisabled, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshClientIntervalCountMaxIsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshClientIntervalCountMaxIsConfigured);
    return (NULL != (g_desiredSshClientIntervalCountMaxIsConfigured = DuplicateString(value ? value : g_sshDefaultSshClientIntervalCountMax))) ?
        SetSshOption("ClientAliveCountMax", g_desiredSshClientIntervalCountMaxIsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshClientAliveIntervalIsConfigured(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshClientAliveIntervalIsConfigured);
    return (NULL != (g_desiredSshClientAliveIntervalIsConfigured = DuplicateString(value ? value : g_sshDefaultSshClientAliveInterval))) ?
        SetSshOption("ClientAliveInterval", g_desiredSshClientAliveIntervalIsConfigured, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshLoginGraceTimeIsSet(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshLoginGraceTimeIsSet);
    return (NULL != (g_desiredSshLoginGraceTimeIsSet = DuplicateString(value ? value : g_sshDefaultSshLoginGraceTime))) ?
        SetSshOption("LoginGraceTime", g_desiredSshLoginGraceTimeIsSet, log) : ENOMEM;
}

int SshUtilsRemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, void* log)
{
    FREE_MEMORY(g_desiredOnlyApprovedMacAlgorithmsAreUsed);
    return (NULL != (g_desiredOnlyApprovedMacAlgorithmsAreUsed = DuplicateString(value ? value : g_sshDefaultSshMacs))) ?
        SetSshOption("MACs", g_desiredOnlyApprovedMacAlgorithmsAreUsed, log) : ENOMEM;
}

int SshUtilsRemediateEnsureSshWarningBannerIsEnabled(char* value, void* log)
{
    FREE_MEMORY(g_desiredSshWarningBannerIsEnabled);
    return (NULL != (g_desiredSshWarningBannerIsEnabled = DuplicateString(value ? value : g_sshDefaultSshBannerText))) ?
        SetSshWarningBanner(atoi(g_desiredPermissionsOnEtcSshSshdConfig), g_desiredSshWarningBannerIsEnabled, log) : ENOMEM;
}

int SshUtilsRemediateEnsureUsersCannotSetSshEnvironmentOptions(char* value, void* log)
{
    FREE_MEMORY(g_desiredUsersCannotSetSshEnvironmentOptions);
    return (NULL != (g_desiredUsersCannotSetSshEnvironmentOptions = DuplicateString(value ? value : g_sshDefaultSshNo))) ?
        SetSshOption("PermitUserEnvironment", g_desiredUsersCannotSetSshEnvironmentOptions, log) : ENOMEM;
}

int SshUtilsRemediateEnsureAppropriateCiphersForSsh(char* value, void* log)
{
    FREE_MEMORY(g_desiredAppropriateCiphersForSsh);
    return (NULL != (g_desiredAppropriateCiphersForSsh = DuplicateString(value ? value : g_sshDefaultSshCiphers))) ?
        (((0 == SetSshOption("Ciphers", g_desiredAppropriateCiphersForSsh, log)) &&
        RestartDaemon("sshd", log)) ? 0 : ENOENT) : ENOMEM;
}