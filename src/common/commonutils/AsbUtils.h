// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ASB_UTILS_H
#define ASB_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

void AsbInitialize(void* log);
void AsbShutdown(void* log);

int AsbMmiGet(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, void* log);
int AsbMmiSet(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes, void* log);

#ifdef __cplusplus
}
#endif

#endif // ASB_UTILS_H
