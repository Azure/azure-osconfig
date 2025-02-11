// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceInterface.h"
#include "CommonUtils.h"

void ComplianceInitialize(void* log)
{
    UNUSED(log);
}

void ComplianceShutdown(void)
{
}

MMI_HANDLE ComplianceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    UNUSED(clientName);
    UNUSED(maxPayloadSizeBytes);
    return nullptr;
}

void ComplianceMmiClose(MMI_HANDLE clientSession)
{
    UNUSED(clientSession);
}

int ComplianceMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes)
{
    UNUSED(clientName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);
    return -1;
}

int ComplianceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);
    return -1;
}

int ComplianceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);
    return -1;
}

void ComplianceMmiFree(char* payload)
{
    UNUSED(payload);
}
