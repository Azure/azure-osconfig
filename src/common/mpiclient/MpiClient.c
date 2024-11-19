// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mpi.h>

#include "MpiClient.h"

#define MPI_MAX_CONTENT_LENGTH 64

#define HTTP_INTERNAL_SERVER_ERROR 500

extern MPI_HANDLE g_mpiHandle;

static int CallMpi(const char* name, const char* request, char** response, int* responseSize, void* log)
{
    const char* mpiSocket = "/run/osconfig/mpid.sock";
    const char* dataFormat = "POST /%s/ HTTP/1.1\r\nHost: OSConfig\r\nUser-Agent: OSConfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";

    int socketHandle = -1;
    char* data = {0};
    int estimatedDataSize = 0;
    int actualDataSize = 0;
    char contentLengthString[MPI_MAX_CONTENT_LENGTH] = {0};
    struct sockaddr_un socketAddress = {0};
    socklen_t socketLength = 0;
    ssize_t bytes = 0;
    int status = MPI_OK;
    int httpStatus = -1;

    if ((NULL == name) || (NULL == request) || (NULL == response) || (NULL == responseSize))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpi(%s): invalid arguments (%d)", name, status);
        return status;
    }

    *response = NULL;
    *responseSize = 0;

    if (0 != (status = CheckFileAccess(mpiSocket, 0, 0, 6770, NULL, IsFullLoggingEnabled() ? log : NULL)))
    {
        if (0 != (status = SetFileAccess(mpiSocket, 0, 0, 6770, IsFullLoggingEnabled() ? log : NULL)))
        {
            OsConfigLogError(log, "CallMpi(%s): access to the MPI socket is not protected, cannot call the MPI (%d)", name, status);
            return status;
        }
    }

    snprintf(contentLengthString, sizeof(contentLengthString), "%d", (int)strlen(request));
    estimatedDataSize = strlen(name) + strlen(dataFormat) + strlen(request) + strlen(contentLengthString) + 1;

    data = (char*)malloc(estimatedDataSize);
    if (NULL == data)
    {
        status = ENOMEM;
        OsConfigLogError(log, "CallMpi(%s): failed to allocate memory for request (%d)", name, status);
        return status;
    }

    memset(data, 0, estimatedDataSize);

    socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
    if (0 > socketHandle)
    {
        status = errno ? errno : EIO;
        OsConfigLogError(log, "CallMpi(%s): failed to open socket '%s' (%d)", name, mpiSocket, status);
    }
    else
    {
        memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sun_family = AF_UNIX;
        strncpy(socketAddress.sun_path, mpiSocket, sizeof(socketAddress.sun_path) - 1);
        socketLength = sizeof(socketAddress);
    }

    if (MPI_OK == status)
    {
        if (0 == connect(socketHandle, (struct sockaddr*)&socketAddress, socketLength))
        {
            snprintf(data, estimatedDataSize, dataFormat, name, strlen(request), request);
            actualDataSize = (int)strlen(data);
        }
        else
        {
            status = errno ? errno : EIO;
            OsConfigLogError(log, "CallMpi(%s): failed to connect to socket '%s' (%d)", name, mpiSocket, status);
        }
    }

    if (MPI_OK == status)
    {
        bytes = send(socketHandle, data, actualDataSize, 0);
        if (bytes != actualDataSize)
        {
            status = errno ? errno : EIO;
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "CallMpi(%s): failed to send request '%s' (%d bytes) to socket '%s' (%d)", name, data, actualDataSize, mpiSocket, status);
            }
            else
            {
                OsConfigLogError(log, "CallMpi(%s): failed to send request to socket '%s' of %d bytes (%d)", name, mpiSocket, actualDataSize, status);
            }
        }
        else if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "CallMpi(%s): sent to '%s' '%s' (%d bytes)", name, mpiSocket, data, actualDataSize);
        }
    }

    FREE_MEMORY(data);

    if (MPI_OK == status)
    {
        httpStatus = ReadHttpStatusFromSocket(socketHandle, log);
        status = (200 == httpStatus) ? MPI_OK : httpStatus;

        *responseSize = ReadHttpContentLengthFromSocket(socketHandle, log);
        *response = (char*)malloc(*responseSize + 1);
        if (NULL != *response)
        {
            memset(*response, 0, *responseSize + 1);

            if (*responseSize != read(socketHandle, *response, *responseSize))
            {
                if ((MPI_OK == status) || IsFullLoggingEnabled())
                {
                    OsConfigLogError(log, "CallMpi(%s): failed to read %d bytes response from socket '%s' (%d)", name, *responseSize, mpiSocket, status);
                }
                status = errno ? errno : EIO;
            }
        }
        else
        {
            status = ENOMEM;
            *responseSize = 0;
            OsConfigLogError(log, "CallMpi(%s): failed to allocate memory for response (%d)", name, status);
        }
    }

    if (0 <= socketHandle)
    {
        close(socketHandle);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CallMpi(name: '%s', request: '%s', response: '%s', response size: %d bytes) to socket '%s' returned %d",
            name, request, *response, *responseSize, mpiSocket, status);
    }

    return status;
}

