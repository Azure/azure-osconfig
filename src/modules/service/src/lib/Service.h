// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ADHS_H
#define ADHS_H

#ifdef __cplusplus
extern "C"
{
#endif

void ServiceInitialize(const char* configFile);
void ServiceShutdown(void);

MMI_HANDLE ServiceMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void ServiceMmiClose(MMI_HANDLE clientSession);
int ServiceMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int ServiceMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int ServiceMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void ServiceMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // ADHS_H
