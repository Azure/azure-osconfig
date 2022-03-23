// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <errno.h>

#include "OsInfo.h"

static char* g_osInfoModuleName = "OsInfo module";
static char* g_osInfoComponentName = "OsInfo";
static char* g_osNameObject = "OsName";
static char* g_osVersionObject = "OsVersion";
static char* g_cpuTypeObject = "Processor";
static char* g_kernelNameObject = "KernelName";
static char* g_kernelReleaseObject = "KernelRelease";
static char* g_kernelVersionObject = "KernelVersion";
static char* g_productNameObject = "ProductName";
static char* g_productVendorObject = "ProductVendor";

static const char* g_osInfoLogFile = "/var/log/osconfig_osinfo.log";
static const char* g_osInfoRolledLogFile = "/var/log/osconfig_osinfo.bak";

static const char* g_osInfoModuleInfo = "{\"Name\": \"OsInfo\","
    "\"Description\": \"Provides functionality to observe OS and device information\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"OsInfo\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static char* g_osName = NULL;
static char* g_osVersion = NULL;
static char* g_cpuType = NULL;
static char* g_kernelName = NULL;
static char* g_kernelRelease = NULL;
static char* g_kernelVersion = NULL;
static char* g_productName = NULL;
static char* g_productVendor = NULL;

static unsigned int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE OsInfoGetLog(void)
{
    return g_log;
}

void OsInfoInitialize(void)
{
    g_log = OpenLog(g_osInfoLogFile, g_osInfoRolledLogFile);

    g_osName = GetOsName(OsInfoGetLog());
    g_osVersion = GetOsVersion(OsInfoGetLog());
    g_cpuType = GetCpu(OsInfoGetLog());
    g_kernelName = GetOsKernelName(OsInfoGetLog());
    g_kernelRelease = GetOsKernelRelease(OsInfoGetLog());
    g_kernelVersion = GetOsKernelVersion(OsInfoGetLog());
    g_productVendor = GetProductVendor(OsInfoGetLog());
    g_productName = GetProductName(OsInfoGetLog());

    OsConfigLogInfo(OsInfoGetLog(), "%s initialized", g_osInfoModuleName);
}

void OsInfoShutdown(void)
{
    FREE_MEMORY(g_osName);
    FREE_MEMORY(g_osVersion);
    FREE_MEMORY(g_cpuType);
    FREE_MEMORY(g_kernelName);
    FREE_MEMORY(g_kernelRelease);
    FREE_MEMORY(g_kernelVersion);
    FREE_MEMORY(g_productVendor);
    FREE_MEMORY(g_productName);
    
    OsConfigLogInfo(OsInfoGetLog(), "%s shutting down", g_osInfoModuleName);
    
    CloseLog(&g_log);
}

MMI_HANDLE OsInfoMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_osInfoModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    g_referenceCount += 1;
    OsConfigLogInfo(OsInfoGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_osInfoModuleName, (char*)clientSession)) || (g_referenceCount <= 0) || (NULL == g_osName)) ? false : true;
}

void OsInfoMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        g_referenceCount -= 1;
        OsConfigLogInfo(OsInfoGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(OsInfoGetLog(), "MmiClose() called outside of a valid session");
    }
}

int OsInfoMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return status;
    }
    
    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_osInfoModuleInfo);
    
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_osInfoModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(OsInfoGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int OsInfoMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    char* value = NULL;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    
    if ((MMI_OK == status) && (strcmp(componentName, g_osInfoComponentName)))
    {
        OsConfigLogError(OsInfoGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    
    if (MMI_OK == status)
    {
        if (0 == strcmp(objectName, g_osNameObject))
        {
            value = g_osName;
        }
        else if (0 == strcmp(objectName, g_osVersionObject))
        {
            value = g_osVersion;
        }
        else if (0 == strcmp(objectName, g_cpuTypeObject))
        {
            value = g_cpuType;
        }
        else if (0 == strcmp(objectName, g_kernelNameObject))
        {
            value = g_kernelName;
        }
        else if (0 == strcmp(objectName, g_kernelReleaseObject))
        {
            value = g_kernelRelease;
        }
        else if (0 == strcmp(objectName, g_kernelVersionObject))
        {
            value = g_kernelVersion;
        }
        else if (0 == strcmp(objectName, g_productNameObject))
        {
            value = g_productName;
        }
        else if (0 == strcmp(objectName, g_productVendorObject))
        {
            value = g_productVendor;
        }
        else
        {
            OsConfigLogError(OsInfoGetLog(), "MmiGet called for an unsupported object name (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        // The string value is wrapped in "" and is not null terminated
        *payloadSizeBytes = strlen(value) + 2;
        
        if ((g_maxPayloadSizeBytes > 0) && (*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(OsInfoGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated", 
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
        if (*payload)
        {
            // snprintf counts in the null terminator for the target string (terminator that is excluded from payload)
            snprintf(*payload, *payloadSizeBytes + 1, "\"%s\"", value);
        }
        else
        {
            OsConfigLogError(OsInfoGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }    

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(OsInfoGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int OsInfoMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    OsConfigLogInfo(OsInfoGetLog(), "No desired objects, MmiSet not implemented");
    return EPERM;
}

void OsInfoMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
