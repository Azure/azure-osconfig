// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* DuplicateString(const char* source)
{
    int length = 0;
    char* duplicate = NULL;

    if (NULL == source)
    {
        return duplicate;
    }

    length = (int)strlen(source);
    duplicate = (char*)malloc(length + 1);
    if (NULL != duplicate)
    {
        memcpy(duplicate, source, length);
        duplicate[length] = 0;
    }

    return duplicate;
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

static bool Free(void* value)
{
    FREE_MEMORY(value);
    return true;
}

int CheckLockoutForFailedPasswordAttempts(const char* fileName, void* log)
{
    char* value = NULL;
    int option = 0;

    return ((0 == CheckFileExists(fileName, log)) &&
        ((NULL != (value = GetStringOptionFromFile(fileName, "auth", ' ', log))) && (0 == strcmp("required", value)) && Free(value)) &&
        ((NULL != (value = GetStringOptionFromFile(fileName, "required", ' ', log))) && (0 == strcmp("pam_tally2.so", value)) && Free(value)) &&
        ((NULL != (value = GetStringOptionFromFile(fileName, "pam_tally2.so", ' ', log))) && (0 == strcmp("file=/var/log/tallylog", value)) && Free(value)) &&
        ((NULL != (value = GetStringOptionFromFile(fileName, "file", '=', log))) && (0 == strcmp("/var/log/tallylog", value)) && Free(value)) &&
        ((1 <= (option = GetIntegerOptionFromFile(fileName, "deny", '=', log))) && (5 >= option)) &&
        (0 < (option = GetIntegerOptionFromFile(fileName, "unlock_time", '=', log)))) ? 0 : ENOENT;
}