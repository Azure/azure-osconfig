// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "Configuration.h"

static const char* g_configurationModuleName = "OSConfig Configuration module";
static const char* g_configurationComponentName = "OsConfigConfiguration";
//static const char* g_desiredConfigurationObject = "desiredConfiguration";
static const char* g_modelVersionObject = "modelVersion";
static const char* g_refreshIntervalObject = "refreshInterval";
static const char* g_localManagementEnabledObject = "localManagementEnabled";
static const char* g_fullLoggingEnabledObject = "fullLoggingEnabled";
static const char* g_commandLoggingEnabledObject = "commandLoggingEnabled";
static const char* g_iotHubProtocolObject = "iotHubProtocol";

static const char* g_osConfigConfigurationFile = "/etc/osconfig/osconfig.json";

static const char* g_configurationLogFile = "/var/log/osconfig_configuration.log";
static const char* g_configurationRolledLogFile = "/var/log/osconfig_configuration.bak";

static const char* g_configurationModuleInfo = "{\"Name\": \"OsConfigConfiguration\","
    "\"Description\": \"Provides functionality to manage OSConfig configuration on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Nickel\","
    "\"Components\": [\"OsConfigConfiguration\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static OSCONFIG_LOG_HANDLE g_log = NULL;

//static char* g_desiredConfiguration = NULL; //needed? Not really

static int g_modelVersion = DEFAULT_DEVICE_MODEL_ID;
static int g_refreshInterval = DEFAULT_REPORTING_INTERVAL;
static bool g_localManagementEnabled = false;
static bool g_fullLoggingEnabled = false;
static bool g_commandLoggingEnabled = false;
static int g_iotHubProtocol = 0;

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE ConfigurationGetLog(void)
{
    return g_log;
}

void ConfigurationInitialize(void)
{
    g_log = OpenLog(g_configurationLogFile, g_configurationRolledLogFile);

    char* jsonConfiguration = LoadStringFromFile(g_osConfigConfigurationFile, false, ConfigurationGetLog());
    if (NULL != jsonConfiguration)
    {
        g_modelVersion = GetModelVersionFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_refreshInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_localManagementEnabled = GetLocalManagementFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_iotHubProtocol = GetIotHubProtocolFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        FREE_MEMORY(jsonConfiguration);
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "Could not read configuration from %s", g_osConfigConfigurationFile);
    }
        
    OsConfigLogInfo(ConfigurationGetLog(), "%s initialized", g_configurationModuleName);
}

void ConfigurationShutdown(void)
{
    OsConfigLogInfo(ConfigurationGetLog(), "%s shutting down", g_configurationModuleName);
    
    CloseLog(&g_log);
}

MMI_HANDLE ConfigurationMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_configurationModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(ConfigurationGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_configurationModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void ConfigurationMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(ConfigurationGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiClose() called outside of a valid session");
    }
}

int ConfigurationMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return status;
    }
    
    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_configurationModuleInfo);
    
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_configurationModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ConfigurationGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int ConfigurationMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    char buffer[10] = {0};

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    
    if ((MMI_OK == status) && (strcmp(componentName, g_configurationComponentName)))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    
    if (MMI_OK == status)
    {
        if (0 == strcmp(objectName, g_modelVersionObject))
        {
            snprintf(buffer, sizeof(buffer), "%u", g_modelVersion);
        }
        else if (0 == strcmp(objectName, g_refreshIntervalObject))
        {
            snprintf(buffer, sizeof(buffer), "%s", g_refreshIntervalObject ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_localManagementEnabledObject))
        {
            snprintf(buffer, sizeof(buffer), "%s", g_localManagementEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_fullLoggingEnabledObject))
        {
            snprintf(buffer, sizeof(buffer), "%s", g_fullLoggingEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_commandLoggingEnabledObject))
        {
            snprintf(buffer, sizeof(buffer), "%s", g_commandLoggingEnabled ? "true" : "false");
        }
        else if (0 == strcmp(objectName, g_iotHubProtocolObject))
        {
            snprintf(buffer, sizeof(buffer), "%u", g_iotHubProtocol);
        }
        else
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }
    }

    if (MMI_OK == status)
    {
        *payloadSizeBytes = strlen(buffer);

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported buffer will be truncated",
                componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
        if (*payload)
        {
            memcpy(*payload, buffer, *payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }    

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ConfigurationGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int ConfigurationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    OsConfigLogInfo(ConfigurationGetLog(), "No desired objects, MmiSet not implemented");
    
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);
    
    return EPERM;
}

void ConfigurationMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
