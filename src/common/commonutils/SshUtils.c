// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "SshUtils.h"

static const char* g_sshServerService = "sshd";
static const char* g_sshServerConfiguration = "/etc/ssh/sshd_config";
static const char* g_sshServerConfigurationBackup = "/etc/ssh/sshd_config.bak";
static const char* g_osconfigRemediationConf = "/etc/ssh/sshd_config.d/osconfig_remediation.conf";
static const char* g_sshdConfigRemediationHeader = "# Azure OSConfig Remediation";

static const char* g_sshPort = "Port";
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
static const char* g_sshDefaultSshPort = "22";
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
static const char* g_auditEnsureSshPortIsConfiguredObject = "auditEnsureSshPortIsConfigured";
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
static const char* g_remediateEnsureSshPortIsConfiguredObject = "remediateEnsureSshPortIsConfigured";
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

static const char* g_initEnsurePermissionsOnEtcSshSshdConfigObject = "initEnsurePermissionsOnEtcSshSshdConfig";
static const char* g_initEnsureSshPortIsConfiguredObject = "initEnsureSshPortIsConfigured";
static const char* g_initEnsureSshBestPracticeProtocolObject = "initEnsureSshBestPracticeProtocol";
static const char* g_initEnsureSshBestPracticeIgnoreRhostsObject = "initEnsureSshBestPracticeIgnoreRhosts";
static const char* g_initEnsureSshLogLevelIsSetObject = "initEnsureSshLogLevelIsSet";
static const char* g_initEnsureSshMaxAuthTriesIsSetObject = "initEnsureSshMaxAuthTriesIsSet";
static const char* g_initEnsureAllowUsersIsConfiguredObject = "initEnsureAllowUsersIsConfigured";
static const char* g_initEnsureDenyUsersIsConfiguredObject = "initEnsureDenyUsersIsConfigured";
static const char* g_initEnsureAllowGroupsIsConfiguredObject = "initEnsureAllowGroupsIsConfigured";
static const char* g_initEnsureDenyGroupsConfiguredObject = "initEnsureDenyGroupsConfigured";
static const char* g_initEnsureSshHostbasedAuthenticationIsDisabledObject = "initEnsureSshHostbasedAuthenticationIsDisabled";
static const char* g_initEnsureSshPermitRootLoginIsDisabledObject = "initEnsureSshPermitRootLoginIsDisabled";
static const char* g_initEnsureSshPermitEmptyPasswordsIsDisabledObject = "initEnsureSshPermitEmptyPasswordsIsDisabled";
static const char* g_initEnsureSshClientIntervalCountMaxIsConfiguredObject = "initEnsureSshClientIntervalCountMaxIsConfigured";
static const char* g_initEnsureSshClientAliveIntervalIsConfiguredObject = "initEnsureSshClientAliveIntervalIsConfigured";
static const char* g_initEnsureSshLoginGraceTimeIsSetObject = "initEnsureSshLoginGraceTimeIsSet";
static const char* g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject = "initEnsureOnlyApprovedMacAlgorithmsAreUsed";
static const char* g_initEnsureSshWarningBannerIsEnabledObject = "initEnsureSshWarningBannerIsEnabled";
static const char* g_initEnsureUsersCannotSetSshEnvironmentOptionsObject = "initEnsureUsersCannotSetSshEnvironmentOptions";
static const char* g_initEnsureAppropriateCiphersForSshObject = "initEnsureAppropriateCiphersForSsh";

static char* g_desiredPermissionsOnEtcSshSshdConfig = NULL;
static char* g_desiredSshPort = NULL;
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
    const char* commandTemplateForOne = "%s | grep  -m 1 -w %s";
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

