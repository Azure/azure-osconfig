// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <OsInfo.h>
//#include <rapidjson/document.h>

void __attribute__((constructor)) InitModule(void)
{
    OpenLog();
    OsConfigLogInfo(GetLog(), "%s module loaded", OSINFO);
}

void __attribute__((destructor)) DestroyModule(void)
{
    OsConfigLogInfo(GetLog(), "%s module unloaded", OSINFO);
    CloseLog();
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return GetInfo(clientName, payload, payloadSizeBytes);
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
    return EPERM;
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = Get(clientSession, componentName, objectName, payload, payloadSizeBytes);

    if (IsFullLoggingEnabled())
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(GetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
        else
        {
            OsConfigLogError(GetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
        }
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (!payload)
    {
        return;
    }
    delete[] payload;
}