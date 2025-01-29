// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>

#include "DeliveryOptimization.h"

static const char* g_deliveryOptimizationModuleInfo = "{\"Name\": \"DeliveryOptimization\","
    "\"Description\": \"Provides functionality to observe and configure Delivery Optimization (DO)\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"DeliveryOptimization\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_deliveryOptimizationModuleName = "DeliveryOptimization module";
static const char* g_deliveryOptimizationComponentName = "DeliveryOptimization";

static const char* g_reportedCacheHostObjectName = "cacheHost";
static const char* g_reportedCacheHostSourceObjectName = "cacheHostSource";
static const char* g_reportedCacheHostFallbackObjectName = "cacheHostFallback";
static const char* g_reportedPercentageDownloadThrottleObjectName = "percentageDownloadThrottle";
static const char* g_desiredDeliveryOptimizationPoliciesObjectName = "desiredDeliveryOptimizationPolicies";
static const char* g_desiredCacheHostSettingName = "cacheHost";
static const char* g_desiredCacheHostSourceSettingName = "cacheHostSource";
static const char* g_desiredCacheHostFallbackSettingName = "cacheHostFallback";
static const char* g_desiredPercentageDownloadThrottleSettingName = "percentageDownloadThrottle";

static const char* g_cacheHostConfigName = "DOCacheHost";
static const char* g_cacheHostSourceConfigName = "DOCacheHostSource";
static const char* g_cacheHostFallbackConfigName = "DOCacheHostFallback";
static const char* g_percentageDownloadThrottleConfigName = "DOPercentageDownloadThrottle";

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static const char* g_deliveryOptimizationConfigFile = NULL;
static const char* g_deliveryOptimizationLogFile = "/var/log/osconfig_deliveryoptimization.log";
static const char* g_deliveryOptimizationRolledLogFile = "/var/log/osconfig_deliveryoptimization.bak";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE DeliveryOptimizationGetLog()
{
    return g_log;
}

void DeliveryOptimizationInitialize(const char* configFile)
{
    g_deliveryOptimizationConfigFile = configFile;
    g_log = OpenLog(g_deliveryOptimizationLogFile, g_deliveryOptimizationRolledLogFile);

    OsConfigLogInfo(DeliveryOptimizationGetLog(), "%s initialized", g_deliveryOptimizationModuleName);
}

void DeliveryOptimizationShutdown(void)
{
    OsConfigLogInfo(DeliveryOptimizationGetLog(), "%s shutting down", g_deliveryOptimizationModuleName);

    g_deliveryOptimizationConfigFile = NULL;
    CloseLog(&g_log);
}

MMI_HANDLE DeliveryOptimizationMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_deliveryOptimizationModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(DeliveryOptimizationGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_deliveryOptimizationModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void DeliveryOptimizationMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(DeliveryOptimizationGetLog(), "MmiClose(%p)", clientSession);
    }
    else
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiClose() called outside of a valid session");
    }
}

int DeliveryOptimizationMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGetInfo(%s, %p, %p) called with invalid arguments", clientName, payload, payloadSizeBytes);
        return status;
    }

    *payloadSizeBytes = (int)strlen(g_deliveryOptimizationModuleInfo);
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memset(*payload, 0, *payloadSizeBytes);
        memcpy(*payload, g_deliveryOptimizationModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(DeliveryOptimizationGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int DeliveryOptimizationMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    const char* jsonPropertyName = NULL;
    int jsonPropertyType = JSONNull;
    JSON_Value *rootValue = NULL;
    JSON_Object *rootObject = NULL;
    JSON_Value *value = NULL;
    char *json = NULL;
    const char* emptyJsonPayload = NULL;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_deliveryOptimizationComponentName))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet called for an unsupported component name '%s'", componentName);
        status = EINVAL;
    }
    else
    {
        if (0 == strcmp(objectName, g_reportedCacheHostObjectName))
        {
            jsonPropertyName = g_cacheHostConfigName;
            jsonPropertyType = JSONString;
        }
        else if (0 == strcmp(objectName, g_reportedCacheHostSourceObjectName))
        {
            jsonPropertyName = g_cacheHostSourceConfigName;
            jsonPropertyType = JSONNumber;
        }
        else if (0 == strcmp(objectName, g_reportedCacheHostFallbackObjectName))
        {
            jsonPropertyName = g_cacheHostFallbackConfigName;
            jsonPropertyType = JSONNumber;
        }
        else if (0 == strcmp(objectName, g_reportedPercentageDownloadThrottleObjectName))
        {
            jsonPropertyName = g_percentageDownloadThrottleConfigName;
            jsonPropertyType = JSONNumber;
        }
        else
        {
            OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet called for an unsupported object name '%s'", objectName);
            status = EINVAL;
        }
    }

    if ((MMI_OK == status) && (NULL != jsonPropertyName) && (JSONNull != jsonPropertyType))
    {
        rootValue = json_parse_file(g_deliveryOptimizationConfigFile);

        if (json_value_get_type(rootValue) == JSONObject)
        {
            rootObject = json_value_get_object(rootValue);

            if (json_object_has_value_of_type(rootObject, jsonPropertyName, jsonPropertyType))
            {
                value = json_object_get_value(rootObject, jsonPropertyName);
                json = json_serialize_to_string(value);

                if (NULL != json)
                {
                    *payloadSizeBytes = strlen(json);
                    if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
                    {
                        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated", componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);
                        *payloadSizeBytes = g_maxPayloadSizeBytes;
                    }

                    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
                    if (NULL != *payload)
                    {
                        memset(*payload, 0, *payloadSizeBytes);
                        memcpy(*payload, json, *payloadSizeBytes);
                    }
                    else
                    {
                        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet failed to allocate %d bytes", *payloadSizeBytes + 1);
                        *payloadSizeBytes = 0;
                        status = ENOMEM;
                    }

                    json_free_serialized_string(json);
                }
                else
                {
                    if (IsFullLoggingEnabled())
                    {
                        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet failed to serialize JSON property '%s'", jsonPropertyName);
                    }
                    status = EINVAL;
                }
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet failed to find JSON property '%s'", jsonPropertyName);
                }
                status = EINVAL;
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet failed to parse JSON file '%s'", g_deliveryOptimizationConfigFile);
            }
            status = EINVAL;
        }

        if (NULL != rootValue)
        {
            json_value_free(rootValue);
        }

        // Reset status and payload if JSON file could not be parsed or property not found, as it may yet have to be configured.
        if (MMI_OK != status)
        {
            if (JSONNumber == jsonPropertyType)
            {
                emptyJsonPayload = "0";
            }
            else if (JSONString == jsonPropertyType)
            {
                emptyJsonPayload = "\"\"";
            }

            *payloadSizeBytes = strlen(emptyJsonPayload);
            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);

            if (NULL != payload)
            {
                memset(*payload, 0, *payloadSizeBytes);
                strncpy(*payload, emptyJsonPayload, *payloadSizeBytes);
                status = MMI_OK;
            }
            else
            {
                OsConfigLogError(DeliveryOptimizationGetLog(), "MmiGet failed to allocate %d bytes", *payloadSizeBytes + 1);
                *payloadSizeBytes = 0;
                status = ENOMEM;
            }
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(DeliveryOptimizationGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int DeliveryOptimizationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;
    char *buffer = NULL;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Value* newValue = NULL;
    JSON_Object* newObject = NULL;
    unsigned int count = 0;
    const char* name = NULL;
    JSON_Value *currentValue = NULL;
    const char* cacheHost = NULL;
    int cacheHostSource = 0;
    int cacheHostFallback = 0;
    int percentageDownloadThrottle = 0;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (payloadSizeBytes <= 0))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet(%s, %s, %p, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
    }
    else if (!IsValidSession(clientSession))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
    else if (0 != strcmp(componentName, g_deliveryOptimizationComponentName))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet called for an unsupported component name '%s'", componentName);
        status = EINVAL;
    }
    else if (0 != strcmp(objectName, g_desiredDeliveryOptimizationPoliciesObjectName))
    {
        OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet called for an unsupported object name '%s'", objectName);
        status = EINVAL;
    }
    else
    {
        // Copy payload to local buffer with null terminator for json_parse_string.
        buffer = malloc(payloadSizeBytes + 1);
        if (NULL != buffer)
        {
            memset(buffer, 0, payloadSizeBytes + 1);
            memcpy(buffer, payload, payloadSizeBytes);

            rootValue = json_parse_string(buffer);
            if (json_value_get_type(rootValue) == JSONObject)
            {
                rootObject = json_value_get_object(rootValue);

                // Create new JSON_Value with validated output.
                newValue = json_value_init_object();
                newObject = json_value_get_object(newValue);

                count = json_object_get_count(rootObject);
                for (unsigned int i = 0; i < count; i++)
                {
                    name = json_object_get_name(rootObject, i);
                    currentValue = json_object_get_value(rootObject, name);

                    if ((0 == strcmp(name, g_desiredCacheHostSettingName)) && (JSONString == json_value_get_type(currentValue)))
                    {
                        cacheHost = json_value_get_string(currentValue);
                        json_object_set_string(newObject, g_cacheHostConfigName, cacheHost);
                    }
                    else if ((0 == strcmp(name, g_desiredCacheHostSourceSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
                    {
                        cacheHostSource = (int)json_value_get_number(currentValue);
                        if ((cacheHostSource >= 0) && (cacheHostSource <= 3))
                        {
                            json_object_set_number(newObject, g_cacheHostSourceConfigName, cacheHostSource);
                        }
                        else
                        {
                            OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet called with invalid cacheHostSource %d", cacheHostSource);
                            status = EINVAL;
                        }
                    }
                    else if ((0 == strcmp(name, g_desiredCacheHostFallbackSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
                    {
                        cacheHostFallback = (int)json_value_get_number(currentValue);
                        json_object_set_number(newObject, g_cacheHostFallbackConfigName, cacheHostFallback);
                    }
                    else if ((0 == strcmp(name, g_desiredPercentageDownloadThrottleSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
                    {
                        percentageDownloadThrottle = (int)json_value_get_number(currentValue);
                        if ((percentageDownloadThrottle >= 0) && (percentageDownloadThrottle <= 100))
                        {
                            json_object_set_number(newObject, g_percentageDownloadThrottleConfigName, percentageDownloadThrottle);
                        }
                        else
                        {
                            OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet called with invalid percentageDownloadThrottle %d", percentageDownloadThrottle);
                            status = EINVAL;
                        }
                    }
                }

                if (JSONSuccess != json_serialize_to_file_pretty(newValue, g_deliveryOptimizationConfigFile))
                {
                    OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet failed to write JSON file '%s'", g_deliveryOptimizationConfigFile);
                    status = EIO;
                }

                json_value_free(newValue);
            }
            else
            {
                OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet failed to parse JSON '%.*s'", payloadSizeBytes, payload);
                status = EINVAL;
            }

            json_value_free(rootValue);
        }
        else
        {
            OsConfigLogError(DeliveryOptimizationGetLog(), "MmiSet failed to allocate %d bytes", payloadSizeBytes + 1);
            status = ENOMEM;
        }
    }

    OsConfigLogInfo(DeliveryOptimizationGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);

    FREE_MEMORY(buffer);

    return status;
}

void DeliveryOptimizationMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