static char* ParseString(void* log, char* jsonString)
{
    JSON_Value* jsonValue = NULL;
    const char* parsedValue = NULL;
    char* returnValue = NULL;

    jsonValue = json_parse_string(jsonString);
    if (NULL == jsonValue)
    {
        OsConfigLogError(log, "ParseString: json_parse_string on '%s' failed", jsonString);
    }
    else
    {
        parsedValue = json_value_get_string(jsonValue);
        if (NULL == parsedValue)
        {
            OsConfigLogError(log, "ParseString: json_value_get_string on '%s' failed", jsonString);
        }
        else
        {
            returnValue = strdup(parsedValue);
            if (NULL == returnValue)
            {
                OsConfigLogError(log, "ParseString: strdup on '%s' failed", parsedValue);
            }
        }
        json_value_free(jsonValue);
    }

    return returnValue;
}

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes, void* log)
{
    const char *name = "MpiOpen";
    const char *requestBodyFormat = "{ \"ClientName\": \"%s\", \"MaxPayloadSizeBytes\": %d }";

    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;
    MPI_HANDLE mpiHandle = NULL;
    char maxPayloadSizeBytesString[MPI_MAX_CONTENT_LENGTH] = {0};
    char* mpiHandleValue = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(log, "CallMpiOpen: invalid NULL MPI handle");
        return NULL;
    }

    snprintf(maxPayloadSizeBytesString, sizeof(maxPayloadSizeBytesString), "%d", maxPayloadSizeBytes);
    requestSize = strlen(requestBodyFormat) + strlen(clientName) + strlen(maxPayloadSizeBytesString) + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(log, "CallMpiOpen(%s, %u): failed to allocate memory for request", clientName, maxPayloadSizeBytes);
        return NULL;
    }

    snprintf(request, requestSize, requestBodyFormat, clientName, maxPayloadSizeBytes);

    status = CallMpi(name, request, &response, &responseSize, log);

    FREE_MEMORY(request);

    mpiHandle = (MPI_OK == status) ? (MPI_HANDLE)response : NULL;

    if ((NULL != mpiHandle) && (NULL == (mpiHandleValue = ParseString(log, (char*)mpiHandle))))
    {
        OsConfigLogError(log, "CallMpiOpen: invalid MPI handle '%s'", (char*)mpiHandle);
        mpiHandle = NULL;
    }

    OsConfigLogInfo(log, "CallMpiOpen(%s, %u): %p ('%s')", clientName, maxPayloadSizeBytes, mpiHandle, mpiHandleValue);

    FREE_MEMORY(mpiHandleValue);

    // The MPI handle returned here is a JSON string already wrapped in ""
    return mpiHandle;
}

void CallMpiClose(MPI_HANDLE clientSession, void* log)
{
    const char *name = "MpiClose";
    const char *requestBodyFormat = "{ \"ClientSession\": %s }";

    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;

    if ((NULL == clientSession) || (0 == strlen((char*)clientSession)))
    {
        OsConfigLogError(log, "CallMpiClose(%p) called with invalid argument", clientSession);
        return;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)clientSession) + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(log, "CallMpiClose(%p): failed to allocate memory for request", clientSession);
        return;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)clientSession);

    CallMpi(name, request, &response, &responseSize, log);

    FREE_MEMORY(request);
    FREE_MEMORY(response);

    OsConfigLogInfo(log, "CallMpiClose(%p)", clientSession);
}

