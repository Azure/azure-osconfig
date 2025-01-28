// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* DuplicateString(const char* source)
{
    if (NULL == source)
    {
        return NULL;
    }

    return strdup(source);
}

char* DuplicateStringToLowercase(const char* source)
{
    int length = 0, i = 0;
    char* duplicate = NULL;

    if (NULL != (duplicate = DuplicateString(source)))
    {
        length = (int)strlen(duplicate);
        for (i = 0; i < length; i++)
        {
            duplicate[i] = tolower(duplicate[i]);
        }
    }

    return duplicate;
}

char* FormatAllocateString(const char* format, ...)
{
    char* stringToReturn = NULL;
    int formatResult = 0;
    int sizeOfBuffer = 0;

    if (NULL == format)
    {
        return stringToReturn;
    }

    va_list arguments;
    va_start(arguments, format);
    // snprintf returns the required buffer size, excluding the null terminator
    sizeOfBuffer = vsnprintf(NULL, 0, format, arguments);
    va_end(arguments);

    if (sizeOfBuffer >= 0)
    {
        if (NULL != (stringToReturn = malloc((size_t)sizeOfBuffer + 1)))
        {
            va_start(arguments, format);
            formatResult = vsnprintf(stringToReturn, sizeOfBuffer + 1, format, arguments);
            va_end(arguments);

            if ((formatResult < 0) || (formatResult > sizeOfBuffer))
            {
                FREE_MEMORY(stringToReturn);
            }
        }
    }
    return stringToReturn;
}

char* ConcatenateStrings(const char* first, const char* second)
{
    char* result = NULL;
    size_t resultSize = 0;

    if ((NULL == first) || (NULL == second))
    {
        return result;
    }

    resultSize = strlen(first) + strlen(second) + 1;

    if (NULL != (result = malloc(resultSize)))
    {
        memset(result, 0, resultSize);
        memcpy(result, first, strlen(first));
        strncat(result, second, resultSize);
    }

    return result;
}

int SleepMilliseconds(long milliseconds)
{
    struct timespec remaining = {0};
    struct timespec interval = {0};

    if ((milliseconds < 0) || (milliseconds > 999999999))
    {
        return EINVAL;
    }

    interval.tv_sec = (int)(milliseconds / 1000);
    interval.tv_nsec = (milliseconds % 1000) * 1000000;

    return nanosleep(&interval, &remaining);
}

char* GetHttpProxyData(void* log)
{
    const char* proxyVariables[] = {
        "http_proxy",
        "https_proxy",
        "HTTP_PROXY",
        "HTTPS_PROXY"
    };
    int proxyVariablesSize = ARRAY_SIZE(proxyVariables);

    char* proxyData = NULL;
    char* environmentVariable = NULL;
    int i = 0;

    for (i = 0; i < proxyVariablesSize; i++)
    {
        environmentVariable = getenv(proxyVariables[i]);
        if (NULL != environmentVariable)
        {
            // The environment variable string must be treated as read-only, make a copy for our use:
            proxyData = DuplicateString(environmentVariable);
            if (NULL == proxyData)
            {
                OsConfigLogError(log, "Cannot make a copy of the %s variable: %d", proxyVariables[i], errno);
            }
            else
            {
                OsConfigLogInfo(log, "Proxy data from %s: %s", proxyVariables[i], proxyData);
            }
            break;
        }
    }

    return proxyData;
}

size_t HashString(const char* source)
{
    // djb2 hashing algorithm

    size_t hash = 5381;
    size_t length = 0;
    size_t i = 0;

    if (NULL == source)
    {
        return 0;
    }

    length = strlen(source);

    for (i = 0; i < length; i++)
    {
        hash = ((hash << 5) + hash) + source[i];
    }

    return hash;
}

bool FreeAndReturnTrue(void* value)
{
    FREE_MEMORY(value);
    return true;
}

char* RepairBrokenEolCharactersIfAny(const char* value)
{
    char* result = NULL;
    size_t length = 0;
    size_t i = 0, j = 0;

    if ((NULL == value) || (2 >= (length = strlen(value))) || (NULL == (result = malloc(length + 1))))
    {
        FREE_MEMORY(result);
        return result;
    }

    memset(result, 0, length + 1);

    for (i = 0, j = 0; (i < length) && (j < length); i++, j++)
    {
        if ((i < (length - 1)) && (value[i] == '\\') && (value[i + 1] == 'n'))
        {
            result[j] = '\n';
            i += 1;
        }
        else
        {
            result[j] = value[i];
        }
    }

    return result;
}

