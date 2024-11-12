// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* UrlEncode(const char* target, size_t targetSize)
{
    size_t i = 0, j = 0;
    int encodedLength = 0;
    char* encodedTarget = NULL;

    if ((NULL == target) || (0 == targetSize))
    {
        return NULL;
    }

    encodedLength = (3 * targetSize) + 3;

    if (NULL != (encodedTarget = (char*)malloc(encodedLength)))
    {
        memset(encodedTarget, 0, encodedLength);

        for (i = 0; i < targetSize; i++)
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
                sprintf(&encodedTarget[j], "%%%02X", target[i]);
                j += strlen(&encodedTarget[j]);
            }
        }
    }

    return encodedTarget;
}

char* UrlDecode(const char* target, size_t targetSize)
{
    size_t i = 0, j = 0;
    char buffer[3] = {0};
    unsigned int value = 0;
    char* decodedTarget = NULL;

    if ((NULL == target) || (0 == targetSize))
    {
        return NULL;
    }

    if (NULL != (decodedTarget = (char*)malloc(targetSize + 3)))
    {
        memset(decodedTarget, 0, targetSize + 3);

        for (i = 0, j = 0; (i < targetSize) && (j < targetSize); i++)
        {
            if ((isalnum(target[j])) || ('-' == target[j]) || ('_' == target[j]) || ('.' == target[j]) || ('~' == target[j]))
            {
                decodedTarget[i] = target[j];
                j += 1;
            }
            else if ('%' == target[j])
            {
                if (((j + 2) < targetSize) && ('0' == target[j + 1]) && ('A' == toupper(target[j + 2])))
                {
                    decodedTarget[i] = '\n';
                }
                else
                {
                    memcpy(buffer, &target[j + 1], 2);
                    buffer[2] = 0;
                    
                    sscanf(buffer, "%x", &value);
                    snprintf(&decodedTarget[i], targetSize - i, "%c", value);
                }
                
                j += sizeof(buffer);
            }
        }
    }

    return decodedTarget;
}