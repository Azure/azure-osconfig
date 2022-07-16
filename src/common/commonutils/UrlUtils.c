// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* UrlEncode(char* target)
{
    if (NULL == target)
    {
        return NULL;
    }

    int i = 0, j = 0;
    int targetLength = (int)strlen(target);
    int encodedLength = 3 * targetLength;
    char* encodedTarget = (char*)malloc(encodedLength);
    if (NULL != encodedTarget)
    {
        memset(encodedTarget, 0, encodedLength);

        for (i = 0; i < targetLength; i++)
        {
            if ((isalnum(target[i])) || ('-' == target[i]) || ('_' == target[i]) || ('.' == target[i]) || ('~' == target[i]))
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

char* UrlDecode(char* target)
{
    int i = 0, j = 0;
    char buffer[3] = {0};
    unsigned int value = 0;
    int targetLength = 0;
    char* decodedTarget = NULL;

    if (NULL == target)
    {
        return NULL;
    }

    targetLength = (int)strlen(target);
    
    // The length of the decoded string is the same as the length of the encoded string or smaller
    decodedTarget = (char*)malloc(targetLength + 1);
    if (NULL != decodedTarget)
    {
        memset(decodedTarget, 0, targetLength + 1);

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
                    sprintf(&decodedTarget[i], "%c", value);
                }
                
                j += sizeof(buffer);
            }
        }
    }

    return decodedTarget;
}