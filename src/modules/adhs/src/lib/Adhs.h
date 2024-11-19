// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ADHS_H
#define ADHS_H

#ifdef __cplusplus
extern "C"
{
#endif

void AdhsInitialize(const char* configFile);
void AdhsShutdown(void);

MMI_HANDLE AdhsMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void AdhsMmiClose(MMI_HANDLE clientSession);
int AdhsMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int AdhsMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int AdhsMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void AdhsMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // ADHS_H
