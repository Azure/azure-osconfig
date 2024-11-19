// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include <SecurityBaseline.h>

void __attribute__((constructor)) InitModule(void)
{
    SecurityBaselineInitialize();
}

void __attribute__((destructor)) DestroyModule(void)
{
    SecurityBaselineShutdown();
}

// This module implements one global static session for all clients. This allows the MMI implementation
// to be placed in the static module library and the module to get increased unit-test coverage.
// The module SO library remains a simple wrapper for the MMI calls without any additional implementation.

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return SecurityBaselineMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return SecurityBaselineMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return SecurityBaselineMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return SecurityBaselineMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return SecurityBaselineMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return SecurityBaselineMmiFree(payload);
}
