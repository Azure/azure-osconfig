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

#define MAX_FORMAT_ALLOCATE_STRING_LENGTH 512
char* FormatAllocateString(const char* format, ...)
{
    char buffer[MAX_FORMAT_ALLOCATE_STRING_LENGTH] = {0};
    int formatResult = 0;
    char* stringToReturn = NULL;

    if (NULL == format)
    {
        return stringToReturn;
    }

    va_list arguments;
    va_start(arguments, format);
    formatResult = vsnprintf(buffer, MAX_FORMAT_ALLOCATE_STRING_LENGTH, format, arguments);
    va_end(arguments);

    if ((formatResult > 0) && (formatResult < MAX_FORMAT_ALLOCATE_STRING_LENGTH))
    {
        stringToReturn = DuplicateString(buffer);
    }

    return stringToReturn;
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