int ConvertStringToIntegers(const char* source, char separator, int** integers, int* numIntegers, void* log)
{
    const char space = ' ';
    char* value = NULL;
    size_t sourceLength = 0;
    size_t i = 0;
    int status = 0;

    if ((NULL == source) || (NULL == integers) || (NULL == numIntegers))
    {
        OsConfigLogError(log, "ConvertSpaceSeparatedStringsToIntegers: invalid arguments");
        return EINVAL;
    }

    FREE_MEMORY(*integers);
    *numIntegers = 0;

    sourceLength = strlen(source);

    for (i = 0; i < sourceLength; i++)
    {
        if (NULL == (value = DuplicateString(&(source[i]))))
        {
            OsConfigLogError(log, "ConvertSpaceSeparatedStringsToIntegers: failed to duplicate string");
            status = ENOMEM;
            break;
        }
        else
        {
            TruncateAtFirst(value, separator);
            i += strlen(value);

            if (space != separator)
            {
                RemoveTrailingBlanks(value);
            }

            if (0 == *numIntegers)
            {
                *integers = (int*)malloc(sizeof(int));
                *numIntegers = 1;
            }
            else
            {
                *numIntegers += 1;
                *integers = realloc(*integers, (size_t)((*numIntegers) * sizeof(int)));
            }

            if (NULL == *integers)
            {
                OsConfigLogError(log, "ConvertSpaceSeparatedStringsToIntegers: failed to allocate memory");
                *numIntegers = 0;
                status = ENOMEM;
                break;
            }
            else
            {
                (*integers)[(*numIntegers) - 1] = atoi(value);
            }

            FREE_MEMORY(value);
        }
    }

    if (0 != status)
    {
        FREE_MEMORY(*integers);
        *numIntegers = 0;
    }

    OsConfigLogInfo(log, "ConvertStringToIntegers: %d (%d integers converted from '%s' separated with '%c')", status, *numIntegers, source, separator);

    return status;
}

int CheckAllWirelessInterfacesAreDisabled(char** reason, void* log)
{
    const char* command = "iwconfig 2>&1 | egrep -v 'no wireless extensions|not found' | grep Frequency";
    int status = 0;

    if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "CheckAllWirelessInterfacesAreDisabled: wireless interfaces are enabled");
        OsConfigCaptureReason(reason, "At least one active wireless interface is present");
        status = EEXIST;
    }
    else
    {
        OsConfigLogInfo(log, "CheckAllWirelessInterfacesAreDisabled: no wireless interfaces are enabled");
        OsConfigCaptureSuccessReason(reason, "No active wireless interfaces are present");
        status = 0;
    }

    return status;
}

int DisableAllWirelessInterfaces(void* log)
{
    const char* nmcli = "nmcli";
    const char* rfkill = "rfkill";
    const char* nmCliRadioAllOff = "nmcli radio wifi off";
    const char* rfKillBlockAll = "rfkill block all";
    int status = 0;

    if (0 == CheckAllWirelessInterfacesAreDisabled(NULL, log))
    {
        OsConfigLogInfo(log, "DisableAllWirelessInterfaces: no active wireless interfaces are present");
        return 0;
    }

    if ((0 != IsPresent(nmcli, log)) && (0 != IsPresent(rfkill, log)))
    {
        OsConfigLogInfo(log, "DisableAllWirelessInterfaces: neither '%s' or '%s' are installed", nmcli, rfkill);
        if (0 != (status = InstallOrUpdatePackage(rfkill, log)))
        {
            OsConfigLogError(log, "DisableAllWirelessInterfaces: neither '%s' or '%s' are installed, also failed "
                "to install '%s', automatic remediation is not possible", nmcli, rfkill, rfkill);
            status = ENOENT;
        }
    }

    if (0 == status)
    {
        if (0 == IsPresent(nmcli, log))
        {
            if (0 != (status = ExecuteCommand(NULL, nmCliRadioAllOff, true, false, 0, 0, NULL, NULL, log)))
            {
                OsConfigLogError(log, "DisableAllWirelessInterfaces: '%s' failed with %d", nmCliRadioAllOff, status);
            }
        }

        if (0 == IsPresent(rfkill, log))
        {
            if (0 != (status = ExecuteCommand(NULL, rfKillBlockAll, true, false, 0, 0, NULL, NULL, log)))
            {
                OsConfigLogError(log, "DisableAllWirelessInterfaces: '%s' failed with %d", rfKillBlockAll, status);
            }
        }
    }

    OsConfigLogInfo(log, "DisableAllWirelessInterfaces completed with %d", status);

    return status;
}