// SSH servers that implement OpenSSH version 8.2 or newer support Include
// See https://www.openssh.com/txt/release-8.2, quote: "add an Include sshd_config keyword that allows including additional configuration files"
static int IsSshConfigIncludeSupported(void* log)
{
    const char* expectedPrefix = "unknown option -- V OpenSSH_";
    const char* command = "sshd -V";
    const int minVersionMajor = 8;
    const int minVersionMinor = 2;
    char* textResult = NULL;
    size_t textResultLength = 0;
    size_t textPrefixLength = 0;
    char* textCursor = NULL;
    char versionMajorString[2] = {0};
    char versionMinorString[2] = {0};
    int versionMajor = 0;
    int versionMinor = 0;
    int result = 0;

    if (false == IsDaemonActive(g_sshServerService, log))
    {
        OsConfigLogInfo(log, "IsSshConfigIncludeSupported: the OpenSSH Server service '%s' is not active on this device, assuming Include is not supported", g_sshServerService);
        return EEXIST;
    }

    // Extract the two version digits from the SSH server response to unsupported command -V (the OpenSSH way)
    // For example, version 8.9 from response "unknown option -- V OpenSSH_8.9p1..."

    ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, NULL);

    if (NULL != textResult)
    {
        if (((textPrefixLength = strlen(expectedPrefix)) + 3) < (textResultLength = strlen(textResult)))
        {
            textCursor = textResult + strlen(expectedPrefix) + 1;
            if (isdigit(textCursor[0]) && ('.' == textCursor[1]) && isdigit(textCursor[2]))
            {
                versionMajorString[0] = textCursor[0];
                versionMinorString[0] = textCursor[2];
                versionMajor = atoi(versionMajorString);
                versionMinor = atoi(versionMinorString);
            }

            if ((versionMajor >= minVersionMajor) && (versionMinor >= minVersionMinor))
            {
                OsConfigLogInfo(log, "IsSshConfigIncludeSupported: %s reports OpenSSH version %d.%d (%d.%d or newer) and appears to support Include",
                    g_sshServerService, versionMajor, versionMinor, minVersionMajor, minVersionMinor);
                result = 0;
            }
            else
            {
                OsConfigLogInfo(log, "IsSshConfigIncludeSupported: %s reports OpenSSH version %d.%d (older than %d.%d) and appears to not support Include",
                    g_sshServerService, versionMajor, versionMinor, minVersionMajor, minVersionMinor);
                result = ENOENT;
            }
        }
        else
        {
            OsConfigLogInfo(log, "IsSshConfigIncludeSupported: unexpected response to '%s' ('%s'), assuming Include is not supported", command, textResult);
            result = ENOENT;
        }
    }
    else
    {
        OsConfigLogInfo(log, "IsSshConfigIncludeSupported: unexpected response to '%s', assuming Include is not supported", command);
        result = ENOENT;
    }

    FREE_MEMORY(textResult);

    return result;
}

