// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "SshUtils.h"

static const char* g_sshServerService = "sshd";
static const char* g_sshServerConfiguration = "/etc/ssh/sshd_config";
static const char* g_osconfigRemediationConf = "/etc/ssh/sshd_config.d/osconfig_remediation.conf";
static const char* g_sshdConfigRemediationHeader = "# Azure OSConfig Remediation\nInclude /etc/ssh/sshd_config.d/osconfig_remediation.conf\n";

static const char* g_sshProtocol = "Protocol";
static const char* g_sshIgnoreHosts = "IgnoreRhosts";
static const char* g_sshLogLevel = "LogLevel";
static const char* g_sshMaxAuthTries = "MaxAuthTries";
static const char* g_sshAllowUsers = "AllowUsers";
static const char* g_sshDenyUsers = "DenyUsers";
static const char* g_sshAllowGroups = "AllowGroups";
static const char* g_sshDenyGroups = "DenyGroups";
static const char* g_sshHostBasedAuthentication = "HostBasedAuthentication";
static const char* g_sshPermitRootLogin = "PermitRootLogin";
static const char* g_sshPermitEmptyPasswords = "PermitEmptyPasswords";
static const char* g_sshClientAliveCountMax = "ClientAliveCountMax";
static const char* g_sshLoginGraceTime = "LoginGraceTime";
static const char* g_sshClientAliveInterval = "ClientAliveInterval";
static const char* g_sshMacs = "MACs";
static const char* g_sshPermitUserEnvironment = "PermitUserEnvironment";
static const char* g_sshBanner = "Banner";
static const char* g_sshCiphers = "Ciphers";

static const char* g_sshDefaultSshSshdConfigAccess = "600";
static const char* g_sshDefaultSshProtocol = "2";
static const char* g_sshDefaultSshYes = "yes";
static const char* g_sshDefaultSshNo = "no";
static const char* g_sshDefaultSshLogLevel = "INFO";
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
static const char* g_sshBannerFile = "/etc/azsec/banner.txt";
static const char* g_sshDefaultSshBannerText =
    "#######################################################################\n\n"
    "Authorized access only!\n\n"
    "If you are not authorized to access or use this system, disconnect now!\n\n"
    "#######################################################################\n";

static const char* g_auditEnsurePermissionsOnEtcSshSshdConfigObject = "auditEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_auditEnsureSshBestPracticeProtocolObject = "auditEnsureSshBestPracticeProtocol";
static const char* g_auditEnsureSshBestPracticeIgnoreRhostsObject = "auditEnsureSshBestPracticeIgnoreRhosts";
static const char* g_auditEnsureSshLogLevelIsSetObject = "auditEnsureSshLogLevelIsSet";
static const char* g_auditEnsureSshMaxAuthTriesIsSetObject = "auditEnsureSshMaxAuthTriesIsSet";
static const char* g_auditEnsureAllowUsersIsConfiguredObject = "auditEnsureAllowUsersIsConfigured";
static const char* g_auditEnsureDenyUsersIsConfiguredObject = "auditEnsureDenyUsersIsConfigured";
static const char* g_auditEnsureAllowGroupsIsConfiguredObject = "auditEnsureAllowGroupsIsConfigured";
static const char* g_auditEnsureDenyGroupsConfiguredObject = "auditEnsureDenyGroupsConfigured";
static const char* g_auditEnsureSshHostbasedAuthenticationIsDisabledObject = "auditEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_auditEnsureSshPermitRootLoginIsDisabledObject = "auditEnsureSshPermitRootLoginIsDisabled";
static const char* g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject = "auditEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject = "auditEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_auditEnsureSshClientAliveIntervalIsConfiguredObject = "auditEnsureSshClientAliveIntervalIsConfigured";
static const char* g_auditEnsureSshLoginGraceTimeIsSetObject = "auditEnsureSshLoginGraceTimeIsSet";
static const char* g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "auditEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_auditEnsureSshWarningBannerIsEnabledObject = "auditEnsureSshWarningBannerIsEnabled";
static const char* g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject = "auditEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_auditEnsureAppropriateCiphersForSshObject = "auditEnsureAppropriateCiphersForSsh";

