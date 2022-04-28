// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpUtils.h"
#include "inc/MpiProxy.h"

extern MPI_HANDLE g_mpiHandle;

// TBD - to be moved to Platform
// Use these as following in the process that makes MPI calls:
// At the beginning and end of each call:
// memset(g_mpiCall, 0, sizeof(g_mpiCall));
// At the beginning of the MPI execution within each call, for example:
// snprintf(g_mpiCall, sizeof(g_mpiCall), g_mpiCallTemplate, "MpiGet", componentName, propertyName);
char g_mpiCall[MPI_CALL_MESSAGE_LENGTH] = {0};

int CallMpi(const char* name, const char* request, char** response, int* responseSize)
{
    const char* mpiSocket = "/run/osconfig/platformd.sock";
    const char* dataFormat = "POST /%s/ HTTP/1.1\r\nHost: osconfig\r\nUser-Agent: osconfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";
    
    int socketHandle = -1;
    char* data = {0};
    int dataSize = 0;
    char contentLengthString[50] = {0};
    struct sockaddr_un socketAddress = {0};
    socklen_t socketLength = 0;
    ssize_t bytes = 0;
    int status = MPI_OK;
    int httpStatus = -1;
    

    if ((NULL == name) || (NULL == request) || (NULL == response) || (NULL == responseSize))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpi(%s): invalid arguments (%d)", name, status);
        return status;
    }
    
    *response = NULL;
    *responseSize = 0;

    snprintf(contentLengthString, sizeof(contentLengthString), "%d", strlen(request));
    dataSize = strlen(dataFormat) + strlen(request) + contentLengthString;

    data = (char*)malloc(dataSize);
    if (NULL == data)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpi(%s): failed to allocate memory for request (%d)", name, status);
        return status;
    }

    socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
    if (0 > socketHandle)
    {
        status = errno ? errno : EIO;
        OsConfigLogError(GetLog(), "CallMpi(%s): failed to open socket (%d)", name, status);
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
            snprintf(data, dataSize, dataFormat, name, strlen(request), request);
        }
        else
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s): failed to connect to socket (%d)", name, status);
        }
    }

    if (MPI_OK == status)
    {
        bytes = send(socketHandle, data, strlen(data), 0);
        if (bytes != (int)strlen(data))
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi (%s): failed to send request to socket (%d)", name, status);
        }
    }

    FREE_MEMORY(data);

    if (MPI_OK == status)
    {
        httpStatus = ReadHttpStatusFromSocket(socketHandle, GetLog());
        status = (200 == httpStatus) ? MPI_OK : httpStatus;
    }

    if (MPI_OK == status)
    {
        *responseSize = ReadHttpContentLengthFromSocket(socketHandle, GetLog()) + 1;
        *response = (char*)malloc(*responseSize);
        if (NULL != *response)
        {
            if ((*responseSize - 1) != read(socketHandle, *response, *responseSize - 1))
            {
                status = errno ? errno : EIO;
                OsConfigLogError(GetLog(), "CallMpi(%s): failed to read %d bytes response from socket (%d)", name, *responseSize - 1, status);
            }
        }
        else
        {
            status = ENOMEM;
            *responseSize = 0;
            OsConfigLogError(GetLog(), "CallMpi(%s): failed to allocate memory for response (%d)", name, status);
        }
    }

    if (0 <= socketHandle)
    {
        close(socketHandle);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s, %s, %s, %d) returned %d", name, request, *response, *responseSize, status);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s) returned %d response bytes and %d", name, *responseSize, status);
    }
    
    return status;
}

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    const char *name = "MpiOpen";
    const char *requestBodyFormat = "{ \"ClientName\": \"%s\", \"MaxPayloadSizeBytes\": %d }";
    
    char* request = NULL; 
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;
    MPI_HANDLE mpiHandle = NULL;
    char maxPayloadSizeBytesString[30] = {0};

    if (NULL == clientName)
    {
        OsConfigLogError(GetLog(), "CallMpiOpen: invalid arguments");
        return NULL;
    }

    snprintf(maxPayloadSizeBytesString, sizeof(maxPayloadSizeBytesString), "%d", maxPayloadSizeBytes);
    requestSize = strlen(requestBodyFormat) + strlen(clientName) + strlen(maxPayloadSizeBytesString);

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(GetLog(), "CallMpiOpen: failed to allocate memory for request");
        return NULL;
    }

    snprintf(request, requestSize, requestBodyFormat, clientName, maxPayloadSizeBytes);

    status = CallMpi(name, request, &response, &responseSize);

    FREE_MEMORY(request);

    mpiHandle = (MPI_OK == status) ? (MPI_HANDLE)response : NULL;
    
    OsConfigLogInfo(GetLog(), "MpiOpen(%s, %u): %p", clientName, maxPayloadSizeBytes, mpiHandle);

    return mpiHandle;
}