static int CheckOnlyApprovedMacAlgorithmsAreUsed(const char* macs, char** reason, void* log)
{
    char* sshMacs = NULL;
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
    else if (NULL == (sshMacs = DuplicateStringToLowercase(g_sshMacs)))
    {
        OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: failed to duplicate string to lowercase");
        return ENOMEM;
    }

    if (NULL == (macsValue = GetSshServerState(sshMacs, log)))
    {
        OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: '%s' not found in SSH Server response from 'sshd -T'", sshMacs);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", sshMacs);
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
                    OsConfigCaptureReason(reason, "'%s' MAC algorithm found in SSH Server response is unapproved", value);
                }

                i += strlen(value);
                FREE_MEMORY(value);
                continue;
            }
        }
    }

    if (0 == status)
    {
        OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%s' (all approved MAC algorithms)", g_sshServerService, sshMacs, macsValue);
    }

    FREE_MEMORY(macsValue);
    FREE_MEMORY(sshMacs);

    OsConfigLogInfo(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckAppropriateCiphersForSsh(const char* ciphers, char** reason, void* log)
{
    char* sshCiphers = NULL;
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
    else if (NULL == (sshCiphers = DuplicateStringToLowercase(g_sshCiphers)))
    {
        OsConfigLogError(log, "CheckAppropriateCiphersForSsh: failed to duplicate string to lowercase");
        return ENOMEM;
    }

    if (NULL == (ciphersValue = GetSshServerState(sshCiphers, log)))
    {
        OsConfigLogError(log, "CheckAppropriateCiphersForSsh: '%s' not found in SSH Server response", sshCiphers);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", sshCiphers);
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
                    OsConfigCaptureReason(reason, "Cipher '%s' found in SSH Server response is unapproved", value);
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
                    OsConfigCaptureReason(reason, "Cipher '%s' is required and is not found in SSH Server response", &(ciphers[i]));
                }

                i += strlen(value);
                FREE_MEMORY(value);
                continue;
            }
        }
    }

    if (0 == status)
    {
        OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%s' (only approved ciphers)", g_sshServerService, sshCiphers, ciphersValue);
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
            OsConfigCaptureReason(reason, "'%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, value);
            status = ENOENT;
        }
        else
        {
            OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%s'", g_sshServerService, option, value);
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
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", option);
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
                OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%d' (that is greater than zero)", g_sshServerService, clientAliveInterval, actualValue);
            }
            else
            {
                OsConfigLogError(log, "CheckSshClientAliveInterval: 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
                OsConfigCaptureReason(reason, "'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
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
                OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%d' (that is %d or less)", g_sshServerService, loginGraceTime, targetValue, actualValue);
            }
            else
            {
                OsConfigLogError(log, "CheckSshLoginGraceTime: 'logingracetime' is not set to %d or less in SSH Server response (but to %d)", targetValue, actualValue);
                OsConfigCaptureReason(reason, "'logingracetime' is not set to a value of %d or less in SSH Server response (but to %d)", targetValue, actualValue);
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
                OsConfigCaptureReason(reason, "'%s' is set to '%s' but the file cannot be read", banner, actualValue);
                status = ENOENT;
            }
            else  if (0 != strcmp(contents, bannerText))
            {
                OsConfigLogError(log, "CheckSshWarningBanner: banner text is:\n%s instead of:\n%s", contents, bannerText);
                OsConfigCaptureReason(reason, "Banner text from file '%s' is different from the expected text", bannerFile);
                status = ENOENT;
            }
            else if (0 == (status = CheckFileAccess(bannerFile, 0, 0, desiredAccess, reason, log)))
            {
                OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%s', this file has access '%u' and contains the expected banner text",
                    g_sshServerService, banner, actualValue, desiredAccess);
            }
        }

        FREE_MEMORY(contents);
        FREE_MEMORY(actualValue);
    }

    FREE_MEMORY(banner);

    OsConfigLogInfo(log, "CheckSshWarningBanner: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static char* FormatInclusionForRemediation(void* log)
{
    const char* inclusionTemplate = "%s\nInclude %s\n";
    char* inclusion = NULL;
    size_t inclusionSize = 0;

    inclusionSize = strlen(inclusionTemplate) + strlen(g_sshdConfigRemediationHeader) + strlen(g_osconfigRemediationConf) + 1;
    if (NULL != (inclusion = malloc(inclusionSize)))
    {
        memset(inclusion, 0, inclusionSize);
        snprintf(inclusion, inclusionSize, inclusionTemplate, g_sshdConfigRemediationHeader, g_osconfigRemediationConf);
    }
    else
    {
        OsConfigLogError(log, "FormatInclusionForRemediation: out of memory");
    }

    return inclusion;
}

int CheckSshProtocol(char** reason, void* log)
{
    const char* protocolTemplate = "%s %s";
    char* protocol = NULL;
    char* inclusion = NULL;
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
    else if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogError(log, "CheckSshProtocol: the SSH Server configuration file '%s' is not present on this device", g_sshServerConfiguration);
        OsConfigCaptureReason(reason, "'%s' is not present on this device", g_sshServerConfiguration);
        status = EEXIST;
    }
    if (0 == (status = CheckLineFoundNotCommentedOut(g_sshServerConfiguration, '#', protocol, reason, log)))
    {
        OsConfigLogInfo(log, "CheckSshProtocol: '%s' is found uncommented in %s", protocol, g_sshServerConfiguration);
        status = 0;
    }
    else
    {
        OsConfigLogError(log, "CheckSshProtocol: '%s' is not found uncommented with '#' in %s", protocol, g_sshServerConfiguration);
        status = ENOENT;

        if (0 == IsSshConfigIncludeSupported(log))
        {
            if (false == FileExists(g_osconfigRemediationConf))
            {
                OsConfigLogError(log, "CheckSshProtocol: the OSConfig remediation file '%s' is not present on this device", g_osconfigRemediationConf);
                OsConfigCaptureReason(reason, "The OSConfig remediation file '%s' is not present on this device", g_osconfigRemediationConf);
                status = EEXIST;
            }
            else if (NULL == (inclusion = FormatInclusionForRemediation(log)))
            {
                OsConfigLogError(log, "CheckSshProtocol: FormatInclusionForRemediation failed");
                status = ENOMEM;
            }
            else if (0 != FindTextInFile(g_sshServerConfiguration, inclusion, log))
            {
                OsConfigLogError(log, "CheckSshProtocol: '%s' is not found included in '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
                OsConfigCaptureReason(reason, "'%s' is not found included in %s", g_osconfigRemediationConf, g_sshServerConfiguration);
                status = ENOENT;
            }
            else if (0 == (status = CheckLineFoundNotCommentedOut(g_osconfigRemediationConf, '#', protocol, reason, log)))
            {
                OsConfigLogInfo(log, "CheckSshProtocol: '%s' is found uncommented in %s", protocol, g_osconfigRemediationConf);
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "CheckSshProtocol: '%s' is not found uncommented with '#' in %s", protocol, g_osconfigRemediationConf);
                status = ENOENT;
            }

            FREE_MEMORY(inclusion);
        }
    }

    FREE_MEMORY(protocol);

    OsConfigLogInfo(log, "CheckSshProtocol: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

    return status;
}

static int CheckAllowDenyUsersGroups(const char* lowercase, const char* expectedValue, char** reason, void* log)
{
    const char* commandTemplate = "%s -T | grep \"%s %s\"";
    char* command = NULL;
    char* textResult = NULL;
    size_t commandLength = 0;
    size_t valueLength = 0;
    size_t i = 0;
    char* value = NULL;
    int status = 0;

    if ((NULL == lowercase) || (NULL == expectedValue))
    {
        OsConfigLogError(log, "CheckAllowDenyUsersGroups: invalid arguments");
        return EINVAL;
    }
    else if (0 != IsSshServerActive(log))
    {
        return status;
    }
    else if (NULL == strchr(expectedValue, ' '))
    {
        // If the expected value is not a list of users or groups separated by space, but a single user or group, then:
        return CheckSshOptionIsSet(lowercase, expectedValue, NULL, reason, log);
    }

    valueLength = strlen(expectedValue);

    for (i = 0; i < valueLength; i++)
    {
        if (NULL == (value = DuplicateString(&(expectedValue[i]))))
        {
            OsConfigLogError(log, "CheckAllowDenyUsersGroups: failed to duplicate string");
            status = ENOMEM;
            break;
        }
        else
        {
            TruncateAtFirst(value, ' ');

            commandLength = strlen(commandTemplate) + strlen(g_sshServerService) + strlen(lowercase) + strlen(value) + 1;
            if (NULL != (command = malloc(commandLength)))
            {
                memset(command, 0, commandLength);
                snprintf(command, commandLength, commandTemplate, g_sshServerService, lowercase, value);

                status = ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, NULL);

                FREE_MEMORY(textResult);
                FREE_MEMORY(command);
            }
            else
            {
                OsConfigLogError(log, "CheckAllowDenyUsersGroups: failed to allocate memory");
                status = ENOMEM;
                FREE_MEMORY(value);
                break;
            }

            i += strlen(value);
            FREE_MEMORY(value);
        }
    }

    if (0 == status)
    {
        OsConfigCaptureSuccessReason(reason, "%s reports that '%s' is set to '%s'", g_sshServerService, lowercase, expectedValue);
    }
    else
    {
        OsConfigCaptureReason(reason, "'%s' is not set to '%s' in SSH Server response", lowercase, expectedValue);
    }

    OsConfigLogInfo(log, "CheckAllowDenyUsersGroups: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);

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

static char* FormatRemediationValues(void* log)
{
    const char* remediationTemplate = "%s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n";
    char* remediation = NULL;
    size_t remediationSize = 0;

    remediationSize = strlen(remediationTemplate) + strlen(g_sshdConfigRemediationHeader) +
        strlen(g_sshPort) + strlen(g_desiredSshPort ? g_desiredSshPort : g_sshDefaultSshPort) +
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

    if (NULL != (remediation = malloc(remediationSize)))
    {
        memset(remediation, 0, remediationSize);
        snprintf(remediation, remediationSize, remediationTemplate, g_sshdConfigRemediationHeader,
            g_sshPort, g_desiredSshPort ? g_desiredSshPort : g_sshDefaultSshPort,
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
    }
    else
    {
        OsConfigLogError(log, "GetRemediationToSaveToFile: out of memory");
    }

    return remediation;
}

static int IncludeRemediationSshConfFile(void* log)
{
    const char* etcSshSshdConfigD = "/etc/ssh/sshd_config.d";
    const char* configurationTemplate = "%s%s";
    char* originalConfiguration = NULL;
    char* newConfiguration = NULL;
    char* inclusion = NULL;
    size_t newConfigurationSize = 0;
    size_t inclusionSize = 0;
    int desiredAccess = atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess);
    int status = 0;

    if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogInfo(log, "IncludeRemediationSshConfFile: '%s' is not present on this device", g_sshServerConfiguration);
        return EEXIST;
    }
    else if ((NULL == (inclusion = FormatInclusionForRemediation(log))) || (0 == (inclusionSize = strlen(inclusion))))
    {
        OsConfigLogInfo(log, "IncludeRemediationSshConfFile: failed preparing the inclusion statement, cannot include '%s' into '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
        return ENOMEM;
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
            if (0 != strncmp(originalConfiguration, inclusion, inclusionSize))
            {
                newConfigurationSize = strlen(configurationTemplate) + inclusionSize + strlen(originalConfiguration) + 1;

                if (NULL != (newConfiguration = malloc(newConfigurationSize)))
                {
                    memset(newConfiguration, 0, newConfigurationSize);
                    snprintf(newConfiguration, newConfigurationSize, configurationTemplate, inclusion, originalConfiguration);

                    if (true == SecureSaveToFile(g_sshServerConfiguration, newConfiguration, newConfigurationSize, log))
                    {
                        OsConfigLogInfo(log, "IncludeRemediationSshConfFile: '%s' is now included into '%s'", g_osconfigRemediationConf, g_sshServerConfiguration);
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

            FREE_MEMORY(originalConfiguration);
        }
        else
        {
            OsConfigLogError(log, "IncludeRemediationSshConfFile: failed to read from '%s'", g_sshServerConfiguration);
            status = EEXIST;
        }
    }

    SetFileAccess(g_sshServerConfiguration, 0, 0, desiredAccess, log);

    FREE_MEMORY(inclusion);

    return status;
}

static int SaveRemediationToConfFile(void* log)
{
    char* newRemediation = NULL;
    char* currentRemediation = NULL;
    size_t newRemediationSize = 0;
    int status = 0;

    if ((NULL == (newRemediation = FormatRemediationValues(log))) || (0 == (newRemediationSize = strlen(newRemediation))))
    {
        OsConfigLogInfo(log, "SaveRemediationToConfFile: failed formatting, cannot save remediation to '%s'", g_osconfigRemediationConf);
        return ENOMEM;
    }

    if ((NULL != (currentRemediation = LoadStringFromFile(g_osconfigRemediationConf, false, log))) && (0 == strncmp(currentRemediation, newRemediation, newRemediationSize)))
    {
        OsConfigLogInfo(log, "SaveRemediationToConfFile: '%s' already contains the correct remediation values:\n---\n%s---", g_osconfigRemediationConf, newRemediation);
    }
    else
    {
        if (true == SavePayloadToFile(g_osconfigRemediationConf, newRemediation, newRemediationSize, log))
        {
            OsConfigLogInfo(log, "SaveRemediationToConfFile: '%s' is now updated to the following remediation values:\n---\n%s---", g_osconfigRemediationConf, newRemediation);
        }
        else
        {
            OsConfigLogError(log, "SaveRemediationToConfFile: failed to save remediation values to '%s'", g_osconfigRemediationConf);
            status = ENOENT;
        }
    }

    SetFileAccess(g_osconfigRemediationConf, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), log);

    FREE_MEMORY(newRemediation);
    FREE_MEMORY(currentRemediation);

    return status;
}

static int BackupSshdConfig(const char* configuration, void* log)
{
    size_t configurationSize = 0;
    int status = 0;

    if ((false == FileExists(g_sshServerConfigurationBackup)) && (NULL != configuration) && (0 < (configurationSize = strlen(configuration))))
    {
        if (false == SavePayloadToFile(g_sshServerConfigurationBackup, configuration, configurationSize, log))
        {
            status = ENOENT;
        }
    }

    return status;
}

static int SaveRemediationToSshdConfig(void* log)
{
    const char* configurationTemplate = "%s%s";
    char* originalConfiguration = NULL;
    char* newConfiguration = NULL;
    char* remediation = NULL;
    size_t remediationSize = 0;
    size_t newConfigurationSize = 0;
    int desiredAccess = atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess);
    int status = 0;

    if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogInfo(log, "SaveRemediationToSshdConfig: '%s' is not present on this device", g_sshServerConfiguration);
        status = EEXIST;
    }
    else if ((NULL == (remediation = FormatRemediationValues(log))) || (0 == (remediationSize = strlen(remediation))))
    {
        OsConfigLogInfo(log, "SaveRemediationToSshdConfig: failed formatting, cannot save remediation values to '%s'", g_sshServerConfiguration);
        status = ENOMEM;
    }
    else if (NULL == (originalConfiguration = LoadStringFromFile(g_sshServerConfiguration, false, log)))
    {
        OsConfigLogError(log, "SaveRemediationToSshdConfig: failed to read from '%s'", g_sshServerConfiguration);
        status = EEXIST;
    }
    else if (0 != (status = BackupSshdConfig(originalConfiguration, log)))
    {
        OsConfigLogInfo(log, "SaveRemediationToSshdConfig: failed to make a backup copy of '%s'", g_sshServerConfiguration);
    }
    else if (0 == strncmp(originalConfiguration, remediation, remediationSize))
    {
        OsConfigLogInfo(log, "SaveRemediationToSshdConfig: '%s' already contains the correct remediation values:\n---\n%s---", g_sshServerConfiguration, remediation);
        status = 0;
    }
    else
    {
        FREE_MEMORY(originalConfiguration);

        if (NULL != (originalConfiguration = LoadStringFromFile(g_sshServerConfigurationBackup, false, log)))
        {
            newConfigurationSize = strlen(configurationTemplate) + remediationSize + strlen(originalConfiguration) + 1;
            if (NULL != (newConfiguration = malloc(newConfigurationSize)))
            {
                memset(newConfiguration, 0, newConfigurationSize);
                snprintf(newConfiguration, newConfigurationSize, configurationTemplate, remediation, originalConfiguration);

                if (true == SecureSaveToFile(g_sshServerConfiguration, newConfiguration, newConfigurationSize, log))
                {
                    OsConfigLogInfo(log, "SaveRemediationToSshdConfig: '%s' is now updated to include the following remediation values:\n---\n%s---", g_sshServerConfiguration, remediation);
                    status = 0;
                }
                else
                {
                    OsConfigLogError(log, "SaveRemediationToSshdConfig: failed to save remediation values to '%s'", g_sshServerConfiguration);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "SaveRemediationToSshdConfig: out of memory, cannot save remediation values to '%s'", g_sshServerConfiguration);
                status = ENOMEM;
            }
        }
        else
        {
            OsConfigLogError(log, "SaveRemediationToSshdConfig: failed to read from '%s'", g_sshServerConfigurationBackup);
            status = EEXIST;
        }
    }

    FREE_MEMORY(remediation);
    FREE_MEMORY(originalConfiguration);
    FREE_MEMORY(newConfiguration);

    SetFileAccess(g_sshServerConfigurationBackup, 0, 0, desiredAccess, log);
    SetFileAccess(g_sshServerConfiguration, 0, 0, desiredAccess, log);

    return status;
}

int InitializeSshAudit(void* log)
{
    int status = 0;

    g_auditOnlySession = true;

    if ((NULL == (g_desiredPermissionsOnEtcSshSshdConfig = DuplicateString(g_sshDefaultSshSshdConfigAccess))) ||
        (NULL == (g_desiredSshPort = DuplicateString(g_sshDefaultSshPort))) ||
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
    bool configurationChanged = false;

    OsConfigLogInfo(log, "SshAuditCleanup: %s", g_auditOnlySession ? "audit only" : "audit and remediate");

    if (false == g_auditOnlySession)
    {
        if (0 == IsSshConfigIncludeSupported(log))
        {
            IncludeRemediationSshConfFile(log);
            if (0 == SaveRemediationToConfFile(log))
            {
                configurationChanged = true;
            }
        }
        else if (0 == SaveRemediationToSshdConfig(log))
        {
            configurationChanged = true;
        }

        if (configurationChanged)
        {
            // Signal to the SSH Server service to reload configuration
            RestartDaemon(g_sshServerService, log);
        }
    }

    FREE_MEMORY(g_desiredPermissionsOnEtcSshSshdConfig);
    FREE_MEMORY(g_desiredSshPort);
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

int InitializeSshAuditCheck(const char* name, char* value, void* log)
{
    bool isValidValue = ((NULL == value) || (0 == value[0])) ? false : true;
    int status = 0;

    if (NULL == name)
    {
        OsConfigLogError(log, "InitializeSshAuditCheck: invalid check name argument");
        return EINVAL;
    }

    if ((0 == strcmp(name, g_remediateEnsurePermissionsOnEtcSshSshdConfigObject)) || (0 == strcmp(name, g_initEnsurePermissionsOnEtcSshSshdConfigObject)))
    {
        FREE_MEMORY(g_desiredPermissionsOnEtcSshSshdConfig);
        status = (NULL != (g_desiredPermissionsOnEtcSshSshdConfig = DuplicateString(isValidValue ? value : g_sshDefaultSshSshdConfigAccess))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshPortIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureSshPortIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredSshPort);
        status = (NULL != (g_desiredSshPort = DuplicateString(isValidValue ? value : g_sshDefaultSshPort))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshBestPracticeProtocolObject)) || (0 == strcmp(name, g_initEnsureSshBestPracticeProtocolObject)))
    {
        FREE_MEMORY(g_desiredSshBestPracticeProtocol);
        status = (NULL != (g_desiredSshBestPracticeProtocol = DuplicateString(isValidValue ? value : g_sshDefaultSshProtocol))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshBestPracticeIgnoreRhostsObject)) || (0 == strcmp(name, g_initEnsureSshBestPracticeIgnoreRhostsObject)))
    {
        FREE_MEMORY(g_desiredSshBestPracticeIgnoreRhosts);
        status = (NULL != (g_desiredSshBestPracticeIgnoreRhosts = DuplicateString(isValidValue ? value : g_sshDefaultSshYes))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshLogLevelIsSetObject)) || (0 == strcmp(name, g_initEnsureSshLogLevelIsSetObject)))
    {
        FREE_MEMORY(g_desiredSshLogLevelIsSet);
        status = (NULL != (g_desiredSshLogLevelIsSet = DuplicateString(isValidValue ? value : g_sshDefaultSshLogLevel))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshMaxAuthTriesIsSetObject)) || (0 == strcmp(name, g_initEnsureSshMaxAuthTriesIsSetObject)))
    {
        FREE_MEMORY(g_desiredSshMaxAuthTriesIsSet);
        status = (NULL != (g_desiredSshMaxAuthTriesIsSet = DuplicateString(isValidValue ? value : g_sshDefaultSshMaxAuthTries))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureAllowUsersIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureAllowUsersIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredAllowUsersIsConfigured);
        status = (NULL != (g_desiredAllowUsersIsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshAllowUsers))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureDenyUsersIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureDenyUsersIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredDenyUsersIsConfigured);
        status = (NULL != (g_desiredDenyUsersIsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshDenyUsers))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureAllowGroupsIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureAllowGroupsIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredAllowGroupsIsConfigured);
        status = (NULL != (g_desiredAllowGroupsIsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshAllowGroups))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureDenyGroupsConfiguredObject)) || (0 == strcmp(name, g_initEnsureDenyGroupsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredDenyGroupsConfigured);
        status = (NULL != (g_desiredDenyGroupsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshDenyGroups))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject)) || (0 == strcmp(name, g_initEnsureSshHostbasedAuthenticationIsDisabledObject)))
    {
        FREE_MEMORY(g_desiredSshHostbasedAuthenticationIsDisabled);
        status = (NULL != (g_desiredSshHostbasedAuthenticationIsDisabled = DuplicateString(isValidValue ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshPermitRootLoginIsDisabledObject)) || (0 == strcmp(name, g_initEnsureSshPermitRootLoginIsDisabledObject)))
    {
        FREE_MEMORY(g_desiredSshPermitRootLoginIsDisabled);
        status = (NULL != (g_desiredSshPermitRootLoginIsDisabled = DuplicateString(isValidValue ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject)) || (0 == strcmp(name, g_initEnsureSshPermitEmptyPasswordsIsDisabledObject)))
    {
        FREE_MEMORY(g_desiredSshPermitEmptyPasswordsIsDisabled);
        status = (NULL != (g_desiredSshPermitEmptyPasswordsIsDisabled = DuplicateString(isValidValue ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureSshClientIntervalCountMaxIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredSshClientIntervalCountMaxIsConfigured);
        status = (NULL != (g_desiredSshClientIntervalCountMaxIsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshClientIntervalCountMax))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshClientAliveIntervalIsConfiguredObject)) || (0 == strcmp(name, g_initEnsureSshClientAliveIntervalIsConfiguredObject)))
    {
        FREE_MEMORY(g_desiredSshClientAliveIntervalIsConfigured);
        status = (NULL != (g_desiredSshClientAliveIntervalIsConfigured = DuplicateString(isValidValue ? value : g_sshDefaultSshClientAliveInterval))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshLoginGraceTimeIsSetObject)) || (0 == strcmp(name, g_initEnsureSshLoginGraceTimeIsSetObject)))
    {
        FREE_MEMORY(g_desiredSshLoginGraceTimeIsSet);
        status = (NULL != (g_desiredSshLoginGraceTimeIsSet = DuplicateString(isValidValue ? value : g_sshDefaultSshLoginGraceTime))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject)) || (0 == strcmp(name, g_initEnsureOnlyApprovedMacAlgorithmsAreUsedObject)))
    {
        FREE_MEMORY(g_desiredOnlyApprovedMacAlgorithmsAreUsed);
        status = (NULL != (g_desiredOnlyApprovedMacAlgorithmsAreUsed = DuplicateString(isValidValue ? value : g_sshDefaultSshMacs))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshWarningBannerIsEnabledObject)) || (0 == strcmp(name, g_initEnsureSshWarningBannerIsEnabledObject)))
    {
        FREE_MEMORY(g_desiredSshWarningBannerIsEnabled);
        status = (NULL != (g_desiredSshWarningBannerIsEnabled = (isValidValue && (NULL != strstr(value, "\\n"))) ?
            RepairBrokenEolCharactersIfAny(value) : DuplicateString(isValidValue ? value : g_sshDefaultSshBannerText))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject)) || (0 == strcmp(name, g_initEnsureUsersCannotSetSshEnvironmentOptionsObject)))
    {
        FREE_MEMORY(g_desiredUsersCannotSetSshEnvironmentOptions);
        status = (NULL != (g_desiredUsersCannotSetSshEnvironmentOptions = DuplicateString(isValidValue ? value : g_sshDefaultSshNo))) ? 0 : ENOMEM;
    }
    else if ((0 == strcmp(name, g_remediateEnsureAppropriateCiphersForSshObject)) || (0 == strcmp(name, g_initEnsureAppropriateCiphersForSshObject)))
    {
        FREE_MEMORY(g_desiredAppropriateCiphersForSsh);
        status = (NULL != (g_desiredAppropriateCiphersForSsh = DuplicateString(isValidValue ? value : g_sshDefaultSshCiphers))) ? 0 : ENOMEM;
    }
    else
    {
        OsConfigLogError(log, "InitializeSshAuditCheck: unsupported check name '%s'", name);
        status = EINVAL;
    }

    OsConfigLogInfo(log, "InitializeSshAuditCheck: '%s' to '%s', %d", name, value ? value : "default", status);

    return status;
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
    else if (0 == strcmp(name, g_auditEnsureSshPortIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshPort);
        CheckSshOptionIsSet(lowercase, g_desiredSshPort ? g_desiredSshPort : g_sshDefaultSshPort, NULL, reason, log);
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
        CheckAllowDenyUsersGroups(lowercase, g_desiredAllowUsersIsConfigured ? g_desiredAllowUsersIsConfigured : g_sshDefaultSshAllowUsers, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureDenyUsersIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshDenyUsers);
        CheckAllowDenyUsersGroups(lowercase, g_desiredDenyUsersIsConfigured ? g_desiredDenyUsersIsConfigured : g_sshDefaultSshDenyUsers, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureAllowGroupsIsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshAllowGroups);
        CheckAllowDenyUsersGroups(lowercase, g_desiredAllowGroupsIsConfigured ? g_desiredAllowGroupsIsConfigured : g_sshDefaultSshAllowGroups, reason, log);
    }
    else if (0 == strcmp(name, g_auditEnsureDenyGroupsConfiguredObject))
    {
        lowercase = DuplicateStringToLowercase(g_sshDenyGroups);
        CheckAllowDenyUsersGroups(lowercase, g_desiredDenyGroupsConfigured ? g_desiredDenyGroupsConfigured : g_sshDefaultSshDenyGroups, reason, log);
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
            atoi(g_desiredPermissionsOnEtcSshSshdConfig ? g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), reason, log);
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
        if (0 == (status = InitializeSshAuditCheck(name, value, log)))
        {
            status = SetFileAccess(g_sshServerConfiguration, 0, 0, atoi(g_desiredPermissionsOnEtcSshSshdConfig ?
                g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), log);
        }
    }
    else if ((0 == strcmp(name, g_remediateEnsureSshPortIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshBestPracticeProtocolObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshBestPracticeIgnoreRhostsObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshLogLevelIsSetObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshMaxAuthTriesIsSetObject)) ||
        (0 == strcmp(name, g_remediateEnsureAllowUsersIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureDenyUsersIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureAllowGroupsIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureDenyGroupsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshHostbasedAuthenticationIsDisabledObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshPermitRootLoginIsDisabledObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshPermitEmptyPasswordsIsDisabledObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshClientIntervalCountMaxIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshClientAliveIntervalIsConfiguredObject)) ||
        (0 == strcmp(name, g_remediateEnsureSshLoginGraceTimeIsSetObject)) ||
        (0 == strcmp(name, g_remediateEnsureOnlyApprovedMacAlgorithmsAreUsedObject)) ||
        (0 == strcmp(name, g_remediateEnsureUsersCannotSetSshEnvironmentOptionsObject)) ||
        (0 == strcmp(name, g_remediateEnsureAppropriateCiphersForSshObject)))
    {
        status = InitializeSshAuditCheck(name, value, log);
    }
    else if (0 == strcmp(name, g_remediateEnsureSshWarningBannerIsEnabledObject))
    {
        if (0 == (status = InitializeSshAuditCheck(name, value, log)))
        {
            status = SetSshWarningBanner(atoi(g_desiredPermissionsOnEtcSshSshdConfig ?
                g_desiredPermissionsOnEtcSshSshdConfig : g_sshDefaultSshSshdConfigAccess), g_desiredSshWarningBannerIsEnabled, log);
        }
    }
    else
    {
        OsConfigLogError(log, "ProcessSshAuditCheck: unsupported check name '%s', nothing done", name);
        status = 0;
    }

    FREE_MEMORY(lowercase);

    if ((NULL != reason) && (NULL == *reason))
    {
        if (0 != IsSshServerActive(log))
        {
            OsConfigCaptureSuccessReason(reason, "%s is not present or active, nothing to audit", g_sshServerService);
        }
        else
        {
            OsConfigLogError(log, "ProcessSshAuditCheck(%s): audit failure without a reason", name);
            OsConfigCaptureReason(reason, SECURITY_AUDIT_FAIL);
        }
    }
    else if ((NULL != value) && (NULL == reason))
    {
        g_auditOnlySession = false;
    }

    OsConfigLogInfo(log, "ProcessSshAuditCheck(%s, '%s'): '%s' and %d", name, value ? value : "", (NULL != reason) ? *reason : "", status);

    return status;
}
