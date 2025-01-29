// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define MAX_MPI_URI_LENGTH 32

static char* ReadUntilStringFound(int socketHandle, const char* what, void* log)
{
    char* found = NULL;
    char* buffer = NULL;
    int size = 1;

    if ((NULL == what) || (socketHandle < 0))
    {
        OsConfigLogError(log, "ReadUntilStringFound: invalid arguments");
        return NULL;
    }

    buffer = (char*)malloc(size + 1);
    if (NULL == buffer)
    {
        OsConfigLogError(log, "ReadUntilStringFound: out of memory allocating initial buffer");
        return NULL;
    }

    memset(buffer, 0, size + 1);

    while (1 == read(socketHandle, &(buffer[size - 1]), 1))
    {
        if (NULL != (found = strstr(buffer, what)))
        {
            buffer[size] = 0;
            break;
        }
        else
        {
            size += 1;
            buffer = (char*)realloc(buffer, size + 1);
            if (NULL == buffer)
            {
                OsConfigLogError(log, "ReadUntilStringFound: out of memory reallocating buffer");
                break;
            }
            else
            {
                buffer[size] = 0;
            }
        }
    }

    if (NULL == found)
    {
        FREE_MEMORY(buffer);
    }

    return buffer;
}

char* ReadUriFromSocket(int socketHandle, void* log)
{
    const char* postPrefix = "POST /";
    char* returnUri = NULL;
    char* buffer = NULL;
    char bufferUri[MAX_MPI_URI_LENGTH] = {0};
    int uriLength = 0;
    size_t i = 0;

    if (socketHandle < 0)
    {
        OsConfigLogError(log, "ReadUriFromSocket: invalid socket (%d)", socketHandle);
        return NULL;
    }

    buffer = ReadUntilStringFound(socketHandle, postPrefix, log);
    if (NULL == buffer)
    {
        OsConfigLogError(log, "ReadUriFromSocket: '%s' prefix not found", postPrefix);
        return NULL;
    }

    FREE_MEMORY(buffer);

    for (i = 0; i < sizeof(bufferUri); i++)
    {
        if (1 == read(socketHandle, &(bufferUri[i]), 1))
        {
            if (isalpha(bufferUri[i]))
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }

    uriLength = i;

    returnUri = (char*)malloc(uriLength + 1);
    if (NULL != returnUri)
    {
        memset(returnUri, 0, uriLength + 1);
        strncpy(returnUri, bufferUri, uriLength);

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "ReadUriFromSocket: %s", returnUri);
        }
    }
    else
    {
        OsConfigLogError(log, "ReadUriFromSocket: out of memory");
    }

    return returnUri;
}

int ReadHttpStatusFromSocket(int socketHandle, void* log)
{
    const char* httpPrefix = "HTTP/1.1";

    int httpStatus = 404;
    char* buffer = NULL;
    char status[4] = {0};
    char expectedSpace = 'x';

    if (socketHandle < 0)
    {
        OsConfigLogError(log, "ReadHttpStatusFromSocket: invalid socket (%d)", socketHandle);
        return httpStatus;
    }

    buffer = ReadUntilStringFound(socketHandle, httpPrefix, log);
    if (NULL == buffer)
    {
        OsConfigLogError(log, "ReadHttpStatusFromSocket: '%s' prefix not found", httpPrefix);
        return httpStatus;
    }

    if ((1 == read(socketHandle, &expectedSpace, 1)) && (' ' == expectedSpace) &&
        (3 == read(socketHandle, status, 3)) && (isdigit(status[0]) && (status[0] >= '1') && (status[0] <= '5') && isdigit(status[1]) && isdigit(status[2])))
    {
        httpStatus = atoi(status);

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "ReadHttpStatusFromSocket: %d ('%s')", httpStatus, status);
        }
    }

    FREE_MEMORY(buffer);

    return httpStatus;
}

int ReadHttpContentLengthFromSocket(int socketHandle, void* log)
{
    const char* contentLengthLabel = "Content-Length: ";
    const char* doubleTerminator = "\r\n\r\n";

    int httpContentLength = 0;
    char* buffer = NULL;
    char* contentLength = NULL;
    char isolatedContentLength[64] = {0};
    size_t i = 0;

    if (socketHandle < 0)
    {
        OsConfigLogError(log, "ReadHttpContentLengthFromSocket: invalid socket (%d)", socketHandle);
        return httpContentLength;
    }

    buffer = ReadUntilStringFound(socketHandle, doubleTerminator, log);
    if (NULL != buffer)
    {
        contentLength = strstr(buffer, contentLengthLabel);
        if (NULL != contentLength)
        {
            contentLength += strlen(contentLengthLabel);

            for (i = 0; i < sizeof(isolatedContentLength) - 1; i++)
            {
                if (isdigit(contentLength[i]))
                {
                    isolatedContentLength[i] = contentLength[i];
                }
                else
                {
                    break;
                }
            }

            if (isdigit(isolatedContentLength[0]))
            {
                httpContentLength = atoi(isolatedContentLength);

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(log, "ReadHttpContentLengthFromSocket: %d ('%s')", httpContentLength, isolatedContentLength);
                }
            }
        }

        FREE_MEMORY(buffer);
    }

    return httpContentLength;
}
