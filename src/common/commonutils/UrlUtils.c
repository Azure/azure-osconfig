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

        for (i = 0, j = 0; (i < targetSize) && (j < encodedLength - 1); i++)
        {
            if (isalnum(target[i]) || ('-' == target[i]) || ('_' == target[i]) || ('.' == target[i]) || ('~' == target[i]))
            {
                encodedTarget[j++] = target[i];
            }
            else if ('\n' == target[i])
            {
                if ((j + 3) < encodedLength)
                {
                    memcpy(&encodedTarget[j], "%0A", 3);
                    j += 3;
                }
                else
                {
                    FREE_MEMORY(encodedTarget);
                    break;
                }
            }
            else
            {
                if ((j + 3) < encodedLength)
                {
                    snprintf(&encodedTarget[j], 4, "%%%02X", (unsigned char)target[i]);
                    j += 3;
                }
                else
                {
                    FREE_MEMORY(encodedTarget);
                    break;
                }
            }
        }

        if (NULL != encodedTarget)
        {
            encodedTarget[j] = 0;
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
            if (isalnum(target[j]) || ('-' == target[j]) || ('_' == target[j]) || ('.' == target[j]) || ('~' == target[j]))
            {
                decodedTarget[i] = target[j];
                j += 1;
            }
            else if ('%' == target[j])
            {
                if (((j + 2) < targetSize) && isxdigit(target[j + 1]) && isxdigit(target[j + 2]))
                {
                    buffer[0] = target[j + 1];
                    buffer[1] = target[j + 2];
                    buffer[2] = 0;
                    sscanf(buffer, "%x", &value);
                    decodedTarget[i] = (char)value;
                    j += 3;
                }
                else
                {
                    FREE_MEMORY(decodedTarget);
                    break;
                }
            }
            else
            {
                FREE_MEMORY(decodedTarget);
                break;
            }
        }

        if (NULL != decodedTarget)
        {
            decodedTarget[i] = 0;
        }
    }

    return decodedTarget;
}