static const char* g_remediateEnsurePermissionsOnEtcSshSshdConfigObject = "remediateEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_remediateEnsureSshBestPracticeProtocolObject = "remediateEnsureSshBestPracticeProtocol";
static const char* g_remediateEnsureSshBestPracticeIgnoreRhostsObject = "remediateEnsureSshBestPracticeIgnoreRhosts";
static const char* g_remediateEnsureSshLogLevelIsSetObject = "remediateEnsureSshLogLevelIsSet";
static const char* g_remediateEnsureSshMaxAuthTriesIsSetObject = "remediateEnsureSshMaxAuthTriesIsSet";
static const char* g_remediateEnsureAllowUsersIsConfiguredObject = "remediateEnsureAllowUsersIsConfigured";
static const char* g_remediateEnsureDenyUsersIsConfiguredObject = "remediateEnsureDenyUsersIsConfigured";
static const char* g_remediateEnsureAllowGroupsIsConfiguredObject = "remediateEnsureAllowGroupsIsConfigured";
static const char* g_remediateEnsureDenyGroupsConfiguredObject = "remediateEnsureDenyGroupsConfigured";
static const char* g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject = "remediateEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_remediateEnsureSshPermitRootLoginIsDisabledObject = "remediateEnsureSshPermitRootLoginIsDisabled";
static const char* g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject = "remediateEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject = "remediateEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_remediateEnsureSshClientAliveIntervalIsConfiguredObject = "remediateEnsureSshClientAliveIntervalIsConfigured";
static const char* g_remediateEnsureSshLoginGraceTimeIsSetObject = "remediateEnsureSshLoginGraceTimeIsSet";
static const char* g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "remediateEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_remediateEnsureSshWarningBannerIsEnabledObject = "remediateEnsureSshWarningBannerIsEnabled";
static const char* g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject = "remediateEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_remediateEnsureAppropriateCiphersForSshObject = "remediateEnsureAppropriateCiphersForSsh";

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

static bool g_auditOnlySession = true;

