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

static const char *g_requestBodyFormatMpiOpen = "{ \"Function\": \"MpiOpen\", \"Arguments\": { \"ClientName\": \"%s\", \"MaxPayloadSizeBytes\": %d } }";
static const char *g_requestBodyFormatMpiClose = "{ \"Function\": \"MpiClose\", \"Arguments\": { \"ClientName\": \"%s\" } }";
static const char *g_requestBodyFormatMpiGet = "{ \"Function\": \"MpiGet\", \"Arguments\": { \"ClientName\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\" } }";
static const char *g_requestBodyFormatMpiSet = "{ \"Function\": \"MpiSet\", \"Arguments\": { \"ClientName\": \"%s\", \"ComponentName\": \"%s\", \"ObjectName\": \"%s\", \"Payload\": %s } }";
static const char *g_requestBodyFormatMpiGetReported = "{ \"Function\": \"MpiGetReported\", \"Arguments\": { \"ClientName\": \"%s\" } }";
static const char *g_requestBodyFormatMpiSetDesired = "{ \"Function\": \"MpiSetDesired\", \"Arguments\": { \"ClientName\": \"%s\", \"Payload\": %s } }";

static const int g_bufferSize = 1024; //TODO replace this

int CallMpi(const char* requestBody, char** response, int* responseSize)
{
    const char* mpiSocket = "/run/osconfig/platformd.sock";
    const char* dataFormat = "POST /mpi HTTP/1.1\r\nHost: osconfig\r\nUser-Agent: osconfig\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s";
    const char* terminator = "\r\n\r\n";

    char data[g_bufferSize] = {0};
    struct sockaddr_un socketAddress = {0};
    socklen_t socketLength = 0;
    int responseLength = 0;
    int socket = -1;
    ssize_t bytes = 0;
    char* header = NULL;
    int status = MPI_OK;

    if ((NULL == requestBody) || (NULL == response) || (NULL == responseSize))
    {
        status = EINVAL;
        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) called with invalid arguments: %d", requestBody, response, responseSize, status);
        return status;
    }
    
    *response = NULL;
    *responseSize = 0;

    socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
    if (0 > socketHandle)
    {
        status = errno ? errno : EIO;
        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to open socket: %d", requestBody, response, responseSize, status);
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
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to connect to socket: %d", requestBody, response, responseSize, status);
        }
        else
        {
            snprintf(data, sizeof(data), dataFormat, strlen(requestBody), requestBody);
        }
    }

    if (MPI_OK == status)
    {
        bytes = send(socketHandle, data, strlen(data), 0);
        if (bytes != strlen(data))
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to send request to socket: %d", requestBody, response, responseSize, status);
        }
    }

    if (MPI_OK == status)
    {
        memset(data, 0, sizeof(data));
        bytes = 0;

        bytes = read(socketHandle, data, sizeof(data));
        if (0 < bytes)
        {
            status = errno ? errno : EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to read response from socket: %d", requestBody, response, responseSize, status);
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
                    OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response: %d", requestBody, response, responseSize, status);
                }
                else
                {
                    *response = (char*)malloc(responseLength + 1);
                    if (NULL == *response)
                    {
                        status = ENOMEM;
                        OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) failed to allocate memory for response: %d", requestBody, response, responseSize, status);
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
                OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response header: %d", requestBody, response, responseSize, status);
            }
        }
        else
        {
            status = EIO;
            OsConfigLogError(GetLog(), "CallMpi(%s, %p, %p) invalid response header: %d", requestBody, response, responseSize, status);
        }
    }

    if (0 <= socketHandle)
    {
        close(socketHandle);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s, %s, %s, %s, %d) returned %d", requestBody, *response, *responseSize);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "CallMpi(%s, %s, %p, %p, %d) returned %d", (void*)requestBody, response, *responseSize);
    }
    
    return status;
}

MPI_HANDLE CallMpiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MPI_HANDLE mpiHandle = MpiOpen(clientName, maxPayloadSizeBytes);
    OsConfigLogInfo(GetLog(), "MpiOpen(%s, %u): %p", clientName, maxPayloadSizeBytes, mpiHandle);
    return mpiHandle;
}

void CallMpiClose(MPI_HANDLE clientSession)
{
    OsConfigLogInfo(GetLog(), "MpiClose(%p)", clientSession);
    MpiClose(clientSession);
}

int CallMpiSet(const char* componentName, const char* propertyName, const MPI_JSON_STRING payload, const int payloadSizeBytes)
{
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
    int result = MPI_OK;

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

    result = MpiSetDesired(g_mpiHandle, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "MpiSetDesired(%p, %.*s, %d bytes) returned %d", g_mpiHandle, payloadSizeBytes, payload, payloadSizeBytes, result);
    }

    return MPI_OK;
}

int CallMpiGetReported(MPI_JSON_STRING* payload, int* payloadSizeBytes)
{
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