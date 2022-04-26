// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpUtils.h"
#include "inc/MpiProxy.h"

// Read-only properties need to use the following format for IoT Hub:
// - Simple type:  {"ComponentName":{"__t":"c", "PropertyName" : PropertyValue}}
// - Complex type: {"ComponentName":{"__t":"c", "PropertyName" : {"NameOne":"ValueOne", "NameTwo" : 2, "NameThree" : 3, "NameFour" : "ValueFour", "NameFive" : 5}}}
//
// The MPI will deliver and accept values in the following format (with the component and property names separately submitted):
// - Simple type:  {PropertyValue}
// - Complex type: {"NameOne":"ValueOne", "NameTwo" : 2, "NameThree" : 3, "NameFour" : "ValueFour", "NameFive" : 5}
//
// Read-only properties are updated from device to IoT Hub (MPI GET).
// Writeable properties are updated from IoT Hub to device (MPI SET) and acknowledged back to IoT Hub.

extern MPI_HANDLE g_mpiHandle;

char g_mpiCall[MPI_CALL_MESSAGE_LENGTH] = {0};
static const char g_mpiCallTemplate[] = " during %s to %s.%s\n";

static const int g_bufferSize = 1024; //TODO replace this

int CallMpi(const char* request, char** response, int* responseSize)
{
    const char* mpiSocket = "/run/osconfig/platformd.sock";
    const char* dataFormat = "POST /mpi HTTP/1.1\r\nHost: osconfig\r\nUser-Agent: osconfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";
    const char* terminator = "\r\n\r\n";

    char* data = {0};
    int dataSize = 0;
    struct sockaddr_un socketAddress = {0};
    socklen_t socketLength = 0;
    int responseLength = 0;
    int socket = -1;
    ssize_t bytes = 0;
    char* header = NULL;
    int status = MPI_OK;

    if ((NULL == request) || (NULL == response) || (NULL == responseSize))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) called with invalid arguments: %d", request, response, responseSize, status);
        return status;
    }
    
    *response = NULL;
    *responseSize = 0;

    dataSize = strlen(dataFormat) + strlen(request) + 10;
    data = (char*)malloc(dataSize);
    if (NULL == data)
    {
        status = ENOMEM;
        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to allocate memory for request: %d", request, response, responseSize, status);
        return status;
    }

    socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
    if (0 > socketHandle)
    {
        status = errno ? errno : EIO;
        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to open socket: %d", request, response, responseSize, status);
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
        if (0 != connect(socketHandle, (struct socksocketAddress*)&socketAddress, socketLength))
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to connect to socket: %d", request, response, responseSize, status);
        }
        else
        {
            snprintf(data, dataSize, dataFormat, strlen(request), request);
        }
    }

    if (MPI_OK == status)
    {
        bytes = send(socketHandle, data, strlen(data), 0);
        if (bytes != strlen(data))
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to send request to socket: %d", request, response, responseSize, status);
        }
    }

    if (MPI_OK == status)
    {
        //TODO: replace the buffer size may be troo small for response
        // stream socket - we can read in a loop until done
        bytes = read(socketHandle, buffer, sizeof(buffer));
        if (0 < bytes)
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to read response from socket: %d", request, response, responseSize, status);
        }
    }

    if (MPI_OK == status)
    {
        if (header = strstr(buffer, "HTTP/1.1 20"))
        {
            if (header = strstr(buffer, terminator))
            {
                header += strlen(terminator);

                responseLength = ((start > &buffer[0]) && (bytes > (start - &buffer[0])) ? (bytes - (start - &buffer[0])) : 0;
                if (0 == responseLength)
                {
                    status = EIO;
                    OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response: %d", request, response, responseSize, status);
                }
                else
                {
                    *response = (char*)malloc(responseLength + 1);
                    if (NULL == *response)
                    {
                        status = ENOMEM;
                        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to allocate memory for response: %d", request, response, responseSize, status);
                    }
                    else
                    {
                        memset(*response, 0, responseLength + 1);
                        strncpy(*response, header, responseLength);
                        *responseSize = responseLength;
                    }
                }
            }
            else
            {
                status = EIO;
                OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response header: %d", request, response, responseSize, status);
            }
        }
        else
        {
            status = EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response header: %d", request, response, responseSize, status);
        }
    }

    if (0 <= socketHandle)
    {
        close(socketHandle);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s, %s, %s, %s, %d) returned %d", request, *response, *responseSize);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s, %s, %p, %p, %d) returned %d", (void*)request, response, *responseSize);
    }
    
    return status;
}

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    const char *requestBodyFormatMpiOpen = "{ \"Function\": \"MpiOpen\", \"Arguments\": { \"ClientName\": \"%s\", \"MaxPayloadSizeBytes\": %d } }";
    char* request = NULL; 
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;
    MPI_HANDLE mpiHandle = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(GetLog(), "MpiOpen(%s, %u) called with invalid arguments", clientName, maxPayloadSizeBytes);
        return NULL;
    }
    
    requestSize = strlen(requestBodyFormatMpiOpen) + strlen(clientName) + 10;
    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(GetLog(), "Failed to allocate memory for CallMpiOpen request");
        return NULL;
    }

    snprintf(request, requestSize, requestBodyFormatMpiOpen, clientName, maxPayloadSizeBytes);

    status = CallMpi(request, &response, &responseSize);

    FREE_MEMORY(request);

    mpiHandle = (MPI_OK == status) ? (MPI_HANDLE)response : NULL;
    
    //MPI_HANDLE mpiHandle = MpiOpen(clientName, maxPayloadSizeBytes);
    OsConfigLogInfo(GetLog(), "MpiOpen(%s, %u): %p", clientName, maxPayloadSizeBytes, mpiHandle);
    return mpiHandle;
}