static char* GetSshServerState(const char* name, void* log)
{
    const char* sshdDashTCommand = "sshd -T";
    const char* commandTemplateForOne = "%s | grep  -m 1 %s";
    char* command = NULL;
    char* textResult = NULL;
    int status = 0;

    if (NULL == name)
    {
        if (0 != (status = ExecuteCommand(NULL, sshdDashTCommand, true, false, 0, 0, &textResult, NULL, NULL)))
        {
            OsConfigLogError(log, "GetSshServerState: '%s' failed with %d and '%s'", sshdDashTCommand, status, textResult);
            FREE_MEMORY(textResult);
        }
    }
    else
    {
        if (NULL != (command = FormatAllocateString(commandTemplateForOne, sshdDashTCommand, name)))
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
    char* sshMacs = DuplicateStringToLowercase(g_sshMacs);
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

    if (0 == status)
    {
        OsConfigCaptureSuccessReason(reason, "%sThe %s service reports that '%s' is set to '%s' (all approved MAC algorithms)", g_sshServerService, sshMacs, macsValue);
    }

    FREE_MEMORY(macsValue);
    FREE_MEMORY(sshMacs);

    OsConfigLogInfo(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckAppropriateCiphersForSsh(const char* ciphers, char** reason, void* log)
{
    char* sshCiphers = DuplicateStringToLowercase(g_sshCiphers);
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

    if (0 == status)
    {
        OsConfigCaptureSuccessReason(reason, "%sThe %s service reports that '%s' is set to '%s' (only approved ciphers)", g_sshServerService, sshCiphers, ciphersValue);
    }

    FREE_MEMORY(ciphersValue);
    FREE_MEMORY(sshCiphers);

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
        else
        {
            OsConfigCaptureSuccessReason(reason, "%sThe %s service reports that '%s' is set to '%s'", g_sshServerService, option, value);
        }

        if (NULL != actualValue)
        {
            *actualValue = DuplicateString(value);
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
    char* clientAliveInterval = DuplicateStringToLowercase(g_sshClientAliveInterval);
    int actualValue = 0;
    int status = 0; 
    
    if (0 == IsSshServerActive(log))
    {
        if (0 == (status = CheckSshOptionIsSetToInteger(clientAliveInterval, NULL, &actualValue, reason, log))) 
        {
            OsConfigResetReason(reason);

            if (actualValue > 0)
            {
                OsConfigCaptureSuccessReason(reason, "%sThe %s service reports that '%s' is set to '%d' (that is greater than zero)", g_sshServerService, clientAliveInterval, actualValue);
            }
            else
            {
                OsConfigLogError(log, "CheckSshClientAliveInterval: 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
                OsConfigCaptureReason(reason, "'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)",
                    "%s, also 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
                status = ENOENT;
            }
        }
    }

    FREE_MEMORY(clientAliveInterval);

    OsConfigLogInfo(log, "CheckSshClientAliveInterval: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshLoginGraceTime(const char* value, char** reason, void* log)
{
    char* loginGraceTime = DuplicateStringToLowercase(g_sshLoginGraceTime);
    int targetValue = atoi(value ? value : g_sshDefaultSshLoginGraceTime);
    int actualValue = 0;
    int status = 0; 

    if (0 == IsSshServerActive(log))
    {
        if (0 == (status = CheckSshOptionIsSetToInteger(loginGraceTime, NULL, &actualValue, reason, log)))
        {
            OsConfigResetReason(reason);

            if (actualValue <= targetValue)
            {
                OsConfigCaptureSuccessReason(reason, "%sThe %s service reports that '%s' is set to '%d' (that is %d or less)", g_sshServerService, loginGraceTime, targetValue, actualValue);
            }
            else
            {
                OsConfigLogError(log, "CheckSshLoginGraceTime: 'logingracetime' is not set to %d or less in SSH Server response (but to %d)", targetValue, actualValue);
                OsConfigCaptureReason(reason, "'logingracetime' is not set to a value of %d or less in SSH Server response (but to %d)",
                    "%s, also 'logingracetime' is not set to a value of %d or less in SSH Server response (but to %d)", targetValue, actualValue);
                status = ENOENT;
            }
        }
    }

    FREE_MEMORY(loginGraceTime);

    OsConfigLogInfo(log, "CheckSshLoginGraceTime: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckSshWarningBanner(const char* bannerFile, const char* bannerText, unsigned int desiredAccess, char** reason, void* log)
{
    char* banner = DuplicateStringToLowercase(g_sshBanner);
    char* actualValue = NULL;
    char* contents = NULL;
    int status = 0;

    if (0 == IsSshServerActive(log))
    {
        if ((NULL == bannerFile) || (NULL == bannerText))
        {
            OsConfigLogError(log, "CheckSshWarningBanner: invalid arguments");
            status = EINVAL;
        } 
        else if (0 == (status = CheckSshOptionIsSet(banner, bannerFile, &actualValue, reason, log)))
        {
            OsConfigResetReason(reason);

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
                OsConfigCaptureReason(reason, "Banner text from file '%s' is different from the expected text",
                    "%s, also the banner text from '%s' is different from the expected text", bannerFile);
                status = ENOENT;
            }
            else if (0 == (status = CheckFileAccess(bannerFile, 0, 0, desiredAccess, reason, log)))
            {
                OsConfigCaptureSuccessReason(reason, "%sThe sshd service reports that '%s' is set to '%s', this file has access '%u' and contains the expected banner text", 
                    banner, actualValue, desiredAccess);
            }
        }

        FREE_MEMORY(contents);
        FREE_MEMORY(actualValue);
    }

    FREE_MEMORY(banner);

    OsConfigLogInfo(log, "CheckSshWarningBanner: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

int CheckSshProtocol(char** reason, void* log)
{
    const char* protocolTemplate = "%s %s";
    char* protocol = NULL;
    int status = 0;

    if (0 != IsSshServerActive(log))
    {
        return status;
    }

    if (NULL == (protocol = FormatAllocateString(protocolTemplate, g_sshProtocol, g_desiredSshBestPracticeProtocol ? g_desiredSshBestPracticeProtocol : g_sshDefaultSshProtocol)))
    {
        OsConfigLogError(log, "CheckSshProtocol: FormatAllocateString failed");
        status = ENOMEM;
    }
    else if (false == FileExists(g_osconfigRemediationConf))
    {
        OsConfigLogError(log, "CheckSshProtocol: the OSConfig remediation file '%s' is not present on this device", g_osconfigRemediationConf);
        OsConfigCaptureReason(reason, "'%s' is not present on this device", "%s, also '%s' is not present on this device", g_osconfigRemediationConf);
        status = EEXIST;
    }
    else if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogError(log, "CheckSshProtocol: the SSH Server configuration file '%s' is not present on this device", g_sshServerConfiguration);
        OsConfigCaptureReason(reason, "'%s' is not present on this device", "%s, also '%s' is not present on this device", g_sshServerConfiguration);
        status = EEXIST;
    }
    else if (0 != FindTextInFile(g_sshServerConfiguration, g_sshdConfigRemediationHeader, log))
    {
        OsConfigLogError(log, "CheckSshProtocol: '%s' is not found included in '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
        OsConfigCaptureReason(reason, "'%s' is not found included in %s",
            "%s, also '%s' is not found included in %s", g_osconfigRemediationConf, g_sshServerConfiguration);
        status = ENOENT;
    }
    else if (EEXIST == (status = CheckLineNotFoundOrCommentedOut(g_osconfigRemediationConf, '#', protocol, log)))
    {
        OsConfigLogInfo(log, "CheckSshProtocol: '%s' is found uncommented in %s", protocol, g_sshServerConfiguration);
        OsConfigCaptureSuccessReason(reason, "%s'%s' is found uncommented in %s", protocol, g_sshServerConfiguration);
        status = 0;
    }
    else
    {
        OsConfigLogError(log, "CheckSshProtocol: '%s' is not found uncommented with '#' in %s", protocol, g_sshServerConfiguration);
        OsConfigCaptureReason(reason, "'%s' is not found uncommented with '#' in %s",  
            "%s, also '%s' is not found uncommented with '#' in %s", protocol, g_sshServerConfiguration);
        status = ENOENT;
    }

    FREE_MEMORY(protocol);

    OsConfigLogInfo(log, "CheckSshProtocol: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int SetSshWarningBanner(unsigned int desiredBannerFileAccess, const char* bannerText, void* log)
{
    const char* etcAzSec = "/etc/azsec/";
    int status = 0;

    if (NULL == bannerText)
    {
        OsConfigLogError(log, "SetSshWarningBanner: invalid argument");
        return EINVAL;
    }

    if (false == DirectoryExists(etcAzSec))
    {
        if (0 != mkdir(etcAzSec, desiredBannerFileAccess))
        {
            status = errno ? errno : ENOENT;
            OsConfigLogError(log, "SetSshWarningBanner: mkdir(%s, %u) failed with %d", etcAzSec, desiredBannerFileAccess, status);
        }
    }

    if (true == DirectoryExists(etcAzSec))
    {
        if (SavePayloadToFile(g_sshBannerFile, bannerText, strlen(bannerText), log))
        {
            if (0 != (status = SetFileAccess(g_sshBannerFile, 0, 0, desiredBannerFileAccess, log)))
            {
                OsConfigLogError(log, "SetSshWarningBanner: failed to set desired access %u on banner file %s (%d)", desiredBannerFileAccess, g_sshBannerFile, status);
            }
        }
        else
        {
            status = errno ? errno : ENOENT;
            OsConfigLogError(log, "SetSshWarningBanner: failed to save banner text '%s' to file '%s' with %d", bannerText, etcAzSec, status);
        }
    }

    return status;
}

static int IncludeRemediationSshConfFile(void* log)
{
    const char* etcSshSshdConfigD = "/etc/ssh/sshd_config.d";
    const char* configurationTemplate = "%s%s";
    
    int desiredAccess = atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess);
    char* originalConfiguration = NULL;
    char* newConfiguration = NULL;
    size_t newConfigurationSize = 0;
    int status = 0;

    if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogInfo(log, "IncludeRemediationSshConfFile: '%s' is not present on this device", g_sshServerConfiguration);
        return EEXIST;
    }

    if (false == DirectoryExists(etcSshSshdConfigD))
    {
        if (0 != mkdir(etcSshSshdConfigD, desiredAccess))
        {
            status = errno ? errno : ENOENT;
            OsConfigLogError(log, "IncludeRemediationSshConfFile: mkdir(%s, %u) failed with %d", etcSshSshdConfigD, desiredAccess, status);
        }
    }

    if (true == DirectoryExists(etcSshSshdConfigD))
    {
        if (NULL != (originalConfiguration = LoadStringFromFile(g_sshServerConfiguration, false, log)))
        {
            if (0 != strncmp(originalConfiguration, g_sshdConfigRemediationHeader, strlen(g_sshdConfigRemediationHeader)))
            {
                newConfigurationSize = strlen(configurationTemplate) + strlen(g_sshdConfigRemediationHeader) + strlen(originalConfiguration) + 1;

                if (NULL != (newConfiguration = malloc(newConfigurationSize)))
                {
                    memset(newConfiguration, 0, newConfigurationSize);
                    snprintf(newConfiguration, newConfigurationSize, configurationTemplate, g_sshdConfigRemediationHeader, originalConfiguration);

                    if (true == SavePayloadToFile(g_sshServerConfiguration, newConfiguration, newConfigurationSize, log))
                    {
                        OsConfigLogInfo(log, "IncludeRemediationSshConfFile: '%s' is now included by '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
                        status = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "IncludeRemediationSshConfFile: failed to include '%s' into '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
                        status = ENOENT;
                    }

                    FREE_MEMORY(newConfiguration);
                }
                else
                {
                    OsConfigLogError(log, "IncludeRemediationSshConfFile: out of memory, cannot include '%s' into '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
                    status = ENOMEM;
                }
            }
            else
            {
                OsConfigLogInfo(log, "IncludeRemediationSshConfFile: '%s' is already included by '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
                status = 0;
            }

            SetFileAccess(g_sshServerConfiguration, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), log);

            FREE_MEMORY(originalConfiguration);
        }
        else
        {
            OsConfigLogError(log, "IncludeRemediationSshConfFile: failed to read from '%s'", g_sshServerConfiguration);
            status = EEXIST;
        }
    }

    return status;
}

static int SaveRemediationSshConfFile(void* log)
{
    // 'UsePAM yes' blocks Ciphers and MACs so we need also to set this to 'no' here as other .conf files can set it
    const char* confFileTemplate = "%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\nUsePAM no";

    char* newRemediation = NULL;
    char* currentRemediation = NULL;
    size_t newRemediationSize = 0;
    int status = 0;

    newRemediationSize = strlen(confFileTemplate) +
        strlen(g_sshProtocol) + strlen(g_desiredSshBestPracticeProtocol ? g_desiredSshBestPracticeProtocol : g_sshDefaultSshProtocol) +
        strlen(g_sshIgnoreHosts) + strlen(g_desiredSshBestPracticeIgnoreRhosts ? g_desiredSshBestPracticeIgnoreRhosts : g_sshDefaultSshYes) +
        strlen(g_sshLogLevel) + strlen(g_desiredSshLogLevelIsSet ? g_desiredSshLogLevelIsSet : g_sshDefaultSshLogLevel) +
        strlen(g_sshMaxAuthTries) + strlen(g_desiredSshMaxAuthTriesIsSet ? g_desiredSshMaxAuthTriesIsSet : g_sshDefaultSshMaxAuthTries) +
        strlen(g_sshAllowUsers) + strlen(g_desiredAllowUsersIsConfigured ? g_desiredAllowUsersIsConfigured : g_sshDefaultSshAllowUsers) +
        strlen(g_sshDenyUsers) + strlen(g_desiredDenyUsersIsConfigured ? g_desiredDenyUsersIsConfigured : g_sshDefaultSshDenyUsers) +
        strlen(g_sshAllowGroups) + strlen(g_desiredAllowGroupsIsConfigured ? g_desiredAllowGroupsIsConfigured : g_sshDefaultSshAllowGroups) +
        strlen(g_sshDenyGroups) + strlen(g_desiredDenyGroupsConfigured ? g_desiredDenyGroupsConfigured : g_sshDefaultSshDenyGroups) +
        strlen(g_sshHostBasedAuthentication) + strlen(g_desiredSshHostbasedAuthenticationIsDisabled ? g_desiredSshHostbasedAuthenticationIsDisabled : g_sshDefaultSshNo) +
        strlen(g_sshPermitRootLogin) + strlen(g_desiredSshPermitRootLoginIsDisabled ? g_desiredSshPermitRootLoginIsDisabled : g_sshDefaultSshNo) +
        strlen(g_sshPermitEmptyPasswords) + strlen(g_desiredSshPermitEmptyPasswordsIsDisabled ? g_desiredSshPermitEmptyPasswordsIsDisabled : g_sshDefaultSshNo) +
        strlen(g_sshClientAliveCountMax) + strlen(g_desiredSshClientIntervalCountMaxIsConfigured ? g_desiredSshClientIntervalCountMaxIsConfigured : g_sshDefaultSshClientIntervalCountMax) +
        strlen(g_sshClientAliveInterval) + strlen(g_desiredSshClientAliveIntervalIsConfigured ? g_desiredSshClientAliveIntervalIsConfigured : g_sshDefaultSshClientAliveInterval) +
        strlen(g_sshLoginGraceTime) + strlen(g_desiredSshLoginGraceTimeIsSet ? g_desiredSshLoginGraceTimeIsSet : g_sshDefaultSshLoginGraceTime) +
        strlen(g_sshPermitUserEnvironment) + strlen(g_desiredUsersCannotSetSshEnvironmentOptions ? g_desiredUsersCannotSetSshEnvironmentOptions : g_sshDefaultSshNo) +
        strlen(g_sshBanner) + strlen(g_sshBannerFile) +
        strlen(g_sshMacs) + strlen(g_desiredOnlyApprovedMacAlgorithmsAreUsed ? g_desiredOnlyApprovedMacAlgorithmsAreUsed : g_sshDefaultSshMacs) +
        strlen(g_sshCiphers) + strlen(g_desiredAppropriateCiphersForSsh ? g_desiredAppropriateCiphersForSsh : g_sshDefaultSshCiphers) + 1;

    if (NULL != (newRemediation = malloc(newRemediationSize)))
    {
        memset(newRemediation, 0, newRemediationSize);
        snprintf(newRemediation, newRemediationSize, confFileTemplate,
            g_sshProtocol, g_desiredSshBestPracticeProtocol ? g_desiredSshBestPracticeProtocol : g_sshDefaultSshProtocol,
            g_sshIgnoreHosts, g_desiredSshBestPracticeIgnoreRhosts ? g_desiredSshBestPracticeIgnoreRhosts : g_sshDefaultSshYes,
            g_sshLogLevel, g_desiredSshLogLevelIsSet ? g_desiredSshLogLevelIsSet : g_sshDefaultSshLogLevel,
            g_sshMaxAuthTries, g_desiredSshMaxAuthTriesIsSet ? g_desiredSshMaxAuthTriesIsSet : g_sshDefaultSshMaxAuthTries,
            g_sshAllowUsers, g_desiredAllowUsersIsConfigured ? g_desiredAllowUsersIsConfigured : g_sshDefaultSshAllowUsers,
            g_sshDenyUsers, g_desiredDenyUsersIsConfigured ? g_desiredDenyUsersIsConfigured : g_sshDefaultSshDenyUsers,
            g_sshAllowGroups, g_desiredAllowGroupsIsConfigured ? g_desiredAllowGroupsIsConfigured : g_sshDefaultSshAllowGroups,
            g_sshDenyGroups, g_desiredDenyGroupsConfigured ? g_desiredDenyGroupsConfigured : g_sshDefaultSshDenyGroups,
            g_sshHostBasedAuthentication, g_desiredSshHostbasedAuthenticationIsDisabled ? g_desiredSshHostbasedAuthenticationIsDisabled : g_sshDefaultSshNo,
            g_sshPermitRootLogin, g_desiredSshPermitRootLoginIsDisabled ? g_desiredSshPermitRootLoginIsDisabled : g_sshDefaultSshNo,
            g_sshPermitEmptyPasswords, g_desiredSshPermitEmptyPasswordsIsDisabled ? g_desiredSshPermitEmptyPasswordsIsDisabled : g_sshDefaultSshNo,
            g_sshClientAliveCountMax, g_desiredSshClientIntervalCountMaxIsConfigured ? g_desiredSshClientIntervalCountMaxIsConfigured : g_sshDefaultSshClientIntervalCountMax,
            g_sshClientAliveInterval, g_desiredSshClientAliveIntervalIsConfigured ? g_desiredSshClientAliveIntervalIsConfigured : g_sshDefaultSshClientAliveInterval,
            g_sshLoginGraceTime, g_desiredSshLoginGraceTimeIsSet ? g_desiredSshLoginGraceTimeIsSet : g_sshDefaultSshLoginGraceTime,
            g_sshPermitUserEnvironment, g_desiredUsersCannotSetSshEnvironmentOptions ? g_desiredUsersCannotSetSshEnvironmentOptions : g_sshDefaultSshNo,
            g_sshBanner, g_sshBannerFile,
            g_sshMacs, g_desiredOnlyApprovedMacAlgorithmsAreUsed ? g_desiredOnlyApprovedMacAlgorithmsAreUsed : g_sshDefaultSshMacs,
            g_sshCiphers, g_desiredAppropriateCiphersForSsh ? g_desiredAppropriateCiphersForSsh : g_sshDefaultSshCiphers);

        if ((NULL != (currentRemediation = LoadStringFromFile(g_osconfigRemediationConf, false, log))) && (0 == strncmp(currentRemediation, newRemediation, strlen(newRemediation))))
        {
            OsConfigLogInfo(log, "SaveRemediationSshConfFile: '%s' already contains the correct remediation values:\n---\n%s\n---\n", g_osconfigRemediationConf, newRemediation);
            status = 0;
        }
        else
        {
            if (true == SavePayloadToFile(g_osconfigRemediationConf, newRemediation, newRemediationSize, log))
            {
                OsConfigLogInfo(log, "SaveRemediationSshConfFile: '%s' is now updated to the following remediation values:\n---\n%s\n---\n", g_osconfigRemediationConf, newRemediation);
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "SaveRemediationSshConfFile: failed to save remediation values to '%s'", g_osconfigRemediationConf);
                status = ENOENT;
            }
        }

        FREE_MEMORY(newRemediation);
        FREE_MEMORY(currentRemediation);
    }
    else
    {
        OsConfigLogError(log, "SaveRemediationSshConfFile: out of memory, cannot save remediation values to '%s'", g_osconfigRemediationConf);
        status = ENOMEM;
    }

    SetFileAccess(g_osconfigRemediationConf, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), log);

    return status;
}

int InitializeSshAudit(void* log)
{
    int status = 0;

    g_auditOnlySession = true;

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

void SshAuditCleanup(void* log)
{
    OsConfigLogInfo(log, "SshAuditCleanup: %s", g_auditOnlySession ? "audit only" : "audit and remediate");
    
    if (false == g_auditOnlySession)
    {
        // Even if we cannot include the remediation .conf file, we still want to go ahead and create/update it
        IncludeRemediationSshConfFile(log);

        if (0 == SaveRemediationSshConfFile(log))
        {
            // Signal to the SSH Server service to reload configuration
            RestartDaemon(g_sshServerService, log);
        }
    }
    
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

    g_auditOnlySession = true;
}

int ProcessSshAuditCheck(const char* name, char* value, char** reason, void* log)
{
    char* lowercase = NULL;
    int status = 0;

    if (NULL == name)
    {
        OsConfigLogError(log, "ProcessSshAuditCheck: invalid check name argument");
        return EINVAL;
    }

    OsConfigResetReason(reason);

    if (0 == strcmp(name, g_auditEnsurePermissionsOnEtcSshSshdConfigObject))
    {
        CheckFileAccess(g_sshServerConfiguration, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ? 
            g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshBestPracticeProtocolObject))
    {
        CheckSshProtocol(reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshBestPracticeIgnoreRhostsObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshIgnoreHosts);
        CheckSshOptionIsSet(lowercase, g_desiredSshBestPracticeIgnoreRhosts ? g_desiredSshBestPracticeIgnoreRhosts : g_sshDefaultSshYes, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshLogLevelIsSetObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshLogLevel);
        CheckSshOptionIsSet(lowercase, g_desiredSshLogLevelIsSet ? g_desiredSshLogLevelIsSet : g_sshDefaultSshLogLevel, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshMaxAuthTriesIsSetObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshMaxAuthTries);
        CheckSshOptionIsSet(lowercase, g_desiredSshMaxAuthTriesIsSet ? g_desiredSshMaxAuthTriesIsSet : g_sshDefaultSshMaxAuthTries, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureAllowUsersIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshAllowUsers);
        CheckSshOptionIsSet(lowercase, g_desiredAllowUsersIsConfigured ? g_desiredAllowUsersIsConfigured : g_sshDefaultSshAllowUsers, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureDenyUsersIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshDenyUsers);
        CheckSshOptionIsSet(lowercase, g_desiredDenyUsersIsConfigured ? g_desiredDenyUsersIsConfigured : g_sshDefaultSshDenyUsers, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureAllowGroupsIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshAllowGroups);
        CheckSshOptionIsSet(lowercase, g_desiredAllowGroupsIsConfigured ? g_desiredAllowGroupsIsConfigured : g_sshDefaultSshAllowGroups, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureDenyGroupsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshDenyGroups);
        CheckSshOptionIsSet(lowercase, g_desiredDenyGroupsConfigured ? g_desiredDenyGroupsConfigured : g_sshDefaultSshDenyGroups, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshHostbasedAuthenticationIsDisabledObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshHostBasedAuthentication);
        CheckSshOptionIsSet(lowercase, g_desiredSshHostbasedAuthenticationIsDisabled ? g_desiredSshHostbasedAuthenticationIsDisabled : g_sshDefaultSshNo, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshPermitRootLoginIsDisabledObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshPermitRootLogin);
        CheckSshOptionIsSet(lowercase, g_desiredSshPermitRootLoginIsDisabled ? g_desiredSshPermitRootLoginIsDisabled : g_sshDefaultSshNo, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshPermitEmptyPasswordsIsDisabledObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshPermitEmptyPasswords);
        CheckSshOptionIsSet(lowercase, g_desiredSshPermitEmptyPasswordsIsDisabled ? g_desiredSshPermitEmptyPasswordsIsDisabled : g_sshDefaultSshNo, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshClientIntervalCountMaxIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshClientAliveCountMax);
        CheckSshOptionIsSet(lowercase, g_desiredSshClientIntervalCountMaxIsConfigured ? g_desiredSshClientIntervalCountMaxIsConfigured : g_sshDefaultSshClientIntervalCountMax, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshClientAliveIntervalIsConfiguredObject))
    {
        CheckSshClientAliveInterval(reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshLoginGraceTimeIsSetObject))
    {
        CheckSshLoginGraceTime(g_desiredSshLoginGraceTimeIsSet ? g_desiredSshLoginGraceTimeIsSet : g_sshDefaultSshLoginGraceTime, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
    {
        CheckOnlyApprovedMacAlgorithmsAreUsed(g_desiredOnlyApprovedMacAlgorithmsAreUsed ? g_desiredOnlyApprovedMacAlgorithmsAreUsed : g_sshDefaultSshMacs, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureSshWarningBannerIsEnabledObject))
    {
        CheckSshWarningBanner(g_sshBannerFile, g_desiredSshWarningBannerIsEnabled ? g_desiredSshWarningBannerIsEnabled : g_sshDefaultSshBannerText, 
            atoi(g_desiredPermissionsOnEtcSshSshdConfig), reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureUsersCannotSetSshEnvironmentOptionsObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshPermitUserEnvironment);
        CheckSshOptionIsSet(lowercase, g_desiredUsersCannotSetSshEnvironmentOptions ? g_desiredUsersCannotSetSshEnvironmentOptions : g_sshDefaultSshNo, NULL, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureAppropriateCiphersForSshObject))
    {
        CheckAppropriateCiphersForSsh(g_desiredAppropriateCiphersForSsh ? g_desiredAppropriateCiphersForSsh : g_sshDefaultSshCiphers, reason, log);
    }
    else if (0 == strcmp(name, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject))
    {
        FREE_MEMORY(g_desiredPermissionsOnEtcSshSshdConfig);
        status = (NULL != (g_desiredPermissionsOnEtcSshSshdConfig = DuplicateString(value ? value : g_sshDefaultSshSshdConfigAccess))) ?
            SetFileAccess(g_sshServerConfiguration, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig), log) : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshBestPracticeProtocolObject))
    {
        FREE_MEMORY(g_desiredSshBestPracticeProtocol);
        status = (NULL != (g_desiredSshBestPracticeProtocol = DuplicateString(value ? value : g_sshDefaultSshProtocol))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshBestPracticeIgnoreRhostsObject))
    {
        FREE_MEMORY(g_desiredSshBestPracticeIgnoreRhosts);
        status = (NULL != (g_desiredSshBestPracticeIgnoreRhosts = DuplicateString(value ? value : g_sshDefaultSshYes))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshLogLevelIsSetObject))
    {
        FREE_MEMORY(g_desiredSshLogLevelIsSet);
        status = (NULL != (g_desiredSshLogLevelIsSet = DuplicateString(value ? value : g_sshDefaultSshLogLevel))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshMaxAuthTriesIsSetObject))
    {
        FREE_MEMORY(g_desiredSshMaxAuthTriesIsSet);
        status = (NULL != (g_desiredSshMaxAuthTriesIsSet = DuplicateString(value ? value : g_sshDefaultSshMaxAuthTries))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureAllowUsersIsConfiguredObject))
    {
        FREE_MEMORY(g_desiredAllowUsersIsConfigured);
        status = (NULL != (g_desiredAllowUsersIsConfigured = DuplicateString(value ? value : g_sshDefaultSshAllowUsers))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureDenyUsersIsConfiguredObject))
    {
        FREE_MEMORY(g_desiredDenyUsersIsConfigured);
        status = (NULL != (g_desiredDenyUsersIsConfigured = DuplicateString(value ? value : g_sshDefaultSshDenyUsers))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureAllowGroupsIsConfiguredObject))
    {
        FREE_MEMORY(g_desiredAllowGroupsIsConfigured);
        status = (NULL != (g_desiredAllowGroupsIsConfigured = DuplicateString(value ? value : g_sshDefaultSshAllowGroups))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureDenyGroupsConfiguredObject))
    {
        FREE_MEMORY(g_desiredDenyGroupsConfigured);
        status = (NULL != (g_desiredDenyGroupsConfigured = DuplicateString(value ? value : g_sshDefaultSshDenyGroups))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject))
    {
        FREE_MEMORY(g_desiredSshHostbasedAuthenticationIsDisabled);
        status = (NULL != (g_desiredSshHostbasedAuthenticationIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ?  0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshPermitRootLoginIsDisabledObject))
    {
        FREE_MEMORY(g_desiredSshPermitRootLoginIsDisabled);
        status = (NULL != (g_desiredSshPermitRootLoginIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject))
    {
        FREE_MEMORY(g_desiredSshPermitEmptyPasswordsIsDisabled);
        status = (NULL != (g_desiredSshPermitEmptyPasswordsIsDisabled = DuplicateString(value ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject))
    {
        FREE_MEMORY(g_desiredSshClientIntervalCountMaxIsConfigured);
        status = (NULL != (g_desiredSshClientIntervalCountMaxIsConfigured = DuplicateString(value ? value : g_sshDefaultSshClientIntervalCountMax))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshClientAliveIntervalIsConfiguredObject))
    {
        FREE_MEMORY(g_desiredSshClientAliveIntervalIsConfigured);
        status = (NULL != (g_desiredSshClientAliveIntervalIsConfigured = DuplicateString(value ? value : g_sshDefaultSshClientAliveInterval))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshLoginGraceTimeIsSetObject))
    {
        FREE_MEMORY(g_desiredSshLoginGraceTimeIsSet);
        status = (NULL != (g_desiredSshLoginGraceTimeIsSet = DuplicateString(value ? value : g_sshDefaultSshLoginGraceTime))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject))
    {
        FREE_MEMORY(g_desiredOnlyApprovedMacAlgorithmsAreUsed);
        status = (NULL != (g_desiredOnlyApprovedMacAlgorithmsAreUsed = DuplicateString(value ? value : g_sshDefaultSshMacs))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureSshWarningBannerIsEnabledObject))
    {
        FREE_MEMORY(g_desiredSshWarningBannerIsEnabled);
        status = (NULL != (g_desiredSshWarningBannerIsEnabled = DuplicateString(value ? value : g_sshDefaultSshBannerText))) ?
            SetSshWarningBanner(atoi(g_desiredPermissionsOnEtcSshSshdConfig), g_desiredSshWarningBannerIsEnabled, log) : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject))
    {
        FREE_MEMORY(g_desiredUsersCannotSetSshEnvironmentOptions);
        status = (NULL != (g_desiredUsersCannotSetSshEnvironmentOptions = DuplicateString(value ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if (0 == strcmp(name, g_remediateEnsureAppropriateCiphersForSshObject))
    {
        FREE_MEMORY(g_desiredAppropriateCiphersForSsh);
        status = (NULL != (g_desiredAppropriateCiphersForSsh = DuplicateString(value ? value : g_sshDefaultSshCiphers))) ? 0 : ENOMEM;
    }
    else
    {
        OsConfigLogError(log, "ProcessSshAuditCheck: unsupported check name '%s'", name);
        status = EINVAL;
    }

    FREE_MEMORY(lowercase);

    if ((NULL != reason) && (NULL == *reason))
    {
        if (0 != IsSshServerActive(log))
        {
            OsConfigCaptureSuccessReason(reason, "%s%s not found, nothing to check", g_sshServerService);
        }
        else
        {
            OsConfigLogError(log, "ProcessSshAuditCheck(%s): audit failure without a reason", name);
            if (NULL == (*reason = DuplicateString(SECURITY_AUDIT_FAIL)))
            {
                OsConfigLogError(log, "ProcessSshAuditCheck: DuplicateString failed");
                status = ENOMEM;
            }
        }
    }
    else if ((NULL != value) && (NULL == reason))
    {
        g_auditOnlySession = false;
    }

    OsConfigLogInfo(log, "ProcessSshAuditCheck(%s, '%s'): '%s' and %d", name, value ? value : "", (NULL != reason) ? *reason : "", status);

    return status;
}