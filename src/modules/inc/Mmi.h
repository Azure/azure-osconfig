// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MMI_H
#define MMI_H

typedef void* MMI_HANDLE;

// Plus any error codes from errno.h
#define MMI_OK 0

// Not null terminated, UTF-8, JSON formatted string
typedef char* MMI_JSON_STRING;

#ifdef __cplusplus
extern "C"
{
#endif

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes);
MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes);
void MmiClose(MMI_HANDLE clientSession);
int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes);
int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes);
void MmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // MMI_H