void CallMpiClose(MPI_HANDLE clientSession)
{
    const char *requestBodyFormatMpiClose = "{ \"Function\": \"MpiClose\", \"Arguments\": { \"ClientSession\": \"%s\" } }";
    char* request = NULL; 
    char *response = NULL;
    int requestSize = 0;
    int responseSize = 0;
    int status = MPI_OK;

    if ((NULL == clientSession) || (strlen((char*)clientSession))
    {
        OsConfigLogError(GetLog(), "MpiClose(%p) called with invalid argument", clientSession);
        return;
    }

    requestSize = strlen(requestBodyFormatMpiClose) + 10;
    request = (char*)malloc(requestSize);
    if (NULL == request)
    {
        OsConfigLogError(GetLog(), "Failed to allocate memory for CallMpiClose request");
        return NULL;
    }

    snprintf(request, requestSize, requestBodyFormatMpiClose, (char*)clientSession);

    CallMpi(request, &response, &responseSize);

    FREE_MEMORY(request);
    FREE_MEMORY(response);
    
    //MpiClose(clientSession);
    OsConfigLogInfo(GetLog(), "MpiClose(%p)", clientSession);
}

int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    static const char *requestBodyFormatMpiSet = "{ \"Function\": \"MpiSet\", \"Arguments\": { \"ClientName\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\", \"Payload\": %s } }";

    int result = MPI_OK;

    LogAssert(GetLog(), NULL != g_mpiHandle);
    LogAssert(GetLog(), NULL != componentName);
    LogAssert(GetLog(), NULL != propertyName);
    LogAssert(GetLog(), NULL != payload);
    LogAssert(GetLog(), 0 < payloadSizeBytes);

    memset(g_mpiCall, 0, sizeof(g_mpiCall));

    if (NULL == g_mpiHandle)
    {
        result = EPERM;
        OsConfigLogError(GetLog(), "Cannot call MpiSet without an MPI handle, %d", result);
        return result;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        result = EINVAL;
        OsConfigLogError(GetLog(), "Invalid argument(s), cannot call MpiSet, %d", result);
        return result;
    }

    snprintf(g_mpiCall, sizeof(g_mpiCall), g_mpiCallTemplate, "MpiSet", componentName, propertyName);

    if (IsValidMimObjectPayload(payload, payloadSizeBytes, GetLog()))
    {
        result = MpiSet(g_mpiHandle, componentName, propertyName, payload, payloadSizeBytes);
    }
    else
    {
        // Error is logged by IsValidMimObjectPayload
        result = EINVAL;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiSet(%p, %s, %s, %.*s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, payload, payloadSizeBytes, result);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "MpiSet(%p, %s, %s, %d bytes) returned %d", g_mpiHandle, componentName, propertyName, payloadSizeBytes, result);
    }

    memset(g_mpiCall, 0, sizeof(g_mpiCall));

    return result;
};

