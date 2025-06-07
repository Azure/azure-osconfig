// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SECURITY_BASELINE_H
#define SECURITY_BASELINE_H

#ifdef __cplusplus
extern "C"
{
#endif

void SecurityBaselineInitialize(void);
void SecurityBaselineShutdown(void);

MMI_HANDLE SecurityBaselineMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void SecurityBaselineMmiClose(MMI_HANDLE clientSession);
int SecurityBaselineMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int SecurityBaselineMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int SecurityBaselineMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void SecurityBaselineMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // SECURITY_BASELINE_H
