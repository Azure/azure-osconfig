// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef HOSTNAME_H
#define HOSTNAME_H

void HostNameInitialize(void);
void HostNameShutdown(void);

MMI_HANDLE HostNameMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void HostNameMmiClose(MMI_HANDLE clientSession);
int HostNameMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int HostNameMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int HostNameMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void HostNameMmiFree(MMI_JSON_STRING payload);

#endif // HOSTNAME_H