void CallMpiClose(MPI_HANDLE clientSession)
{
    const char *name = "MpiClose";
    const char *requestBodyFormat = "{ \"ClientSession\": \"%s\" }";
    
    char* request = NULL; 
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;

    if ((NULL == clientSession) || (0 == strlen((char*)clientSession)))
    {
        OsConfigLogError(GetLog(), "CallMpiClose(%p) called with invalid argument", clientSession);
        return;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)clientSession);

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(GetLog(), "CallMpiClose: failed to allocate memory for request");
        return;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)clientSession);

    CallMpi(name, request, &response, &responseSize);

    FREE_MEMORY(request);
    FREE_MEMORY(response);
    
    OsConfigLogInfo(GetLog(), "MpiClose(%p)", clientSession);
}

int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    const char *name = "MpiSet";
    static const char *requestBodyFormat = "{ \"ClientSession\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\", \"Payload\": %s }";

    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(GetLog(), "CallMpiSet: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpiSet: invalid arguments (%d)", status);
        return status;
    }

    if (!IsValidMimObjectPayload(payload, payloadSizeBytes, GetLog()))
    {
        // Error is logged by IsValidMimObjectPayload
        return EINVAL;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + strlen(componentName) + strlen(propertyName) + payloadSizeBytes + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpiSet: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, componentName, propertyName, payload);

    status = CallMpi(name, request, &response, &responseSize);

    FREE_MEMORY(request);

    if ((NULL != response) && (responseSize > 0))
    {
        status = atoi(response);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiSet(%p, %s, %s, %.*s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "MpiSet(%p, %s, %s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, status);
    }

    return status;
};

int CallMpiGet(const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    const char *name = "MpiGet";
    const char *requestBodyFormat = "{ \"ClientSession\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\" }";

    char* request = NULL;
    int requestSize = 0;
    int status = MPI_OK;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(GetLog(), "CallMpiGet: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpiGet: called with invalid arguments (%d)", status);
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + strlen(componentName) + strlen(propertyName);

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpiGet: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, componentName, propertyName);

    status = CallMpi(name, request, payload, payloadSizeBytes);

    FREE_MEMORY(request);

    if ((NULL == *payload) || (*payloadSizeBytes != (int)strlen(*payload)))
    {
        OsConfigLogError(GetLog(), "CallMpiGet: invalid response");

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiGet(%p, %s, %s, %.*s, %d bytes): %d", g_mpiHandle, componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    if (!IsValidMimObjectPayload(*payload, *payloadSizeBytes, GetLog()))
    {
        // Error is logged by IsValidMimObjectPayload
        status = EINVAL;

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }

    return status;
};

int CallMpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    const char *name = "MpiSetDesired";
    static const char *requestBodyFormat = "{ \"ClientName\": \"%s\", \"Payload\": %s }";
    
    char* request = NULL;
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(GetLog(), "CallMpiSettDesired: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == payload) || (0 >= payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpiSettDesired: invalid arguments (%d)", status);
        return status;
    }

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle) + payloadSizeBytes + 1;

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpiSettDesired: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle, payload);

    status = CallMpi(name, request, &response, &responseSize);

    FREE_MEMORY(request);

    if ((NULL != response) && (responseSize > 0))
    {
        status = atoi(response);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiSettDesired(%p, %.*s, %d bytes) returned %d", g_mpiHandle, payloadSizeBytes, payload, payloadSizeBytes, status);
    }

    return status;
}

int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    const char *name = "MpiGetReported";
    static const char *requestBodyFormat = "{ \"ClientSession\": \"%s\" }";

    char* request = NULL;
    int requestSize = 0;
    int status = MPI_OK;

    if ((NULL == g_mpiHandle) || (0 == strlen((char*)g_mpiHandle)))
    {
        status = EPERM;
        OsConfigLogError(GetLog(), "CallMpiGetReported: called without a valid MPI handle (%d)", status);
        return status;
    }

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpiGetReported: called with invalid arguments (%d)", status);
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    requestSize = strlen(requestBodyFormat) + strlen((char*)g_mpiHandle);

    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpiGetReported: failed to allocate memory for request (%d)", status);
        return status;
    }

    snprintf(request, requestSize, requestBodyFormat, (char*)g_mpiHandle);

    status = CallMpi(name, request, payload, payloadSizeBytes);

    FREE_MEMORY(request);

    if ((NULL == *payload) || (*payloadSizeBytes != (int)strlen(*payload)))
    {
        OsConfigLogError(GetLog(), "CallMpiGetReported: invalid response");

        FREE_MEMORY(*payload);
        *payloadSizeBytes = 0;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiGetReported(%p, %.*s, %d bytes): %d", g_mpiHandle, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

void CallMpiFree(MPI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}

void CallMpiDoWork(void)
{
    MpiDoWork();
}

void CallMpiInitialize(void)
{
    OsConfigLogInfo(GetLog(), "Calling MpiInitialize");
    MpiInitialize();
}

void CallMpiShutdown(void)
{
    OsConfigLogInfo(GetLog(), "Calling MpiShutdown");
    MpiShutdown();
}