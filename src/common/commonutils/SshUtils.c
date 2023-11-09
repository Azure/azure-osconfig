// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

static const char* g_sshServerService = "sshd";
static const char* g_sshServerConfiguration = "/etc/ssh/sshd_config";
static const char* g_sshdDashTCommand = "sshd -T";

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

int CheckOnlyApprovedMacAlgorithmsAreUsed(const char** macs, unsigned int numberOfMacs, char** reason, void* log)
{
    const char* sshMacs = "macs";
    char* macsValue = NULL;
    char* value = NULL;
    size_t macsValueLength = 0;
    size_t i = 0, j = 0;
    bool macFound = false;
    int status = 0;

    if ((NULL == macs) || (0 == numberOfMacs))
    {
        OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: invalid arguments (%p, %u)", macs, numberOfMacs);
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

                for (j = 0; j < numberOfMacs; j++)
                {
                    if (0 == strcmp(value, macs[j]))
                    {
                        macFound = true;
                        break;
                    }
                }
                    
                if (false == macFound)
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: unapproved MAC algorithm '%s' found in SSH Server response", value);
                    OsConfigCaptureReason(reason, "Unapproved MAC algorithm '%s' found in SSH Server response", "%s, also MAC algorithm '%s' is unapproved", value);
                }

                i += strlen(value);

                macFound = false;

                FREE_MEMORY(value);

                continue;
            }
        }
    }

    FREE_MEMORY(macsValue);

    OsConfigLogInfo(log, "CheckOnlyApprovedMacAlgorithmsAreUsed: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckAppropriateCiphersForSsh(const char** ciphers, unsigned int numberOfCiphers, char** reason, void* log)
{
    const char* sshCiphers = "ciphers";
    char* ciphersValue = NULL;
    char* value = NULL;
    size_t ciphersValueLength = 0;
    size_t i = 0, j = 0;
    bool cipherFound = false;
    int status = 0;

    if ((NULL == ciphers) || (0 == numberOfCiphers))
    {
        OsConfigLogError(log, "CheckAppropriateCiphersForSsh: invalid arguments (%p, %u)", ciphers, numberOfCiphers);
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

                for (j = 0; j < numberOfCiphers; j++)
                {
                    if (0 == strcmp(value, ciphers[j]))
                    {
                        cipherFound = true;
                        break;
                    }
                }

                if (false == cipherFound)
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckAppropriateCiphersForSsh: unapproved cipher '%s' found in SSH Server response", value);
                    OsConfigCaptureReason(reason, "Unapproved cipher '%s' found in SSH Server response", "%s, also cipher '%s' is unapproved", value);
                }

                i += strlen(value);

                cipherFound = false;

                FREE_MEMORY(value);

                continue;
            }
        }

        for (j = 0; j < numberOfCiphers; j++)
        {
            if (NULL == strstr(ciphersValue, ciphers[j]))
            {
                status = ENOENT;
                OsConfigLogError(log, "CheckAppropriateCiphersForSsh: required cipher '%s' not found in SSH Server response", ciphers[j]);
                OsConfigCaptureReason(reason, "Required cipher '%s' not found in SSH Server response", "%s, also required cipher '%s' is not found", ciphers[j]);
            }
            else
            {
                OsConfigLogInfo(log, "CheckAppropriateCiphersForSsh: required cipher '%s' found in SSH Server response", ciphers[j]);
            }
        }
    }

    FREE_MEMORY(ciphersValue);

    OsConfigLogInfo(log, "CheckAppropriateCiphersForSsh: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckSshOptionIsSet(const char* option, const char* expectedValue, char** actualValue, char** reason, void* log)
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

        FREE_MEMORY(value);
    }
    else
    {
        OsConfigLogError(log, "CheckSshOptionIsSet: '%s' not found in SSH Server response", option);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' is not found in SSH server response", option);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshOptionIsSet: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

static int CheckSshOptionIsSetToInteger(const char* option, int expectedValue, int* actualValue, char** reason, void* log)
{
    char* actualValueString = NULL;
    char* expectedValueString = FormatAllocateString("%d", expectedValue);
    int status = CheckSshOptionIsSet(option, expectedValueString, &actualValueString, reason, log);

    if ((0 == status) && (NULL != actualValue))
    {
        *actualValue = (NULL != actualValueString) ? atoi(actualValueString) : -1;
    }

    FREE_MEMORY(actualValueString);
    FREE_MEMORY(expectedValueString);

    return status;
}

int CheckSshIdleTimeoutInterval(char** reason, void* log)
{
    int actualValue = 0;
    int status = CheckSshOptionIsSetToInteger("clientaliveinterval", 0, &actualValue, reason, log);
    
    if ((0 == IsSshServerActive(log)) && (actualValue <= 0))
    {
        OsConfigLogError(log, "CheckSshIdleTimeoutInterval: 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
        OsConfigCaptureReason(reason, "'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)",
            "%s, also 'clientaliveinterval' is not set to a greater than zero value in SSH Server response (but to %d)", actualValue);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshIdleTimeoutInterval: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckSshLoginGraceTime(char** reason, void* log)
{
    int actualValue = 0;
    int status = CheckSshOptionIsSetToInteger("logingracetime", 0, &actualValue, reason, log);

    if ((0 == IsSshServerActive(log)) && (actualValue > 60))
    {
        OsConfigLogError(log, "CheckSshLoginGraceTime: 'logingracetime' is not set to 60 or less in SSH Server response (but to %d)", actualValue);
        OsConfigCaptureReason(reason, "'logingracetime' is not set to a value of 60 or less in SSH Server response (but to %d)",
            "%s, also 'logingracetime' is not set to a value of 60 or less in SSH Server response (but to %d)", actualValue);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshLoginGraceTime: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int SetSshOption(const char* option, const char* value, void* log)
{
    // SED command template that replaces any instances of '*option' with 'option value' or adds 'option value' when 'option' is missing from sshd_config
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

    OsConfigLogInfo(log, "SetSshOption('%s' to '%s'): %s (%d)", option, value, status ? "failed" : "passed", status);

    return status;
}

int SetSshWarningBanner(unsigned int desiredBannerFileAccess, const char* bannerText, void* log)
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