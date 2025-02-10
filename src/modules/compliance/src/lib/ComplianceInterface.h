// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_INTERFACE_H
#define COMPLIANCE_INTERFACE_H

#include "../inc/Mmi.h"

#ifdef __cplusplus
extern "C"
{
#endif

void ComplianceInitialize(void*);
void ComplianceShutdown();

MMI_HANDLE ComplianceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void ComplianceMmiClose(MMI_HANDLE clientSession);
int ComplianceMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes);
int ComplianceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes);
int ComplianceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes);
void ComplianceMmiFree(char* payload);

#ifdef __cplusplus
}
#endif

#endif // COMPLIANCE_INTERFACE_H
