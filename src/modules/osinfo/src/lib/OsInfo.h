// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef OSINFO_H
#define OSINFO_H

#ifdef __cplusplus
extern "C"
{
#endif

void OsInfoInitialize(void);
void OsInfoShutdown(void);
                  
MMI_HANDLE OsInfoMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void OsInfoMmiClose(MMI_HANDLE clientSession);
int OsInfoMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int OsInfoMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int OsInfoMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void OsInfoMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // OSINFO_H
