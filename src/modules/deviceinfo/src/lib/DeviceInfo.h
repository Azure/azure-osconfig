// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#ifdef __cplusplus
extern "C"
{
#endif

void DeviceInfoInitialize(void);
void DeviceInfoShutdown(void);

MMI_HANDLE DeviceInfoMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void DeviceInfoMmiClose(MMI_HANDLE clientSession);
int DeviceInfoMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int DeviceInfoMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int DeviceInfoMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void DeviceInfoMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // DEVICEINFO_H
