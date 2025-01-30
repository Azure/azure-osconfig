// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
#include <stdatomic.h>
#endif

#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "DeviceInfo.h"

static const char* g_deviceInfoModuleName = "DeviceInfo module";
static const char* g_deviceInfoComponentName = "DeviceInfo";
static const char* g_osNameObject = "osName";
static const char* g_osVersionObject = "osVersion";
static const char* g_cpuTypeObject = "cpuType";
static const char* g_cpuVendorObject = "cpuVendorId";
static const char* g_cpuModelObject = "cpuModel";
static const char* g_totalMemoryObject = "totalMemory";
static const char* g_freeMemoryObject = "freeMemory";
static const char* g_kernelNameObject = "kernelName";
static const char* g_kernelReleaseObject = "kernelRelease";
static const char* g_kernelVersionObject = "kernelVersion";
static const char* g_productVendorObject = "productVendor";
static const char* g_productNameObject = "productName";
static const char* g_productVersionObject = "productVersion";
static const char* g_systemCapabilitiesObject = "systemCapabilities";
static const char* g_systemConfigurationObject = "systemConfiguration";
static const char* g_osConfigVersionObject = "osConfigVersion";

static const char* g_deviceInfoLogFile = "/var/log/osconfig_deviceinfo.log";
static const char* g_deviceInfoRolledLogFile = "/var/log/osconfig_deviceinfo.bak";

static const char* g_deviceInfoModuleInfo = "{\"Name\": \"DeviceInfo\","
    "\"Description\": \"Provides functionality to observe device information\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 3,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"DeviceInfo\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static char* g_osName = NULL;
static char* g_osVersion = NULL;
static char* g_cpuType = NULL;
static char* g_cpuVendor = NULL;
static char* g_cpuModel = NULL;
static long g_totalMemory = 0;
static long g_freeMemory = 0;
static char* g_kernelName = NULL;
static char* g_kernelRelease = NULL;
static char* g_kernelVersion = NULL;
static char* g_productName = NULL;
static char* g_productVendor = NULL;
static char* g_productVersion = NULL;
static char* g_systemCapabilities = NULL;
static char* g_systemConfiguration = NULL;

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
static atomic_int g_referenceCount = 0;
#else
static int g_referenceCount = 0;
#endif

static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE DeviceInfoGetLog(void)
{
    return g_log;
}

void DeviceInfoInitialize(void)
{
    g_log = OpenLog(g_deviceInfoLogFile, g_deviceInfoRolledLogFile);

    g_osName = GetOsName(DeviceInfoGetLog());
    g_osVersion = GetOsVersion(DeviceInfoGetLog());
    g_cpuType = GetCpuType(DeviceInfoGetLog());
    g_cpuVendor = GetCpuVendor(DeviceInfoGetLog());
    g_cpuModel = GetCpuModel(DeviceInfoGetLog());
    g_totalMemory = GetTotalMemory(DeviceInfoGetLog());
    g_freeMemory = GetFreeMemory(DeviceInfoGetLog());
    g_kernelName = GetOsKernelName(DeviceInfoGetLog());
    g_kernelRelease = GetOsKernelRelease(DeviceInfoGetLog());
    g_kernelVersion = GetOsKernelVersion(DeviceInfoGetLog());
    g_productVendor = GetProductVendor(DeviceInfoGetLog());
    g_productName = GetProductName(DeviceInfoGetLog());
    g_productVersion = GetProductVersion(DeviceInfoGetLog());
    g_systemCapabilities = GetSystemCapabilities(DeviceInfoGetLog());
    g_systemConfiguration = GetSystemConfiguration(DeviceInfoGetLog());

    OsConfigLogInfo(DeviceInfoGetLog(), "%s initialized", g_deviceInfoModuleName);
}

