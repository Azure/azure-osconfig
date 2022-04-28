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
    MpiApiInitialize();
}

void CallMpiShutdown(void)
{
    OsConfigLogInfo(GetLog(), "Calling MpiShutdown");
    MpiApiShutdown();
    MpiShutdown();
}