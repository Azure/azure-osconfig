// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include "ComplianceInterface.h"
#include <stddef.h>

void __attribute__((constructor)) InitModule(void)
{
    ComplianceInitialize(NULL);
}

void __attribute__((destructor)) DestroyModule(void)
{
    ComplianceShutdown(NULL);
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return ComplianceMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return ComplianceMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return ComplianceMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return ComplianceMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return ComplianceMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return ComplianceMmiFree(payload);
}
