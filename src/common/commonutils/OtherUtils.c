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

#define DEFAULT_FORMAT_ALLOCATE_STRING_LENGTH 512
char* FormatAllocateString(const char* format, ...)
{
    char* buffer = NULL;
    char* stringToReturn = NULL;
    int formatResult = 0;
    int sizeOfBuffer = DEFAULT_FORMAT_ALLOCATE_STRING_LENGTH;

    if (NULL == format) 
    {
        return stringToReturn;
    }

    while (NULL == stringToReturn)
    {
        if (NULL == (buffer = malloc(sizeOfBuffer)))
        {
            break;
        }

        memset(buffer, 0, sizeOfBuffer);

        va_list arguments;
        va_start(arguments, format);
        formatResult = vsnprintf(buffer, sizeOfBuffer, format, arguments);
        va_end(arguments);

        if ((formatResult > 0) && (formatResult < (int)sizeOfBuffer))
        {
            stringToReturn = DuplicateString(buffer);
            break;
        }

        FREE_MEMORY(buffer);
        sizeOfBuffer += 1;
    }

    FREE_MEMORY(buffer);

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

int ConvertStringsToIntegers(const char* source, char separator, int** integers, int* numIntegers, void* log)
{
    char* value = NULL;
    size_t sourceLength = 0;
    size_t i = 0;
    int status = 0;

    if ((NULL == source) || (NULL == integers) || (NULL == numIntegers))
    {
        OsConfigLogError(log, "ConvertSpaceSeparatedStringsToIntegers: invalid arguments");
        return EINVAL;
    }

    *integers = NULL;
    numIntegers = 0;

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
            RemovePrefixBlanks(value);
            TruncateAtFirst(value, separator);
            RemoveTrailingBlanks(value);

            if (0 == numIntegers)
            {
                *integers = (int*)malloc(sizeof(int));
            }
            else
            {
                *integers = realloc(*integers, size_t(numIntegers * sizeof(int)));
            }

            if (NULL == *integers)
            {
                OsConfigLogError(log, "ConvertSpaceSeparatedStringsToIntegers: failed to allocate memory");
                status = ENOMEM;
                break;
            }
            else
            {
                integers[numIntegers] = atoi(value);
                numIntegers += 1;
            }

            i += strlen(value);
            FREE_MEMORY(value);
            continue;
        }
    }

    OsConfigLogInfo(log, "ConvertStringsToIntegers: %d (%d integers converted from '%s' separated with '%c')", status, numIntegers, source, separator);

    return status;
}