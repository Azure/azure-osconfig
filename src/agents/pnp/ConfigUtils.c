// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/ConfigUtils.h"

static bool IsLoggingEnabledInJsonConfig(const char* jsonString, const char* loggingSetting)
{
    bool result = false;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                result = (0 == (int)json_object_get_number(rootObject, loggingSetting)) ? false : true;
            }
            json_value_free(rootValue);
        }
    }

    return result;
}

bool IsCommandLoggingEnabledInJsonConfig(const char* jsonString)
{
    return IsLoggingEnabledInJsonConfig(jsonString, COMMAND_LOGGING);
}

bool IsFullLoggingEnabledInJsonConfig(const char* jsonString)
{
    return IsLoggingEnabledInJsonConfig(jsonString, FULL_LOGGING);
}

static int GetIntegerFromJsonConfig(const char* valueName, const char* jsonString, int defaultValue, int minValue, int maxValue)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    int valueToReturn = defaultValue;

    if (NULL == valueName)
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: no value name, using the specified default (%d)", defaultValue);
        return valueToReturn;
    }

    if (minValue >= maxValue)
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: bad min (%d) and/or max (%d) values for %s, using default (%d)",
            minValue, maxValue, valueName, defaultValue);
        return valueToReturn;
    }

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                valueToReturn = (int)json_object_get_number(rootObject, valueName);
                if (0 == valueToReturn)
                {
                    valueToReturn = defaultValue;
                    OsConfigLogInfo(GetLog(), "GetIntegerFromJsonConfig: %s value not found or 0, using default (%d)", valueName, defaultValue);
                }
                else if (valueToReturn < minValue)
                {
                    LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: %s value %d too small, using minimum (%d)", valueName, valueToReturn, minValue);
                    valueToReturn = minValue;
                }
                else if (valueToReturn > maxValue)
                {
                    LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: %s value %d too big, using maximum (%d)", valueName, valueToReturn, maxValue);
                    valueToReturn = maxValue;
                }
                else
                {
                    OsConfigLogInfo(GetLog(), "GetIntegerFromJsonConfig: %s: %d", valueName, valueToReturn);
                }
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: json_value_get_object(root) failed, using default (%d) for %s", defaultValue, valueName);
            }
            json_value_free(rootValue);
        }
        else
        {
            LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: json_parse_string failed, using default (%d) for %s", defaultValue, valueName);
        }
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: no configuration data, using default (%d) for %s", defaultValue, valueName);
    }

    return valueToReturn;
}

int GetReportingIntervalFromJsonConfig(const char* jsonString)
{
    return GetIntegerFromJsonConfig(REPORTING_INTERVAL_SECONDS, jsonString, DEFAULT_REPORTING_INTERVAL, MIN_REPORTING_INTERVAL, MAX_REPORTING_INTERVAL);
}

int GetModelVersionFromJsonConfig(const char* jsonString)
{
    return GetIntegerFromJsonConfig(MODEL_VERSION_NAME, jsonString, DEFAULT_DEVICE_MODEL_ID, MIN_DEVICE_MODEL_ID, MAX_DEVICE_MODEL_ID);
}

int GetLocalManagementFromJsonConfig(const char* jsonString)
{
    return GetIntegerFromJsonConfig(LOCAL_MANAGEMENT, jsonString, 0, 0, 1);
}

int GetProtocolFromJsonConfig(const char* jsonString)
{
    return GetIntegerFromJsonConfig(PROTOCOL, jsonString, PROTOCOL_AUTO, PROTOCOL_AUTO, PROTOCOL_MQTT_WS);
}

