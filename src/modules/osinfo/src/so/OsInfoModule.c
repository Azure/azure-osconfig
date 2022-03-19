// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include <OsInfo.h>

void __attribute__((constructor)) InitModule(void)
{
    OsInfoInitialize();
}

void __attribute__((destructor)) DestroyModule(void)
{
    OsInfoShutdown();
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return OsInfoMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return OsInfoMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return OsInfoMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return OsInfoMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return OsInfoMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return OsInfoMmiFree(payload);
}