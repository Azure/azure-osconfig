// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
#include <stdatomic.h>
#endif

#include <version.h>
#include <parson.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#include "Configuration.h"

static const char* g_configurationModuleName = "OSConfig Configuration module";
static const char* g_configurationComponentName = "Configuration";

static const char* g_modelVersionObject = "modelVersion";
static const char* g_refreshIntervalObject = "refreshInterval";
static const char* g_localManagementEnabledObject = "localManagementEnabled";
static const char* g_fullLoggingEnabledObject = "fullLoggingEnabled";
static const char* g_commandLoggingEnabledObject = "commandLoggingEnabled";
static const char* g_iotHubManagementEnabledObject = "iotHubManagementEnabled";
static const char* g_iotHubProtocolObject = "iotHubProtocol";
static const char* g_gitManagementEnabledObject = "gitManagementEnabled";
static const char* g_gitBranchObject = "gitBranch";

static const char* g_desiredRefreshIntervalObject = "desiredRefreshInterval";
static const char* g_desiredLocalManagementEnabledObject = "desiredLocalManagementEnabled";
static const char* g_desiredFullLoggingEnabledObject = "desiredFullLoggingEnabled";
static const char* g_desiredCommandLoggingEnabledObject = "desiredCommandLoggingEnabled";
static const char* g_desiredIotHubManagementEnabledObject = "desiredIotHubManagementEnabled";
static const char* g_desiredIotHubProtocolObject = "desiredIotHubProtocol";
static const char* g_desiredGitManagementEnabledObject = "desiredGitManagementEnabled";
static const char* g_desiredGitBranchObject = "desiredGitBranch";

const char* g_auto = "auto";
const char* g_mqtt = "mqtt";
const char* g_mqttWebSocket = "mqttWebSocket";

static const char* g_osConfigConfigurationFile = "/etc/osconfig/osconfig.json";

#define MAX_CONFIGURATION_PATH 256
char g_configurationFile[MAX_CONFIGURATION_PATH] = {0};

static const char* g_configurationLogFile = "/var/log/osconfig_configuration.log";
static const char* g_configurationRolledLogFile = "/var/log/osconfig_configuration.bak";

static const char* g_configurationModuleInfo = "{\"Name\": \"Configuration\","
    "\"Description\": \"Provides functionality to manage OSConfig configuration on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 4,"
    "\"VersionInfo\": \"Dilithium\","
    "\"Components\": [\"Configuration\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static int g_modelVersion = DEFAULT_DEVICE_MODEL_ID;
static int g_refreshInterval = DEFAULT_REPORTING_INTERVAL;
static bool g_localManagementEnabled = false;
static bool g_fullLoggingEnabled = false;
static bool g_commandLoggingEnabled = false;
static bool g_iotHubManagementEnabled = false;
static int g_iotHubProtocol = 0;
static bool g_gitManagementEnabled = false;
static char* g_gitBranch = NULL;

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
static atomic_int g_referenceCount = 0;
#else
static int g_referenceCount = 0;
#endif
static unsigned int g_maxPayloadSizeBytes = 0;

static OSCONFIG_LOG_HANDLE ConfigurationGetLog(void)
{
    return g_log;
}

static char* LoadConfigurationFromFile(const char* fileName)
{
    char* jsonConfiguration = NULL;
    const char* fileToLoadFrom = fileName ? fileName : g_osConfigConfigurationFile;

    if (NULL != (jsonConfiguration = LoadStringFromFile(fileToLoadFrom, false, ConfigurationGetLog())))
    {
        g_modelVersion = GetModelVersionFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_refreshInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_localManagementEnabled = (GetLocalManagementFromJsonConfig(jsonConfiguration, ConfigurationGetLog())) ? true : false;
        g_fullLoggingEnabled = IsFullLoggingEnabledInJsonConfig(jsonConfiguration);
        g_commandLoggingEnabled = IsCommandLoggingEnabledInJsonConfig(jsonConfiguration);
        g_iotHubManagementEnabled = IsIotHubManagementEnabledInJsonConfig(jsonConfiguration);
        g_iotHubProtocol = GetIotHubProtocolFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
        g_gitManagementEnabled = GetGitManagementFromJsonConfig(jsonConfiguration, ConfigurationGetLog());

        FREE_MEMORY(g_gitBranch);
        g_gitBranch = GetGitBranchFromJsonConfig(jsonConfiguration, ConfigurationGetLog());
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "Could not read configuration from %s", fileToLoadFrom);
    }

    return jsonConfiguration;
}