int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes, void* log)
{
    const char *name = "MpiSet";
    static const char *requestBodyFormat = "{ \"ClientSession\": %s, \"ComponentName\": \"%s\", \"ObjectName\": \"%s\", \"Payload\": %s }";

    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;
    char* statusFromResponse = NULL;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(log, "CallMpiSet: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpiSet: invalid arguments (%d)", status);
        return status;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + strlen(componentName) + strlen(propertyName) + payloadSizeBytes + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(log, "CallMpiSet(%s, %s): failed to allocate memory for request (%d)", componentName, propertyName, status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, componentName, propertyName, payload);

    status = CallMpi(name, request, &response, &responseSize, log);

    FREE_MEMORY(request);

    if ((NULL != response) && (responseSize > 0))
    {
        statusFromResponse = ParseString(log, response);
        status = (NULL == statusFromResponse) ? EINVAL : atoi(statusFromResponse);
        FREE_MEMORY(statusFromResponse);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CallMpiSet(%p, %s, %s, %.*s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogInfo(log, "CallMpiSet(%p, %s, %s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, status);
    }

    return status;
};

int CallMpiGet(const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes, void* log)
{
    const char *name = "MpiGet";
    const char *requestBodyFormat = "{ \"ClientSession\": %s, \"ComponentName\": \"%s\", \"ObjectName\": \"%s\" }";

    char* request = NULL;
    int requestSize = 0;
    int status = MPI_OK;
    char* statusFromResponse = NULL;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(log, "CallMpiGet: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpiGet: called with invalid arguments (%d)", status);
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + strlen(componentName) + strlen(propertyName) + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(log, "CallMpiGet(%s, %s): failed to allocate memory for request (%d)", componentName, propertyName, status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, componentName, propertyName);

    status = CallMpi(name, request, payload, payloadSizeBytes, log);

    FREE_MEMORY(request);

    if (HTTP_INTERNAL_SERVER_ERROR == status)
    {
        if ((NULL != *payload) && (*payloadSizeBytes > 0))
        {
            statusFromResponse = ParseString(log, *payload);
            status = (NULL == statusFromResponse) ? EINVAL : atoi(statusFromResponse);
            FREE_MEMORY(statusFromResponse);

            FREE_MEMORY(*payload);
            *payloadSizeBytes = 0;
        }
        else
        {
            OsConfigLogError(log, "CallMpiGet(%s, %s): invalid response for HTTP internal server error (500)", componentName, propertyName);
            status = EINVAL;
        }
    }
    else if ((NULL != *payload) && (*payloadSizeBytes != (int)strlen(*payload)))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpiGet(%s, %s): invalid response (%d)", componentName, propertyName, status);

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CallMpiGet(%p, %s, %s, %.*s, %d bytes): %d", g_mpiHandle, componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
};

int CallMpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes, void* log)
{
    const char *name = "MpiSetDesired";
    static const char *requestBodyFormat = "{ \"ClientSession\": %s, \"Payload\": %s }";

    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;
    char* statusFromResponse = NULL;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(log, "CallMpiSetDesired: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == payload) || (0 >= payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpiSetDesired: invalid arguments (%d)", status);
        return status;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + payloadSizeBytes + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(log, "CallMpiSetDesired: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, payload);

    status = CallMpi(name, request, &response, &responseSize, log);

    FREE_MEMORY(request);

    if ((NULL != response) && (responseSize > 0))
    {
        statusFromResponse = ParseString(log, response);
        status = (NULL == statusFromResponse) ? EINVAL : atoi(statusFromResponse);
        FREE_MEMORY(statusFromResponse);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CallMpiSetDesired(%p, %.*s, %d bytes) returned %d", g_mpiHandle, payloadSizeBytes, payload, payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogInfo(log, "CallMpiSetDesired(%p, %d bytes) returned %d", g_mpiHandle, payloadSizeBytes, status);
    }

    return status;
}

int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes, void* log)
{
    const char *name = "MpiGetReported";
    static const char *requestBodyFormat = "{ \"ClientSession\": %s }";

    char* request = NULL;
    int requestSize = 0;
    int status = MPI_OK;
    char* statusFromResponse = NULL;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(log, "CallMpiGetReported: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(log, "CallMpiGetReported: called with invalid arguments (%d)", status);
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(log, "CallMpiGetReported: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle);

    status = CallMpi(name, request, payload, payloadSizeBytes, log);

    FREE_MEMORY(request);

    if (HTTP_INTERNAL_SERVER_ERROR == status)
    {
        if ((NULL != *payload) && (*payloadSizeBytes > 0))
        {
            statusFromResponse = ParseString(log, *payload);
            status = (NULL == statusFromResponse) ? EINVAL : atoi(statusFromResponse);
            FREE_MEMORY(statusFromResponse);
        }
        else
        {
            OsConfigLogError(log, "CallMpiGetReported: invalid response for HTTP internal server error (500)");
            status = EINVAL;
        }

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }
    else if ((NULL != *payload) && (*payloadSizeBytes != (int)strlen(*payload)))
    {
        OsConfigLogError(log, "CallMpiGetReported: invalid response (%p, %d)", *payload, *payloadSizeBytes);

        status = EINVAL;

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CallMpiGetReported(%p, %.*s, %d bytes): %d", g_mpiHandle, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

void CallMpiFree(MPI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
