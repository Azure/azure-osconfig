// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <OsInfo.h>
//#include <rapidjson/document.h>

void __attribute__((constructor)) InitModule(void)
{
    OsConfigLogInfo(OsInfoGetLog(), "%s module loaded", OSINFO);
}

void __attribute__((destructor)) DestroyModule(void)
{
    OsConfigLogInfo(OsInfoGetLog(), "%s module unloaded", OSINFO);
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return OsInfoGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return NULL;
}

void MmiClose(MMI_HANDLE clientSession)
{

}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return -1; //EPERM;
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = OsInfoGet(clientSession, componentName, objectName, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(OsInfoGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
        else
        {
            OsConfigLogError(OsInfoGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}