void ConfigurationInitialize(const char* configurationFile)
{
    char* configuration = NULL;
    size_t nameLength = configurationFile ? (ARRAY_SIZE(g_configurationFile) - 1) : strlen(g_osConfigConfigurationFile);

    g_log = OpenLog(g_configurationLogFile, g_configurationRolledLogFile);

    memset(g_configurationFile, 0, sizeof(g_configurationFile));
    strncpy(g_configurationFile, configurationFile ? configurationFile : g_osConfigConfigurationFile, nameLength);

    configuration = LoadConfigurationFromFile(g_configurationFile);
    FREE_MEMORY(configuration);

    OsConfigLogInfo(ConfigurationGetLog(), "%s initialized for target configuration file: %s", g_configurationModuleName, g_configurationFile);
}

void ConfigurationShutdown(void)
{
    OsConfigLogInfo(ConfigurationGetLog(), "%s shutting down", g_configurationModuleName);

    FREE_MEMORY(g_gitBranch);

    CloseLog(&g_log);
}

static int UpdateConfigurationFile(void)
{
    const char* commandLoggingEnabledName = "CommandLogging";
    const char* fullLoggingEnabledName = "FullLogging";
    const char* localManagementEnabledName = "LocalManagement";
    const char* modelVersionName = "ModelVersion";
    const char* iotHubtManagementEnabledName = "IotHubManagement";
    const char* iotHubProtocolName = "IotHubProtocol";
    const char* refreshIntervalName = "ReportingIntervalSeconds";
    const char* gitManagementEnabledName = "GitManagement";
    const char* gitBranchName = "GitBranch";

    int status = MMI_OK;

    JSON_Value* jsonValue = NULL;
    JSON_Object* jsonObject = NULL;

    int modelVersion = g_modelVersion;
    int refreshInterval = g_refreshInterval;
    bool localManagementEnabled = g_localManagementEnabled;
    bool fullLoggingEnabled = g_fullLoggingEnabled;
    bool commandLoggingEnabled = g_commandLoggingEnabled;
    bool iotHubManagementEnabled = g_iotHubManagementEnabled;
    int iotHubProtocol = g_iotHubProtocol;
    bool gitManagementEnabled = g_gitManagementEnabled;
    char* gitBranch = DuplicateString(g_gitBranch);

    char* existingConfiguration = LoadConfigurationFromFile(g_configurationFile);
    char* newConfiguration = NULL;

    if (!existingConfiguration)
    {
        OsConfigLogError(ConfigurationGetLog(), "No configuration file, cannot update configuration");
        return ENOENT;
    }

    if ((modelVersion != g_modelVersion) || (refreshInterval != g_refreshInterval) || (localManagementEnabled != g_localManagementEnabled) ||
        (fullLoggingEnabled != g_fullLoggingEnabled) || (commandLoggingEnabled != g_commandLoggingEnabled) ||
        (iotHubManagementEnabled != g_iotHubManagementEnabled) || (iotHubProtocol != g_iotHubProtocol) ||
        (gitManagementEnabled != g_gitManagementEnabled) || strcmp(gitBranch, g_gitBranch))
    {
        if (NULL == (jsonValue = json_parse_string(existingConfiguration)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_parse_string(%s) failed, UpdateConfigurationFile failed", existingConfiguration);
            status = EINVAL;
        }
        else if (NULL == (jsonObject = json_value_get_object(jsonValue)))
        {
            OsConfigLogError(ConfigurationGetLog(), "json_value_get_object(%s) failed, UpdateConfigurationFile failed", existingConfiguration);
            status = EINVAL;
        }

        if (MMI_OK == status)
        {
            if (JSONSuccess == json_object_set_number(jsonObject, modelVersionName, (double)modelVersion))
            {
                g_modelVersion = modelVersion;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_modelVersionObject, modelVersion);
            }

            if (JSONSuccess == json_object_set_number(jsonObject, refreshIntervalName, (double)refreshInterval))
            {
                g_refreshInterval = refreshInterval;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_refreshIntervalObject, refreshInterval);
            }

            if (JSONSuccess == json_object_set_number(jsonObject, localManagementEnabledName, (double)(localManagementEnabled ? 1 : 0)))
            {
                g_localManagementEnabled = localManagementEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_localManagementEnabledObject, localManagementEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, fullLoggingEnabledName, (double)(fullLoggingEnabled ? 1: 0)))
            {
                g_fullLoggingEnabled = fullLoggingEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_fullLoggingEnabledObject, fullLoggingEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, commandLoggingEnabledName, (double)(commandLoggingEnabled ? 1 : 0)))
            {
                g_commandLoggingEnabled = commandLoggingEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_commandLoggingEnabledObject, commandLoggingEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, iotHubtManagementEnabledName, (double)(iotHubManagementEnabled ? 1 : 0)))
            {
                g_iotHubManagementEnabled = iotHubManagementEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_iotHubManagementEnabledObject, iotHubManagementEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_number(jsonObject, iotHubProtocolName, (double)iotHubProtocol))
            {
                g_iotHubProtocol = iotHubProtocol;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_number(%s, %d) failed", g_iotHubProtocolObject, iotHubProtocol);
            }

            if (JSONSuccess == json_object_set_number(jsonObject, gitManagementEnabledName, (double)(gitManagementEnabled ? 1 : 0)))
            {
                g_gitManagementEnabled = gitManagementEnabled;
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_boolean(%s, %s) failed", g_gitManagementEnabledObject, gitManagementEnabled ? "true" : "false");
            }

            if (JSONSuccess == json_object_set_string(jsonObject, gitBranchName, gitBranch))
            {
                FREE_MEMORY(g_gitBranch);
                g_gitBranch = DuplicateString(gitBranch);
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_object_set_string(%s, %s) failed", g_gitBranchObject, gitBranch);
            }
        }

        if (MMI_OK == status)
        {
            if (NULL != (newConfiguration = json_serialize_to_string_pretty(jsonValue)))
            {
                if (false == SavePayloadToFile(g_configurationFile, newConfiguration, strlen(newConfiguration), ConfigurationGetLog()))
                {
                    OsConfigLogError(ConfigurationGetLog(), "Failed saving configuration to %s", g_osConfigConfigurationFile);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "json_serialize_to_string_pretty failed");
                status = EIO;
            }
        }
    }

    if (MMI_OK == status)
    {
        OsConfigLogInfo(ConfigurationGetLog(), "New configuration successfully applied: %s", IsFullLoggingEnabled() ? newConfiguration : "-");
    }
    else
    {
        OsConfigLogError(ConfigurationGetLog(), "Failed to apply new configuration: %s", IsFullLoggingEnabled() ? newConfiguration : "-");
    }

    if (jsonValue)
    {
        json_value_free(jsonValue);
    }

    if (newConfiguration)
    {
        json_free_serialized_string(newConfiguration);
    }

    FREE_MEMORY(gitBranch);
    FREE_MEMORY(existingConfiguration);

    return status;
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
        OsConfigLogError(ConfigurationGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
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
    JSON_Value* jsonValue = NULL;
    char* serializedValue = NULL;
    int status = MMI_OK;

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
    else if (0 != strcmp(componentName, g_configurationComponentName))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
    else
    {
        if (0 == strcmp(objectName, g_modelVersionObject))
        {
            jsonValue = json_value_init_number((double)g_modelVersion);
        }
        else if (0 == strcmp(objectName, g_refreshIntervalObject))
        {
            jsonValue = json_value_init_number((double)g_refreshInterval);
        }
        else if (0 == strcmp(objectName, g_localManagementEnabledObject))
        {
            jsonValue = json_value_init_boolean(g_localManagementEnabled ? 1 : 0);
        }
        else if (0 == strcmp(objectName, g_fullLoggingEnabledObject))
        {
            jsonValue = json_value_init_boolean(g_fullLoggingEnabled ? 1 : 0);
        }
        else if (0 == strcmp(objectName, g_commandLoggingEnabledObject))
        {
            jsonValue = json_value_init_boolean(g_commandLoggingEnabled ? 1 : 0);
        }
        else if (0 == strcmp(objectName, g_iotHubManagementEnabledObject))
        {
            jsonValue = json_value_init_boolean(g_iotHubManagementEnabled ? 1 : 0);
        }
        else if (0 == strcmp(objectName, g_iotHubProtocolObject))
        {
            switch (g_iotHubProtocol)
            {
                case 1:
                    jsonValue = json_value_init_string(g_mqtt);
                    break;

                case 2:
                    jsonValue = json_value_init_string(g_mqttWebSocket);
                    break;

                case 0:
                default:
                    jsonValue = json_value_init_string(g_auto);

            }
        }
        else if (0 == strcmp(objectName, g_gitManagementEnabledObject))
        {
            jsonValue = json_value_init_boolean(g_gitManagementEnabled ? 1 : 0);
        }
        else if (0 == strcmp(objectName, g_gitBranchObject))
        {
            jsonValue = json_value_init_string(g_gitBranch);
        }
        else
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet called for an unsupported object (%s)", objectName);
            status = EINVAL;
        }

        if ((0 == status) && (NULL == jsonValue))
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s) failed due to json_value_init_* failure", componentName, objectName);
            status = ENOENT;
        }
    }

    if (MMI_OK == status)
    {
        if (NULL == (serializedValue = json_serialize_to_string(jsonValue)))
        {
            OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s): json_serialize_to_string failed", componentName, objectName);
            status = ENOENT;
        }
        else
        {
            *payloadSizeBytes = (int)strlen(serializedValue);

            if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
            {
                OsConfigLogError(ConfigurationGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported buffer will be truncated",
                    componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);

                *payloadSizeBytes = g_maxPayloadSizeBytes;
            }

            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
            if (*payload)
            {
                memcpy(*payload, serializedValue, *payloadSizeBytes);
            }
            else
            {
                OsConfigLogError(ConfigurationGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
                *payloadSizeBytes = 0;
                status = ENOMEM;
            }
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ConfigurationGetLog(), "MmiGet(%p, %s, %s, '%.*s', %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    if (NULL != serializedValue)
    {
        json_free_serialized_string(serializedValue);
    }

    if (NULL != jsonValue)
    {
        json_value_free(jsonValue);
    }

    return status;
}

int ConfigurationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    JSON_Value* jsonValue = NULL;
    const char* jsonString = NULL;
    char* payloadString = NULL;

    int status = MMI_OK;

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
    else if (0 != strcmp(componentName, g_configurationComponentName))
    {
        OsConfigLogError(ConfigurationGetLog(), "MmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if (MMI_OK == status)
    {
        if (NULL != (payloadString = malloc(payloadSizeBytes + 1)))
        {
            memset(payloadString, 0, payloadSizeBytes + 1);
            memcpy(payloadString, payload, payloadSizeBytes);

            if (NULL != (jsonValue = json_parse_string(payloadString)))
            {
                if (0 == strcmp(objectName, g_desiredRefreshIntervalObject))
                {
                    g_refreshInterval = (unsigned int)json_value_get_number(jsonValue);
                }
                else if (0 == strcmp(objectName, g_desiredLocalManagementEnabledObject))
                {
                    g_localManagementEnabled = (0 == json_value_get_boolean(jsonValue)) ? false : true;
                }
                else if (0 == strcmp(objectName, g_desiredFullLoggingEnabledObject))
                {
                    g_fullLoggingEnabled = (0 == json_value_get_boolean(jsonValue)) ? false : true;
                }
                else if (0 == strcmp(objectName, g_desiredCommandLoggingEnabledObject))
                {
                    g_commandLoggingEnabled = (0 == json_value_get_boolean(jsonValue)) ? false : true;
                }
                else if (0 == strcmp(objectName, g_desiredIotHubManagementEnabledObject))
                {
                    g_iotHubManagementEnabled = (0 == json_value_get_boolean(jsonValue)) ? false : true;
                }
                else if (0 == strcmp(objectName, g_desiredIotHubProtocolObject))
                {
                    if (NULL != (jsonString = (char*)json_value_get_string(jsonValue)))
                    {
                        if (0 == strcmp(g_auto, jsonString))
                        {
                            g_iotHubProtocol = 0;
                        }
                        else if (0 == strcmp(g_mqtt, jsonString))
                        {
                            g_iotHubProtocol = 1;
                        }
                        else if (0 == strcmp(g_mqttWebSocket, jsonString))
                        {
                            g_iotHubProtocol = 2;
                        }
                        else
                        {
                            OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s): unsupported value: '%s'", componentName, objectName, payloadString);
                            status = EINVAL;
                        }
                    }
                    else
                    {
                        OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s): json_value_get_string('%s') failed", componentName, objectName, payloadString);
                        status = EINVAL;
                    }
                }
                else if (0 == strcmp(objectName, g_desiredGitManagementEnabledObject))
                {
                    g_gitManagementEnabled = (0 == json_value_get_boolean(jsonValue)) ? false : true;
                }
                else if (0 == strcmp(objectName, g_desiredGitBranchObject))
                {
                    if (NULL != (jsonString = json_value_get_string(jsonValue)))
                    {
                        if (jsonString)
                        {
                            FREE_MEMORY(g_gitBranch);
                            g_gitBranch = DuplicateString(jsonString);
                        }
                        else
                        {
                            OsConfigLogError(ConfigurationGetLog(), "Bad string value for %s (json_value_get_string failed)", g_desiredGitBranchObject);
                            status = EINVAL;
                        }
                    }
                    else
                    {
                        OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s): json_value_get_string('%s') failed", componentName, objectName, payloadString);
                        status = EINVAL;
                    }
                }
                else
                {
                    OsConfigLogError(ConfigurationGetLog(), "MmiSet called for an unsupported object name: %s", objectName);
                    status = EINVAL;
                }
            }
            else
            {
                status = EINVAL;
                OsConfigLogError(ConfigurationGetLog(), "MmiSet(%s, %s): json_parse_string(%s) failed", componentName, objectName, payloadString);
            }
        }
        else
        {
            OsConfigLogError(ConfigurationGetLog(), "Failed to allocate %d bytes of memory, MmiSet failed", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }

    if (MMI_OK == status)
    {
        status = UpdateConfigurationFile();
    }

    if (NULL != jsonValue)
    {
        json_value_free(jsonValue);
    }

    FREE_MEMORY(payloadString);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(ConfigurationGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
    }
    else
    {
        OsConfigLogInfo(ConfigurationGetLog(), "MmiSet(%p, %s, %s) returning %d", clientSession, componentName, objectName, status);
    }

    return status;
}

void ConfigurationMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
