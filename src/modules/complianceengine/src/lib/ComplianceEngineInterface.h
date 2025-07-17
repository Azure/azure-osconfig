// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_INTERFACE_H
#define COMPLIANCEENGINE_INTERFACE_H

#include "../inc/Mmi.h"

#include <Logging.h>

#ifdef __cplusplus
extern "C"
{
#endif

void ComplianceEngineInitialize(OsConfigLogHandle);
void ComplianceEngineShutdown();

MMI_HANDLE ComplianceEngineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void ComplianceEngineMmiClose(MMI_HANDLE clientSession);
int ComplianceEngineMmiGetInfo(const char* clientName, char** payload, int* payloadSizeBytes);
int ComplianceEngineMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes);
int ComplianceEngineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes);
void ComplianceEngineMmiFree(char* payload);

int ComplianceEngineValidatePayload(MMI_HANDLE clientSession, const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // COMPLIANCEENGINE_INTERFACE_H
