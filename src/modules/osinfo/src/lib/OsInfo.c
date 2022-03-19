// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "OsInfo.h"

#define OSINFO_MODULE_INFO "({\"Name\": \"OsInfo\",\"Description\": \"Provides functionality to observe OS and device information\",\"Manufacturer\": \"Microsoft\",\"VersionMajor\": 1,\"VersionMinor\": 0,\"VersionInfo\": \"Copper\",\"Components\": [\"OsInfo\"],\"Lifetime\": 2,\"UserAccount\": 0})"

#define OSINFO_LOG_FILE "/var/log/osconfig_osinfo.log"
#define OSINFO_ROLLED_LOG_FILE "/var/log/osconfig_osinfo.bak"

static char* g_osName = NULL;
static char* g_osVersion = NULL;
static char* g_cpuType = NULL;
static char* g_kernelName = NULL;
static char* g_kernelRelease = NULL;
static char* g_kernelVersion = NULL;
static char* g_productName = NULL;
static char* g_productVendor = NULL;

static int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE g_log = NULL;

static int g_referenceCount = 0;

static char* g_osInfo = "OsInfo module session";

OSCONFIG_LOG_HANDLE OsInfoGetLog(void)
{
    return g_log;
}

void Initialize(void)
{
    g_log = OpenLog(OSINFO_LOG_FILE, OSINFO_ROLLED_LOG_FILE);
    
    g_osName = GetOsName(OsInfoGetLog());
    g_osVersion = GetOsVersion(OsInfoGetLog());
    g_cpuType = GetCpu(OsInfoGetLog());
    g_kernelName = GetOsKernelName(OsInfoGetLog());
    g_kernelRelease = GetOsKernelRelease(OsInfoGetLog());
    g_kernelVersion = GetOsKernelVersion(OsInfoGetLog());
    g_productVendor = GetProductVendor(OsInfoGetLog());
    g_productName = GetProductName(OsInfoGetLog());

    OsConfigLogInfo(OsInfoGetLog(), "%s module initialized", g_osInfo);
}

void Shutdown(void)
{
    FREE_MEMORY(g_osName);
    FREE_MEMORY(g_osVersion);
    FREE_MEMORY(g_cpuType);
    FREE_MEMORY(g_kernelName);
    FREE_MEMORY(g_kernelRelease);
    FREE_MEMORY(g_kernelVersion);
    FREE_MEMORY(g_productVendor);
    FREE_MEMORY(g_productName);
    
    OsConfigLogInfo(OsInfoGetLog(), "%s module shutdown", g_osInfo);
    
    CloseLog(&g_log);
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_osInfo, (char*)clientSession)) || (g_referenceCount = < 0)) ? false : true;
}

MMI_HANDLE OsInfoOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_osInfo;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    g_referenceCount += 1;
    OsConfigLogInfo(OsInfoGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

void OsInfoClose(MMI_HANDLE clientSession)
{
    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiClose() called outside of a valid session");
        return;
    }

    g_referenceCount -= 1;
    OsConfigLogInfo(OsInfoGetLog(), "MmiClose()");
}

int OsInfoGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    if ((NULL == payload) || (payloadSizeBytes))
    {
        return EINVAL;
    }
    
    int status = MMI_OK;
    int size = (int)strlen(OSINFO_MODULE_INFO);

    *payload = NULL;
    *payloadSizeBytes = 0;
    
    *payload = (MMI_JSON_STRING)malloc(size);
    if (*payload)
    {
        memcpy(*payload, OSINFO_MODULE_INFO, size);
        *payloadSizeBytes = size;
    }
    else
    {
        status = ENOMEM;
        OsConfigLogError(OsInfoGetLog(), "MmiGetInfo: failed to allocate %d bytes", size);
    }
    
    if (MMI_OK == status)
    {
        OsConfigLogInfo(OsInfoGetLog(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGetInfo(%s, %.*s, %d) failed with %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int OsInfoGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (payloadSizeBytes))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        return EINVAL;
    }
    else if (!IsValidSession(clientSession))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        return EINVAL;
    }
}

void OsInfoFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
