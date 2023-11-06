// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

const char* g_sshServerService = "sshd";
const char* g_sshServerConfiguration = "/etc/ssh/sshd_config";

static char* GetSshServerState(const char* name, void* log)
{
    const char* commandForAll = "sshd -T";
    const char* commandTemplateForOne = "sshd -T | grep %s";
    char* command = NULL;
    char* textResult = NULL;
    int status = 0;

    if (NULL == name)
    {
        if (0 != (status = ExecuteCommand(NULL, commandForAll, true, false, 0, 0, &textResult, NULL, NULL)))
        {
            OsConfigLogError(log, "GetSshServerState: '%s' failed with %d", commandForAll, status);
        }
    }
    else
    {
        if (NULL != (command = FormatAllocateString(commandTemplateForOne, name)))
        {
            if (0 != (status = ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, NULL)))
            {
                OsConfigLogError(log, "GetSshServerState: '%s' failed with %d", command, status);
            }
            else if (NULL != textResult)
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

static bool IsSshServerActive(void* log)
{
    bool result = true;
    
    if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogInfo(log, "IsSshServerActive: the SSH Server configuration file '%s' is not present on this device", g_sshServerConfiguration);
        result = false;
    }
    
    if (false == IsDaemonActive(g_sshServerService, log))
    {
        OsConfigLogInfo(log, "IsSshServerActive: the SSH Server service '%s' is not active on this device", g_sshServerService);
        result = false;
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
    else if (false == IsSshServerActive(log))
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
    else if (false == IsSshServerActive(log))
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

int CheckLimitedUserAcccessForSsh(const char** values, unsigned int numberOfValues, char** reason, void* log)
{
    char* value = NULL;
    size_t i = 0;
    bool oneFound = false;
    int status = 0;

    if ((NULL == values) || (0 == numberOfValues))
    {
        OsConfigLogError(log, "CheckLimitedUserAcccessForSsh: invalid arguments (%p, %u)", values, numberOfValues);
        return EINVAL;
    }
    else if (false == IsSshServerActive(log))
    {
        return status;
    }

    for (i = 0; i < numberOfValues; i++)
    {
        if (NULL != (value = GetSshServerState(values[i], log)))
        {
            OsConfigLogInfo(log, "CheckLimitedUserAcccessForSsh: '%s' found in SSH Server response set to '%s'", values[i], value);
            FREE_MEMORY(value);
            oneFound = true;
            break;
        }
        else
        {
            OsConfigLogError(log, "CheckLimitedUserAcccessForSsh: '%s' not found in SSH Server response", values[i]);
            OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' is not found in SSH server response", values[i]);
        }
    }

    status = oneFound ? 0 : ENOENT;

    OsConfigLogInfo(log, "CheckLimitedUserAcccessForSsh: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckSshOptionIsSetToString(const char* option, const char* expectedValue, char** reason, void* log)
{
    char* value = NULL;
    int status = 0;

    if ((NULL == option) || (NULL == expectedValue))
    {
        OsConfigLogError(log, "CheckSshOptionIsSetToString: invalid arguments (%s, %s)", option, expectedValue);
        return EINVAL;
    }

    if (false == IsSshServerActive(log))
    {
        return status;
    }

    if (NULL != (value = GetSshServerState(option, log)))
    {
        OsConfigLogInfo(log, "CheckSshOptionIsSetToString: '%s' found in SSH Server response set to '%s'", option, value);

        if (0 != strcmp(value, expectedValue))
        {
            OsConfigLogError(log, "CheckSshOptionIsSetToString: '%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, value);
            OsConfigCaptureReason(reason, "'%s' is not set to '%s' in SSH Server response (but to '%s')",
                "%s, also '%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, value);
            status = ENOENT;
        }

        FREE_MEMORY(value);
    }
    else
    {
        OsConfigLogError(log, "CheckSshOptionIsSetToString: '%s' not found in SSH Server response", option);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' is not found in SSH server response", option);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshOptionIsSetToString: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckSshOptionIsSetToInteger(const char* option, int expectedValue, int* actualValue, char** reason, void* log)
{
    char* value = NULL;
    int integerValue = 0;
    int status = 0;

    if (NULL == option)
    {
        OsConfigLogError(log, "CheckSshOptionIsSetToInteger: invalid argument");
        return EINVAL;
    }

    if (false == IsSshServerActive(log))
    {
        return status;
    }

    if (NULL != (value = GetSshServerState(option, log)))
    {
        integerValue = atoi(value);
        OsConfigLogInfo(log, "CheckSshOptionIsSetToInteger: '%s' found in SSH Server response set to '%s' (%d)", option, value, integerValue);

        if (actualValue)
        {
            *actualValue = integerValue;
        }
        else if (integerValue != expectedValue)
        {
            OsConfigLogError(log, "CheckSshOptionIsSetToInteger: '%s' is not set to %d in SSH Server response (but to %d)", option, expectedValue, integerValue);
            OsConfigCaptureReason(reason, "'%s' is not set to '%s' in SSH Server response (but to '%s')",
                "%s, also '%s' is not set to '%s' in SSH Server response (but to '%s')", option, expectedValue, integerValue);
            status = ENOENT;
        }

        FREE_MEMORY(value);
    }
    else
    {
        OsConfigLogError(log, "CheckSshOptionIsSetToInteger: '%s' not found in SSH Server response", option);
        OsConfigCaptureReason(reason, "'%s' not found in SSH Server response", "%s, also '%s' is not found in SSH server response", option);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshOptionIsSetToInteger: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

int CheckSshIdleTimeoutInterval(char** reason, void* log)
{
    int actualValue = 0;
    int status = CheckSshOptionIsSetToInteger("clientaliveinterval", 0, &actualValue, reason, log);
    
    if (IsSshServerActive(log) && (actualValue <= 0))
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

    if (IsSshServerActive(log) && (actualValue > 60))
    {
        OsConfigLogError(log, "CheckSshLoginGraceTime: 'logingracetime' is not set to 60 or less in SSH Server response (but to %d)", actualValue);
        OsConfigCaptureReason(reason, "'logingracetime' is not set to a value of 60 or less in SSH Server response (but to %d)",
            "%s, also 'logingracetime' is not set to a value of 60 or less in SSH Server response (but to %d)", actualValue);
        status = ENOENT;
    }

    OsConfigLogInfo(log, "CheckSshLoginGraceTime: %s (%d)", status ? "failed" : "passed", status);

    return status;
}

//TODO: add code to apppend an option when it cannot be found ###############################

// Callers must supply a regular expression (via the regex parameter) that mathes the value to be set:
// "[[:alnum:]]+" for simple alphanumeric single word or number values such as "Foo", or "123" or "A1b23"
// "[[:alnum:]]+(,\\s[[:alnum:]]+)*$" for space and comma separated alphanumeric values such as "foo1, foo2, foo3"
// "[[[:alnum:]]-]+(,\\s[[[:alnum:]]-]+)*" for space and comma separated alphanumeric values with dashes such as "foo-1, foo, foo-3"
// "(.+)\\/([^\\/]+)$" for path names such as "/etc/foo/foo.txt"
// ..and so on
int SetSshOption(const char* option, const char* value, const char* regex, void* log)
{
    //const char* commandTemplate = "sed 's/.*%s .*/%s %s/' %s";
    const char* commandTemplate = "sed '/^%s /{h;s/ .*/ %s/};${x;/^$/{s//%s %s/;H};x}' %s";

    //const char* commandTemplate = "sed -E 's/#%s\\s%s/%s %s/;s/%s\\s%s/%s %s/' %s";
    const char* configurationBackup = "/etc/ssh/~sshd_config.bak";

    char* command = NULL;
    char* commandResult = NULL;
    char* originalConfiguration = NULL;
    size_t commandLength = 0;
    int status = 0;

    if ((NULL == option) || (NULL == value) || (NULL == regex))
    {
        OsConfigLogError(log, "SetSshOption: invalid arguments (%s, %s, %s)", option, value, regex);
        return EINVAL;
    }
    else if (false == FileExists(g_sshServerConfiguration))
    {
        OsConfigLogError(log, "SetSshOption: the SSH Server configuration file '%s' is not present on this device, no place to set '%s' to '%s' via '%s'", 
            g_sshServerConfiguration, option, value, regex);
        return ENOENT;
    }

    //commandLength = strlen(commandTemplate) + (4 * strlen(option)) + (2 * strlen(regex)) + (2 * strlen(value)) + strlen(g_sshServerConfiguration) + 1;
    //commandLength = strlen(commandTemplate) + (2 * strlen(option)) + strlen(value) + strlen(g_sshServerConfiguration) + 1;
    commandLength = strlen(commandTemplate) + (2 * strlen(option)) + (2 * strlen(value)) + strlen(g_sshServerConfiguration) + 1;

    if (NULL == (command = malloc(commandLength)))
    {
        OsConfigLogError(log, "SetSshOption: out of memory");
        status = ENOMEM;
    }
    else
    {
        memset(command, 0, commandLength);
        //snprintf(command, commandLength, commandTemplate, option, regex, option, value, option, regex, option, value, g_sshServerConfiguration);
        //snprintf(command, commandLength, commandTemplate, option, option, value, g_sshServerConfiguration);
        snprintf(command, commandLength, commandTemplate, option, value, option, value, g_sshServerConfiguration);

        OsConfigLogInfo(log, "SetSshOption: command: '%s'", command); /////////////// temporary, to be removed

        if ((0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, &commandResult, NULL, log))) && commandResult)
        {
            if (NULL != (originalConfiguration = LoadStringFromFile(g_sshServerConfiguration, false, log)))
            {
                if (SavePayloadToFile(configurationBackup, originalConfiguration, strlen(originalConfiguration), log))
                {
                    OsConfigLogInfo(log, "SetSshOption: saved a backup copy of '%s' to '%s", g_sshServerConfiguration, configurationBackup);

                    if (false == SavePayloadToFile(g_sshServerConfiguration, commandResult, strlen(commandResult), log))
                    {
                        OsConfigLogError(log, "SetSshOption: failed saving the updated configuration to '%s'", g_sshServerConfiguration);
                        status = ENOENT;
                    }
                }
                else
                {
                    OsConfigLogError(log, "SetSshOption: cannot make a copy of '%s' as '%s", g_sshServerConfiguration, configurationBackup);
                    remove(configurationBackup);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "SetSshOption: failed to read current SSH Server configuration from '%s'", g_sshServerConfiguration);
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogInfo(log, "SetSshOption: failed setting '%s' to '%s' in '%s' (%d)", option, value, g_sshServerConfiguration, status);
        }
    }

    FREE_MEMORY(originalConfiguration);
    FREE_MEMORY(commandResult);
    FREE_MEMORY(command);

    OsConfigLogInfo(log, "SetSshOption('%s' to '%s'): %s (%d)", option, value, status ? "failed" : "passed", status);

    return status;
} 