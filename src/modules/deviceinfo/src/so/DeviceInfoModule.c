// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include <DeviceInfo.h>

void __attribute__((constructor)) InitModule(void)
{
    DeviceInfoInitialize();
}

void __attribute__((destructor)) DestroyModule(void)
{
    DeviceInfoShutdown();
}

// This module implements one global static session for all clients. This allows the MMI implementation
// to be placed in the static module library and the module to get increased unit-test coverage.
// The module SO library remains a simple wrapper for the MMI calls without any additional implementation.

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return DeviceInfoMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return DeviceInfoMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return DeviceInfoMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return DeviceInfoMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return DeviceInfoMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return DeviceInfoMmiFree(payload);
}