int LoadReportedFromJsonConfig(const char* jsonString, REPORTED_PROPERTY** reportedProperties)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Object* itemObject = NULL;
    JSON_Array* reportedArray = NULL;
    const char* componentName = NULL;
    const char* propertyName = NULL;
    size_t numReported = 0;
    size_t bufferSize = 0;
    size_t i = 0;
    int numReportedProperties = 0;

    if (NULL == reportedProperties)
    {
        LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: called with an invalid argument, no properties to report");
        return 0;
    }
    
    FREE_MEMORY(*reportedProperties);

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                reportedArray = json_object_get_array(rootObject, REPORTED_NAME);
                if (NULL != reportedArray)
                {
                    numReported = json_array_get_count(reportedArray);
                    OsConfigLogInfo(GetLog(), "LoadReportedFromJsonConfig: found %d %s entries in configuration", (int)numReported, REPORTED_NAME);

                    if (numReported > 0)
                    {
                        bufferSize = numReported * sizeof(REPORTED_PROPERTY);
                        *reportedProperties = (REPORTED_PROPERTY*)malloc(bufferSize);
                        if (NULL != *reportedProperties)
                        {
                            memset(*reportedProperties, 0, bufferSize);
                            numReportedProperties = (int)numReported;

                            for (i = 0; i < numReported; i++)
                            {
                                itemObject = json_array_get_object(reportedArray, i);
                                if (NULL != itemObject)
                                {
                                    componentName = json_object_get_string(itemObject, REPORTED_COMPONENT_NAME);
                                    propertyName = json_object_get_string(itemObject, REPORTED_SETTING_NAME);

                                    if ((NULL != componentName) && (NULL != propertyName))
                                    {
                                        strncpy((*reportedProperties)[i].componentName, componentName, ARRAY_SIZE((*reportedProperties)[i].componentName) - 1);
                                        strncpy((*reportedProperties)[i].propertyName, propertyName, ARRAY_SIZE((*reportedProperties)[i].propertyName) - 1);

                                        OsConfigLogInfo(GetLog(), "LoadReportedFromJsonConfig: found report property candidate at position %d of %d: %s.%s", (int)(i + 1),
                                            numReportedProperties, (*reportedProperties)[i].componentName, (*reportedProperties)[i].propertyName);
                                    }
                                    else
                                    {
                                        LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: %s or %s missing at position %d of %d, no property to report",
                                            REPORTED_COMPONENT_NAME, REPORTED_SETTING_NAME, (int)(i + 1), (int)numReported);
                                    }
                                }
                                else
                                {
                                    LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_array_get_object failed at position %d of %d, no reported property",
                                        (int)(i + 1), (int)numReported);
                                }
                            }
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: out of memory, cannot allocate %d bytes for %d reported properties",
                                (int)bufferSize, (int)numReported);
                        }
                    }
                }
                else
                {
                    LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: no valid %s array in configuration, no properties to report", REPORTED_NAME);
                }
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_value_get_object(root) failed, no properties to report");
            }

            json_value_free(rootValue);
        }
        else
        {
            LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_parse_string failed, no properties to report");
        }
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: no configuration data, no properties to report");
    }

    return numReportedProperties;
}

char* GetHttpProxyData()
{
    const char* proxyVariables[] = {
        "http_proxy",
        "https_proxy",
        "HTTP_PROXY",
        "HTTPS_PROXY"
    };
    int proxyVariablesSize = ARRAY_SIZE(proxyVariables);

    char* proxyData = NULL;
    char* environmentVariable = NULL;
    int i = 0;

    for (i = 0; i < proxyVariablesSize; i++)
    {
        environmentVariable = getenv(proxyVariables[i]);
        if (NULL != environmentVariable)
        {
            // The environment variable string must be treated as read-only, make a copy for our use:
            proxyData = DuplicateString(environmentVariable);
            if (NULL == proxyData)
            {
                LogErrorWithTelemetry(GetLog(), "Cannot make a copy of the %s variable: %d", proxyVariables[i], errno);
            }
            else
            {
                OsConfigLogInfo(GetLog(), "Proxy data from %s: %s", proxyVariables[i], proxyData);
            }
            break;
        }
    }

    return proxyData;
}