// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "Configuration.h"

static const char* g_configurationModuleName = "OSConfig Configuration module";
static const char* g_configurationComponentName = "Configuration";
static const char* g_desiredConfigurationObject = "desiredConfiguration";
static const char* g_modelVersionObject = "modelVersion";
static const char* g_refreshIntervalObject = "refreshInterval";
static const char* g_localManagementEnabledObject = "localManagementEnabled";
static const char* g_fullLoggingEnabledObject = "fullLoggingEnabled";
static const char* g_commandLoggingEnabledObject = "commandLoggingEnabled";
static const char* g_iotHubProtocolObject = "iotHubProtocol";

static const char* g_commandLoggingEnabled = "CommandLogging";
static const char* g_fullLoggingEnabled = "FullLogging";
static const char* g_localManagementEnabled = "LocalManagement";
static const char* g_modelVersion = "ModelVersion";
static const char* g_iotHubProtocol = "IotHubProtocol";
static const char* g_refreshInterval = "ReportingIntervalSeconds";

static const char* g_osConfigDaemon = "osconfig";
static const char* g_osConfigConfigurationFile = "/etc/osconfig/osconfig.json";

static const char* g_configurationLogFile = "/var/log/osconfig_configuration.log";
static const char* g_configurationRolledLogFile = "/var/log/osconfig_configuration.bak";

static const char* g_configurationModuleInfo = "{\"Name\": \"Configuration\","
    "\"Description\": \"Provides functionality to manage OSConfig configuration on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Nickel\","
    "\"Components\": [\"Configuration\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static OSCONFIG_LOG_HANDLE g_log = NULL;

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

static char* LoadConfigurationFromFile(void)
{
    char* jsonConfiguration = NULL;
    
    if (NULL != (jsonConfiguration = LoadStringFromFile(g_osConfigConfigurationFile, false, ConfigurationGetLog())))
    {
        g_modelVersion = GetModelVersionFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_refreshInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_localManagementEnabled = GetLocalManagementFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_fullLoggingEnabled = IsCommandLoggingEnabledInJsonConfig(jsonConfiguration);
        g_commandLoggingEnabled = IsFullLoggingEnabledInJsonConfig(jsonConfiguration);
        g_iotHubProtocol = GetIotHubProtocolFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "Could not read configuration from %s", g_osConfigConfigurationFile);
    }

    return jsonConfiguration;
}


void ConfigurationInitialize(void)
{
    char* configuration = NULL;

    g_log = OpenLog(g_configurationLogFile, g_configurationRolledLogFile);

    configuration = LoadConfigurationFromFile();
    FREE_MEMORY(configuration);
        
    OsConfigLogInfo(ConfigurationGetLog(), "%s initialized", g_configurationModuleName);
}

static int UpdateConfiguration(void)
{
    int status = MMI_OK;

    JSON_Value* jsonValue = NULL;
    JSON_Object* jsonObject = NULL;

    int modelVersion = g_modelVersion;
    int refreshInterval = g_refreshInterval;
    bool localManagementEnabled = g_localManagementEnabled;
    bool fullLoggingEnabled = g_fullLoggingEnabled;
    bool commandLoggingEnabled = g_commandLoggingEnabled;
    int iotHubProtocol = g_iotHubProtocol;

    char* existingConfiguration = LoadConfigurationFromFile();
    char* newConfiguration = NULL;

    if ((modelVersion != g_modelVersion) || (refreshInterval != g_refreshInterval) || (localManagementEnabled != g_localManagementEnabled) || 
        (fullLoggingEnabled != g_fullLoggingEnabled) || (commandLoggingEnabled != g_commandLoggingEnabled) || (iotHubProtocol != g_iotHubProtocol))
    {
        if (NULL == (jsonValue = json_parse_string(existingConfiguration)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_parse_string(%s) failed, UpdateConfiguration failed", existingConfiguration);
            status = EINVAL;
        }
        else if (NULL == (jsonObject = json_value_get_object(jsonValue)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_value_get_object(%s) failed, UpdateConfiguration failed", existingConfiguration);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if (JSONSuccess == json_object_set_number(jsonObject, g_modelVersion, (double)modelVersion))
            {
                g_modelVersion = modelVersion;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_modelVersionObject, modelVersion);
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, g_refreshInterval, (double)refreshInterval))
            {
                g_refreshInterval = refreshInterval;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_refreshIntervalObject, refreshInterval);
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, g_localManagementEnabled, (double)(localManagementEnabled ? 1 : 0)))
            {
                g_localManagementEnabled = localManagementEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_localManagementEnabledObject, localManagementEnabled ? "true" : "false");
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, g_fullLoggingEnabled, (double)(fullLoggingEnabled ? 1: 0)))
            {
                g_fullLoggingEnabled = fullLoggingEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_fullLoggingEnabledObject, fullLoggingEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, g_commandLoggingEnabled, (double)(commandLoggingEnabled ? 1 : 0))
            {
                g_commandLoggingEnabled = commandLoggingEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_commandLoggingEnabledObject, commandLoggingEnabled ? "true" : "false");
            }
            
            if (JSONSuccess == json_object_set_number(jsonObject, g_iotHubProtocol, (double)iotHubProtocol))
            {
                g_iotHubProtocol = iotHubProtocol;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_iotHubProtocolObject, iotHubProtocol);
            }
        }

        if (MMI_OK == status)
        {
            newConfiguration = json_serialize_to_string_pretty(jsonValue);
            OsConfigLogInfo(ConfigurationGetLog(), "New configuration: %s", newConfiguration);  //delete this
            
            if (SavePayloadToFile(g_osConfigConfigurationFile, newConfiguration, strlen(newConfiguration), ConfigurationGetLog()))
            {
                if (false == RestartDaemon(g_osConfigDaemon, ConfigurationGetLog()))
                {
                    OsConfigLogError(ConfigurationGetLog(), "Failed restarting %s to apply configuration", g_osConfigDaemon);
                    status = ESRCH;
                }
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "Failed saving configuration to %s", g_osConfigConfigurationFile);
                status = ENOENT;
            }
        }
    }
        
    if (jsonValue)
    {
        json_value_free(jsonValue);
    }

    FREE_MEMORY(existingConfiguration);
    FREE_MEMORY(newConfiguration);

    return status;
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
    char* configuration = NULL;

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
        configuration = LoadConfigurationFromFile();

        if (0 == strcmp(objectName, g_modelVersionObject))
        {
            snprintf(buffer, sizeof(buffer), "%u", g_modelVersion);
        }
        else if (0 == strcmp(objectName, g_refreshIntervalObject))
        {
            snprintf(buffer, sizeof(buffer), "%u", g_refreshInterval);
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

    FREE_MEMORY(configuration);

    return status;
}

int ConfigurationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;

    char* payloadString = NULL;

    int modelVersion = g_modelVersion;
    int refreshInterval = g_refreshInterval;
    int localManagementEnabled = g_localManagementEnabled ? 1 : 0;
    int fullLoggingEnabled = g_fullLoggingEnabled ? 1 : 0;
    int commandLoggingEnabled = g_commandLoggingEnabled ? 1 : 0;
    int iotHubProtocol = g_iotHubProtocol;

    JSON_Value* jsonValue = NULL;
    JSON_Object* jsonObject = NULL;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s, %s, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (strcmp(componentName, g_configurationComponentName)))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (strcmp(objectName, g_desiredConfigurationObject)))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiSet called for an unsupported object name (%s)", objectName);
        status = EINVAL;
    }

    if (MMI_OK == status)
    {
        if (NULL != (payloadString = malloc(payloadSizeBytes + 1)))
        {
            memset(payloadString, 0, payloadSizeBytes + 1);
            memcpy(payloadString, payload, payloadSizeBytes);
        }
        else
        {
            OsConfigLogError(ConfigurationGetLog(), "Failed to allocate %d bytes of memory, MmiSet failed", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }

    if (MMI_OK == status)
    {
        if (NULL == (jsonValue = json_parse_string(payloadString)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_parse_string(%s) failed, MmiSet failed", payloadString);
            status = EINVAL;
        }
        else if (JSONObject != json_value_get_type(jsonValue))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_value_get_type(%s) did not return JSONObject, MmiSet failed", payloadString);
            status = EINVAL;
        }
        else if (NULL == (jsonObject = json_value_get_object(jsonValue)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_value_get_object(%s) failed, MmiSet failed", payloadString);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if (0 != (modelVersion = (int)json_object_get_number(jsonObject, g_modelVersionObject)))
            {
                g_modelVersion = modelVersion;
            }

            if (0 != (refreshInterval = (int)json_object_get_number(jsonObject, g_refreshIntervalObject)))
            {
                g_refreshInterval = refreshInterval;
            }

            if (-1 != (localManagementEnabled = json_object_get_boolean(jsonObject, g_localManagementEnabledObject)))
            {
                g_localManagementEnabled = localManagementEnabled ? true : false;
            }

            if (-1 != (fullLoggingEnabled = json_object_get_boolean(jsonObject, g_fullLoggingEnabledObject)))
            {
                g_fullLoggingEnabled = fullLoggingEnabled ? true : false;
            }

            if (-1 != (commandLoggingEnabled = json_object_get_boolean(jsonObject, g_commandLoggingEnabledObject)))
            {
                g_commandLoggingEnabled = commandLoggingEnabled ? true : false;
            }

            iotHubProtocol = (int)json_object_get_number(jsonObject, g_iotHubProtocolObject);
            if ((0 == iotHubProtocol) || (1 == iotHubProtocol) || (2 == iotHubProtocol))
            {
                g_iotHubProtocol = iotHubProtocol;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "Unsupported %s value (%d), ignored", g_iotHubProtocolObject, iotHubProtocol);
            }

            status = UpdateConfiguration();
        }
    }

    if (jsonValue)
    {
        json_value_free(jsonValue);
    }
    
    FREE_MEMORY(payloadString);

    OsConfigLogInfo(ConfigurationGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    return status;
}

void ConfigurationMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
