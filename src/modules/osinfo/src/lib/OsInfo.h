// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <errno.h>

#ifndef OSINFO_H
#define OSINFO_H

#define OSINFO "OsInfo"
#define OSINFO_OS_NAME "OsName"
#define OSINFO_OS_VERSION "OsVersion"
#define OSINFO_PROCESSOR "Processor"
#define OSINFO_KERNEL_NAME "KernelName"
#define OSINFO_KERNEL_VERSION "KernelVersion"
#define OSINFO_PRODUCT_VENDOR "ProductVendor"
#define OSINFO_PRODUCT_NAME "ProductName"

#ifdef __cplusplus
extern "C"
{
#endif

OSCONFIG_LOG_HANDLE OsInfoGetLog(void);
             
void Initialize(void);
void Shutdown(void);
                  
MMI_HANDLE OsInfoOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void OsInfoClose(MMI_HANDLE clientSession);
int OsInfoGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int OsInfoGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
void OsInfoFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // OSINFO_H
