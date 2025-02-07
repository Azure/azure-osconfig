// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceInterface.hpp"

// ---------------------------------------------------------------
//                  MMI module C interface
// ---------------------------------------------------------------

void ComplianceInitialize(void* log)
{
    (void)log;
}

void ComplianceShutdown(void)
{
}

MMI_HANDLE ComplianceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    (void)clientName;
    (void)maxPayloadSizeBytes;
    return nullptr;
}

void ComplianceMmiClose(MMI_HANDLE clientSession)
{
    (void)clientSession;
}

int ComplianceMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes)
{
    (void)clientName;
    (void)payload;
    (void)payloadSizeBytes;
    return -1;
}

int ComplianceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes)
{
    (void)clientSession;
    (void)componentName;
    (void)objectName;
    (void)payload;
    (void)payloadSizeBytes;
    return -1;
}

int ComplianceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes)
{
    (void)clientSession;
    (void)componentName;
    (void)objectName;
    (void)payload;
    (void)payloadSizeBytes;
    return -1;
}

void ComplianceMmiFree(char* payload)
{
    (void)payload;
}
