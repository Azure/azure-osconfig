// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_URL_LENGTH 2048

static bool IsValidUrlCharacter(char c)
{
    return (isalnum(c) || ('-' == c) || ('_' == c) || ('.' == c) || ('~' == c) || ('%' == c) || ('\n' == c)) ? true : false;
}

bool IsValidUrl(const char *target)
{
    size_t length = 0, i = 0;
    bool result = true;

    if ((NULL == target) || (0 == (length = strnlen(target, MAX_URL_LENGTH))))
    {
        return false;
    }

    for (i = 0; i < length; i++)
    {
        if (false == IsValidUrlCharacter(target[i]))
        {
            result = false;
        }
    }
    
    return result;
}

char* UrlEncode(const char* target)
{
    size_t targetLength = 0, i = 0, j = 0;
    int encodedLength = 0;
    char* encodedTarget = NULL;

    if ((NULL == target) || (0 == (targetLength = strlen(target))))
    {
        return NULL;
    }

    encodedLength = (3 * targetLength) + 3;

    if (NULL != (encodedTarget = (char*)malloc(encodedLength)))
    {
        memset(encodedTarget, 0, encodedLength);

        for (i = 0; i < targetLength; i++)
        {
            if (isalnum(target[i]) || ('-' == target[i]) || ('_' == target[i]) || ('.' == target[i]) || ('~' == target[i]))
            {
                encodedTarget[j] = target[i];
                j += 1;
            }
            else if ('\n' == target[i])
            {
                memcpy(&encodedTarget[j], "%0A", sizeof("%0A"));
                j += strlen(&encodedTarget[j]);
            }
            else
            {
                snprintf(&encodedTarget[j], encodedLength - j, "%%%02X", target[i]);
                j += strlen(&encodedTarget[j]);
            }
        }
    }

    return encodedTarget;
}

char* UrlDecode(const char* target)
{
    size_t targetLength = 0, i = 0, j = 0;
    char buffer[3] = {0};
    unsigned int value = 0;
    char* decodedTarget = NULL;

    if (false == IsValidUrl(target))
    {
        return NULL;
    }

    if (NULL != (decodedTarget = (char*)malloc(targetLength + 3)))
    {
        memset(decodedTarget, 0, targetLength + 3);

        for (i = 0, j = 0; (i < targetLength) && (j < targetLength); i++)
        {
            if ((isalnum(target[j])) || ('-' == target[j]) || ('_' == target[j]) || ('.' == target[j]) || ('~' == target[j]))
            {
                decodedTarget[i] = target[j];
                j += 1;
            }
            else if ('%' == target[j])
            {
                if (((j + 2) < targetLength) && ('0' == target[j + 1]) && ('A' == toupper(target[j + 2])))
                {
                    decodedTarget[i] = '\n';
                }
                else
                {
                    memcpy(buffer, &target[j + 1], 2);
                    buffer[2] = 0;
                    
                    sscanf(buffer, "%x", &value);
                    snprintf(&decodedTarget[i], targetLength - i, "%c", value);
                }
                
                j += sizeof(buffer);
            }
        }
    }

    return decodedTarget;
}