void DeviceInfoShutdown(void)
{
    FREE_MEMORY(g_osName);
    FREE_MEMORY(g_osVersion);
    FREE_MEMORY(g_cpuType);
    FREE_MEMORY(g_cpuVendor);
    FREE_MEMORY(g_cpuModel);
    FREE_MEMORY(g_kernelName);
    FREE_MEMORY(g_kernelRelease);
    FREE_MEMORY(g_kernelVersion);
    FREE_MEMORY(g_productVendor);
    FREE_MEMORY(g_productName);
    FREE_MEMORY(g_productVersion);
    FREE_MEMORY(g_systemCapabilities);
    FREE_MEMORY(g_systemConfiguration);

    OsConfigLogInfo(DeviceInfoGetLog(), "%s shutting down", g_deviceInfoModuleName);

    CloseLog(&g_log);
}

MMI_HANDLE DeviceInfoMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_deviceInfoModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(DeviceInfoGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_deviceInfoModuleName, (char*)clientSession)) || (g_referenceCount <= 0) || (NULL == g_osName)) ? false : true;
}

void DeviceInfoMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(DeviceInfoGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(DeviceInfoGetLog(), "MmiClose() called outside of a valid session");
    }
}

int DeviceInfoMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_deviceInfoModuleInfo);

    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_deviceInfoModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(DeviceInfoGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(DeviceInfoGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int DeviceInfoMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    char* value = NULL;
    bool isStringValue = true;
    char buffer[10] = {0};

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(DeviceInfoGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(DeviceInfoGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (strcmp(componentName, g_deviceInfoComponentName)))
    {
        OsConfigLogError(DeviceInfoGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
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
        else if (0 == strcmp(objectName, g_cpuVendorObject))
        {
            value = g_cpuVendor;
        }
        else if (0 == strcmp(objectName, g_cpuModelObject))
        {
            value = g_cpuModel;
        }
        else if (0 == strcmp(objectName, g_totalMemoryObject))
        {
            isStringValue = false;
            snprintf(buffer, sizeof(buffer), "%lu", g_totalMemory);
            value = buffer;
        }
        else if (0 == strcmp(objectName, g_freeMemoryObject))
        {
            // Refresh this one at every MmiGet
            g_freeMemory = GetFreeMemory(DeviceInfoGetLog());

            isStringValue = false;
            snprintf(buffer, sizeof(buffer), "%lu", g_freeMemory);
            value = buffer;
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
        else if (0 == strcmp(objectName, g_productVendorObject))
        {
            value = g_productVendor;
        }
        else if (0 == strcmp(objectName, g_productNameObject))
        {
            value = g_productName;
        }
        else if (0 == strcmp(objectName, g_productVersionObject))
        {
            value = g_productVersion;
        }
        else if (0 == strcmp(objectName, g_systemCapabilitiesObject))
        {
            value = g_systemCapabilities;
        }
        else if (0 == strcmp(objectName, g_systemConfigurationObject))
        {
            value = g_systemConfiguration;
        }
        else if (0 == strcmp(objectName, g_osConfigVersionObject))
        {
            value = OSCONFIG_VERSION;
        }
        else
        {
            OsConfigLogError(DeviceInfoGetLog(), "MmiGet called for an unsupported object name (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        // The string value (can be empty string) is wrapped in "" and is not null terminated
        *payloadSizeBytes = (value ? strlen(value) : 0) + (isStringValue ? 2 : 0);

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(DeviceInfoGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated",
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes + 1);
        if (*payload)
        {
            // snprintf counts in the null terminator for the target string (terminator that is excluded from payload)
            snprintf(*payload, *payloadSizeBytes + 1, (isStringValue ? "\"%s\"" : "%s"), value ? value : "");
        }
        else
        {
            OsConfigLogError(DeviceInfoGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(DeviceInfoGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int DeviceInfoMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    OsConfigLogInfo(DeviceInfoGetLog(), "No desired objects, MmiSet not implemented");

    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return EPERM;
}

void DeviceInfoMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