int SetDefaultDenyFirewallPolicy(void* log)
{
    const char* acceptInput = "iptables -A INPUT -j ACCEPT";
    const char* acceptForward = "iptables -A FORWARD -j ACCEPT";
    const char* acceptOutput = "iptables -A OUTPUT -j ACCEPT";
    const char* dropInput = "iptables -P INPUT DROP";
    const char* dropForward = "iptables -P FORWARD DROP";
    const char* dropOutput = "iptables -P OUTPUT DROP";
    int status = 0;

    // First, ensure all current traffic is accepted:
    if (0 != (status = ExecuteCommand(NULL, acceptInput, true, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", acceptInput, status);
    }
    else if (0 != (status = ExecuteCommand(NULL, acceptForward, true, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", acceptForward, status);
    }
    else if (0 != (status = ExecuteCommand(NULL, acceptOutput, true, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", acceptOutput, status);
    }

    if (0 == status)
    {
        // Then set default to drop:
        if (0 != (status = ExecuteCommand(NULL, dropInput, true, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", dropInput, status);
        }
        else if (0 != (status = ExecuteCommand(NULL, dropForward, true, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", dropForward, status);
        }
        else if (0 != (status = ExecuteCommand(NULL, dropOutput, true, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogError(log, "SetDefaultDenyFirewallPolicy: '%s' failed with %d", dropOutput, status);
        }
    }

    OsConfigLogInfo(log, "SetDefaultDenyFirewallPolicy completed with %d", status);

    return 0;
}

char* RemoveCharacterFromString(const char* source, char what, void* log)
{
    char* target = NULL;
    size_t sourceLength = 0, i = 0, j = 0;

    if ((NULL == source) || (0 == (sourceLength = strlen(source))))
    {
        OsConfigLogInfo(log, "RemoveCharacterFromString: empty or no string, nothing to replace");
        return NULL;
    }
    else if (NULL == (target = DuplicateString(source)))
    {
        OsConfigLogInfo(log, "RemoveCharacterFromString: out of memory");
        return NULL;
    }

    memset(target, 0, sourceLength + 1);

    for (i = 0, j = 0; i < sourceLength; i++)
    {
        if (what == source[i])
        {
            continue;
        }
        target[j] = source[i];
        j++;
    }

    OsConfigLogInfo(log, "RemoveCharacterFromString: removed all instances of '%c' if any from '%s' ('%s)", what, source, target);

    return target;
}

char* ReplaceEscapeSequencesInString(const char* source, const char* escapes, unsigned int numEscapes, char replacement, void* log)
{
    char* target = NULL;
    size_t sourceLength = 0, i = 0, j = 0, k = 0;
    bool found = false;

    if ((NULL == source) || (0 == (sourceLength = strlen(source))))
    {
        OsConfigLogInfo(log, "ReplaceEscapeSequencesInString: empty or no string, nothing to replace");
        return NULL;
    }
    else if ((NULL == escapes) || (0 == numEscapes))
    {
        OsConfigLogInfo(log, "ReplaceEscapeSequencesInString: empty or no sequence of characters, nothing to replace");
        return NULL;
    }
    else if (NULL == (target = DuplicateString(source)))
    {
        OsConfigLogInfo(log, "ReplaceEscapeSequencesInString: out of memory");
        return NULL;
    }

    memset(target, 0, sourceLength + 1);

    for (i = 0; i < sourceLength; i++)
    {
        found = false;

        for (j = 0; j < numEscapes; j++)
        {
            if (('\\' == source[i]) && (escapes[j] == source[i + 1]))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            target[k] = replacement;
            i += 1;
        }
        else
        {
            target[k] = source[i];
        }

        k += 1;
    }

    OsConfigLogInfo(log, "ReplaceEscapeSequencesInString returning '%s'", target);

    return target;
}

typedef struct PATH_LOCATIONS
{
    const char* location;
    const char* path;
} PATH_LOCATIONS;

int RemoveDotsFromPath(void* log)
{
    const char* path = "PATH";
    const char* dot = ".";
    const char* printenv = "printenv PATH";
    const char* setenvTemplate = "setenv PATH '%s'";

    PATH_LOCATIONS pathLocations[] = {
        { "/etc/sudoers", "secure_path" },
        { "/etc/environment", "PATH" },
        { "/etc/profile", "PATH" },
        { "/root/.profile", "PATH" }
    };
    unsigned int numPathLocations = ARRAY_SIZE(pathLocations), i = 0;
    char* setenv = NULL;
    char* currentPath = NULL;
    char* newPath = NULL;
    int status = 0, _status = 0;

    if (0 != CheckTextNotFoundInEnvironmentVariable(path, dot, false, NULL, log))
    {
        if (0 == (status == ExecuteCommand(NULL, printenv, false, false, 0, 0, &currentPath, NULL, log)))
        {
            if (NULL != (newPath = RemoveCharacterFromString(currentPath, dot[0], log)))
            {
                if (NULL != (setenv = FormatAllocateString(setenvTemplate, newPath)))
                {
                    if (0 == (status == ExecuteCommand(NULL, setenv, false, false, 0, 0, NULL, NULL, log)))
                    {
                        OsConfigLogInfo(log, "RemoveDotsFromPath: successfully set 'PATH' to '%s'", newPath);
                    }
                    else
                    {
                        OsConfigLogError(log, "RemoveDotsFromPath: '%s failed with %d", setenv, status);
                    }

                    FREE_MEMORY(setenv);
                }
                else
                {
                    OsConfigLogError(log, "RemoveDotsFromPath: out of memory");
                    status = ENOMEM;
                }

                FREE_MEMORY(newPath);
            }
            else
            {
                OsConfigLogError(log, "RemoveDotsFromPath: cannot remove '%c' from '%s'", dot[0], currentPath);
                status = EINVAL;
            }

            FREE_MEMORY(currentPath);
        }
        else
        {
            OsConfigLogError(log, "RemoveDotsFromPath: '%s' failed with %d", printenv, status);
        }
    }

    if (0 == status)
    {
        for (i = 0; i < numPathLocations; i++)
        {
            if (0 == CheckMarkedTextNotFoundInFile(pathLocations[i].location, pathLocations[i].path, dot, '#', NULL, log))
            {
                continue;
            }

            if (NULL != (currentPath = GetStringOptionFromFile(pathLocations[i].location, pathLocations[i].path, ' ', log)))
            {
                if (NULL != (newPath = RemoveCharacterFromString(currentPath, dot[0], log)))
                {
                    if (0 == (_status = SetEtcConfValue(pathLocations[i].location, pathLocations[i].path, newPath, log)))
                    {
                        OsConfigLogInfo(log, "RemoveDotsFromPath: successfully set '%s' to '%s' in '%s'",
                            pathLocations[i].path, pathLocations[i].location, newPath);
                    }

                    FREE_MEMORY(newPath);
                }
                else
                {
                    OsConfigLogError(log, "RemoveDotsFromPath: cannot remove '%c' from '%s' for '%s'",
                        dot[0], currentPath, pathLocations[i].location);
                    _status = EINVAL;
                }

                FREE_MEMORY(currentPath);
            }

            if (_status && (0 == status))
            {
                status = _status;
            }
        }
    }

    return status;
}

int RemoveEscapeSequencesFromFile(const char* fileName, const char* escapes, unsigned int numEscapes, char replacement, void* log)
{
    char* fileContents = NULL;
    char* newFileContents = NULL;
    int status = 0;

    if ((NULL == fileName) || (NULL == escapes) || (0 == numEscapes))
    {
        OsConfigLogInfo(log, "ReplaceEscapesFromFile: invalid argument");
        return EINVAL;
    }
    else if (false == FileExists(fileName))
    {
        OsConfigLogInfo(log, "ReplaceEscapesFromFile: called for a file that does not exist ('%s')", fileName);
        return EEXIST;
    }
    else if (NULL == (fileContents = LoadStringFromFile(fileName, false, log)))
    {
        OsConfigLogInfo(log, "ReplaceEscapesFromFile: cannot read from file '%s'", fileName);
        return ENOENT;
    }

    if (NULL != (newFileContents = ReplaceEscapeSequencesInString(fileContents, escapes, numEscapes, replacement, log)))
    {
        if (false == SecureSaveToFile(fileName, newFileContents, strlen(newFileContents), log))
        {
            OsConfigLogInfo(log, "ReplaceEscapesFromFile: failed saving '%s'", fileName);
            status = ENOENT;
        }
    }
    else
    {
        OsConfigLogInfo(log, "ReplaceEscapesFromFile: failed to replace desired characters in '%s'", fileName);
        status = ENOENT;
    }

    FREE_MEMORY(fileContents);
    FREE_MEMORY(newFileContents);

    return status;
}