int CallMpiGet(const char* componentName, const char* propertyName, MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    const char *requestBodyFormatMpiGet = "{ \"Function\": \"MpiGet\", \"Arguments\": { \"ClientName\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\" } }";

    int result = MPI_OK;

    LogAssert(GetLog(), NULL != g_mpiHandle);
    LogAssert(GetLog(), NULL != componentName);
    LogAssert(GetLog(), NULL != propertyName);
    LogAssert(GetLog(), NULL != payload);
    LogAssert(GetLog(), NULL != payloadSizeBytes);

    memset(g_mpiCall, 0, sizeof(g_mpiCall));

    if (NULL == g_mpiHandle)
    {
        result = EPERM;
        OsConfigLogError(GetLog(), "Cannot call MpiGet without an MPI handle, %d", result);
        return result;
    }

    if ((NULL == componentName) || (NULL == propertyName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        result = EINVAL;
        OsConfigLogError(GetLog(), "Invalid argument(s), cannot call MpiGet, %d", result);
        return result;
    }

    snprintf(g_mpiCall, sizeof(g_mpiCall), g_mpiCallTemplate, "MpiGet", componentName, propertyName);

    *payload = NULL;
    *payloadSizeBytes = 0;

    result = MpiGet(g_mpiHandle, componentName, propertyName, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiGet(%p, %s, %s, %.*s, %d bytes): %d", g_mpiHandle, componentName, propertyName, *payloadSizeBytes, *payload, *payloadSizeBytes, result);
    }

    if (!IsValidMimObjectPayload(*payload, *payloadSizeBytes, GetLog()))
    {
        // Error is logged by IsValidMimObjectPayload
        result = EINVAL;

        CallMpiFree(*payload);
        *payload = NULL;
        *payloadSizeBytes = 0;
    }

    memset(g_mpiCall, 0, sizeof(g_mpiCall));

    return result;
};

int CallMpiSetDesired(const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
    static const char *requestBodyFormatMpiSetDesired = "{ \"Function\": \"MpiSetDesired\", \"Arguments\": { \"ClientName\": \"%s\", \"Payload\": %s } }";
    char request[g_bufferSize] = {0};
    char* data = NULL;
    char *response = NULL;
    int result = MPI_OK;
    int responseSize = 0;
    
    if (NULL == g_mpiHandle)
    {
        result = EPERM;
        OsConfigLogError(GetLog(), "Cannot call MpiSetDesired without an MPI handle, %d", result);
        return result;
    }

    LogAssert(GetLog(), NULL != payload);
    LogAssert(GetLog(), 0 < payloadSizeBytes);

    if ((NULL == payload) || (0 >= payloadSizeBytes))
    {
        result = EINVAL;
        OsConfigLogError(GetLog(), "Invalid argument(s), cannot call MpiSetDesired, %d", result);
        return result;
    }

    data = (char*)malloc(payloadSizeBytes + 1);
    if (NULL == data)
    {
        result = ENOMEM;
        OsConfigLogError(GetLog(), "Failed to allocate memory for MpiSetDesired");
        return result;
    }
    
    memset(data, 0, payloadSizeBytes + 1);
    strncpy(data, payload, payloadSizeBytes);
    
    snprintf(request, sizeof(request), requestBodyFormatMpiSetDesired, g_clientName, payload, data);

    result = CallMpi(request, &response, &responseSize);

    if (MPI_OK == result)
    {
        status = ((NULL != response) && (0 < responseSize)) ? atoi(response) : EIO;
    }

    //result = MpiSetDesired(g_mpiHandle, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiSetDesired(%p, %.*s, %d bytes) returned %d", g_mpiHandle, payloadSizeBytes, payload, payloadSizeBytes, result);
    }

    return MPI_OK;
}

int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
    static const char *requestBodyFormatMpiGetReported = "{ \"Function\": \"MpiGetReported\", \"Arguments\": { \"ClientName\": \"%s\" } }";

    int result = MPI_OK;

    if (NULL == g_mpiHandle)
    {
        result = EPERM;
        OsConfigLogError(GetLog(), "Cannot call MpiGetReported without an MPI handle, %d", result);
        return result;
    }

    LogAssert(GetLog(), NULL != payload);
    LogAssert(GetLog(), NULL != payloadSizeBytes);

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        result = EINVAL;
        OsConfigLogError(GetLog(), "Invalid argument(s), cannot call MpiGetReported, %d", result);
        return result;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    result = MpiGetReported(g_mpiHandle, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiGetReported(%p, %.*s, %d bytes)", g_mpiHandle, *payloadSizeBytes, *payload, *payloadSizeBytes);
    }

    return result;
}

void CallMpiFree(MPI_JSON_STRING payload)
{
    if (NULL != payload)
    {
        MpiFree(payload);
    }
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