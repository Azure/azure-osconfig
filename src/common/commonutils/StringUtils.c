// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* GetOsPrettyName(OsConfigLogHandle log)
{
    const char* command = "cat /etc/os-release | grep PRETTY_NAME=";
    char* result = NULL;
    char* equals = NULL;
    char* end = NULL;

    if ((0 == ExecuteCommand(NULL, command, true, true, 0, 0, &result, NULL, log)) && result)
    {
        // Advance past the '=' character
        if (NULL != (equals = strchr(result, '=')))
        {
            memmove(result, equals + 1, strlen(equals));
        }

        // Strip leading/trailing whitespace and quotes
        while (result[0] == ' ' || result[0] == '\t' || result[0] == '"')
        {
            memmove(result, result + 1, strlen(result));
        }

        end = result + strlen(result);
        while (end > result && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '"' || *(end - 1) == '\n' || *(end - 1) == '\r'))
        {
            *(--end) = '\0';
        }
    }
    else
    {
        FREE_MEMORY(result);
    }

    OsConfigLogDebug(log, "OS pretty name: '%s'", result ? result : "(null)");
    return result;
}

char* DuplicateString(const char* source)
{
    if (NULL == source)
    {
        return NULL;
    }

    return strdup(source);
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
