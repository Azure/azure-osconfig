// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ANSIBLE_H
#define ANSIBLE_H

#ifdef __cplusplus
extern "C"
{
#endif

void AnsibleInitialize(void);
void AnsibleShutdown(void);

MMI_HANDLE AnsibleMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void AnsibleMmiClose(MMI_HANDLE clientSession);
int AnsibleMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int AnsibleMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int AnsibleMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void AnsibleMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // ANSIBLE_H
