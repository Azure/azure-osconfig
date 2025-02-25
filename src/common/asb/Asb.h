// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ASB_H
#define ASB_H

#ifdef __cplusplus
extern "C"
{
#endif

int AsbIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OSCONFIG_LOG_HANDLE log);

void AsbInitialize(OSCONFIG_LOG_HANDLE log);
void AsbShutdown(OSCONFIG_LOG_HANDLE log);

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OSCONFIG_LOG_HANDLE log);
int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log);

#ifdef __cplusplus
}
#endif

#endif // ASB_H
