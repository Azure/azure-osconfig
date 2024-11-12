// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* UrlEncode(const char* target)
{
    size_t targetSize = 0, encodedLength = 0, i = 0, j = 0;
    char* encodedTarget = NULL;

    if ((NULL == target) || (0 == (targetSize = strlen(target))))
    {
        return NULL;
    }

    encodedLength = (3 * targetSize) + 1;

    if (NULL != (encodedTarget = (char*)malloc(encodedLength)))
    {
        memset(encodedTarget, 0, encodedLength);

        for (i = 0, j = 0; (i < targetSize) && (j < encodedLength); i++)
        {
            if (isalnum(target[i]) || ('-' == target[i]) || ('_' == target[i]) || ('.' == target[i]) || ('~' == target[i]))
            {
                encodedTarget[j] = target[i];
                j += 1;
            }
            else if ('\n' == target[i])
            {
                memcpy(&encodedTarget[j], "%0A", 3);
                j += 3;
            }
            else
            {
                sprintf(&encodedTarget[j], "%%%02X", target[i]);
                j += 3;
            }
        }
    }

    return encodedTarget;
}

char* UrlDecode(const char* target)
{
    size_t targetSize = 0, i = 0, j = 0;
    char buffer[3] = {0};
    unsigned int value = 0;
    char* decodedTarget = NULL;

    if ((NULL == target) || (0 == (targetSize = strlen(target))))
    {
        return NULL;
    }

    if (NULL != (decodedTarget = (char*)malloc(targetSize + 1)))
    {
        memset(decodedTarget, 0, targetSize + 1);

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
                    sprintf(&decodedTarget[i], "%c", value);
                }
                
                j += sizeof(buffer);
            }
        }
    }

    return decodedTarget;
}