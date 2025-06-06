// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

// 1 second
#define MIN_REPORTING_INTERVAL 1

// 24 hours
#define MAX_REPORTING_INTERVAL 86400

#define REPORTED_NAME "Reported"
#define REPORTED_COMPONENT_NAME "ComponentName"
#define REPORTED_SETTING_NAME "ObjectName"
#define MODEL_VERSION_NAME "ModelVersion"
#define REPORTING_INTERVAL_SECONDS "ReportingIntervalSeconds"
#define IOT_HUB_MANAGEMENT "IotHubManagement"
#define LOCAL_MANAGEMENT "LocalManagement"
#define PROTOCOL "IotHubProtocol"
#define GIT_MANAGEMENT "GitManagement"
#define GIT_REPOSITORY_URL "GitRepositoryUrl"
#define GIT_BRANCH "GitBranch"
#define LOGGING_LEVEL "LoggingLevel"
#define MAX_LOG_SIZE "MaxLogSize"
#define MAX_LOG_SIZE_DEBUG_MULTIPLIER "MaxLogSizeDebugMultiplier"

#define MIN_DEVICE_MODEL_ID 7
#define MAX_DEVICE_MODEL_ID 999

// Emergency
#define MIN_LOGGING_LEVEL 0
// Informational
#define DEFAULT_LOGGING_LEVEL 6
// Debug
#define MAX_LOGGING_LEVEL 7

#define MIN_MAX_LOG_SIZE 1024
#define MIN_MAX_LOG_SIZE_DEBUG_MULTIPLIER 1
#define MAX_MAX_LOG_SIZE 1073741824
#define MAX_MAX_LOG_SIZE_DEBUG_MULTIPLIER 10
#define DEFAULT_MAX_LOG_SIZE 1048576
#define DEFAULT_MAX_LOG_SIZE_DEBUG_MULTIPLIER 5

static bool IsOptionEnabledInJsonConfig(const char* jsonString, const char* setting)
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
                result = (0 == (int)json_object_get_number(rootObject, setting)) ? false : true;
            }
            json_value_free(rootValue);
        }
    }

    return result;
}

bool IsIotHubManagementEnabledInJsonConfig(const char* jsonString)
{
    return IsOptionEnabledInJsonConfig(jsonString, IOT_HUB_MANAGEMENT);
}

static int GetIntegerFromJsonConfig(const char* valueName, const char* jsonString, int defaultValue, int minValue, int maxValue, OsConfigLogHandle log)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    int valueToReturn = defaultValue;

    if (NULL == valueName)
    {
        OsConfigLogDebug(log, "GetIntegerFromJsonConfig: no value name, using the specified default (%d)", defaultValue);
        return valueToReturn;
    }

    if (minValue >= maxValue)
    {
        OsConfigLogDebug(log, "GetIntegerFromJsonConfig: bad min (%d) and/or max (%d) values for '%s', using default (%d)", minValue, maxValue, valueName, defaultValue);
        return valueToReturn;
    }

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                valueToReturn = (int)json_object_get_number(rootObject, valueName);
                if ((0 == valueToReturn) && (0 != minValue) && (0 != maxValue))
                {
                    valueToReturn = defaultValue;
                    OsConfigLogDebug(log, "GetIntegerFromJsonConfig: '%s' value not found, using default (%d)", valueName, defaultValue);
                }
                else if (valueToReturn < minValue)
                {
                    OsConfigLogDebug(log, "GetIntegerFromJsonConfig: '%s' value %d too small, using minimum (%d)", valueName, valueToReturn, minValue);
                    valueToReturn = minValue;
                }
                else if (valueToReturn > maxValue)
                {
                    OsConfigLogDebug(log, "GetIntegerFromJsonConfig: '%s' value %d too big, using maximum (%d)", valueName, valueToReturn, maxValue);
                    valueToReturn = maxValue;
                }
                else
                {
                    OsConfigLogDebug(log, "GetIntegerFromJsonConfig: '%s': %d", valueName, valueToReturn);
                }
            }
            else
            {
                OsConfigLogDebug(log, "GetIntegerFromJsonConfig: json_value_get_object(root) failed, using default (%d) for '%s'", defaultValue, valueName);
            }
            json_value_free(rootValue);
        }
        else
        {
            OsConfigLogDebug(log, "GetIntegerFromJsonConfig: json_parse_string failed, using default (%d) for '%s'", defaultValue, valueName);
        }
    }
    else
    {
        OsConfigLogDebug(log, "GetIntegerFromJsonConfig: no configuration data, using default (%d) for '%s'", defaultValue, valueName);
    }

    return valueToReturn;
}

LoggingLevel GetLoggingLevelFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(LOGGING_LEVEL, jsonString, DEFAULT_LOGGING_LEVEL, MIN_LOGGING_LEVEL, MAX_LOGGING_LEVEL, log);
}

int GetMaxLogSizeFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(MAX_LOG_SIZE, jsonString, DEFAULT_MAX_LOG_SIZE, MIN_MAX_LOG_SIZE, MAX_MAX_LOG_SIZE, log);
}

int GetMaxLogSizeDebugMultiplierFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(MAX_LOG_SIZE_DEBUG_MULTIPLIER, jsonString, DEFAULT_MAX_LOG_SIZE_DEBUG_MULTIPLIER, MIN_MAX_LOG_SIZE_DEBUG_MULTIPLIER, MAX_MAX_LOG_SIZE_DEBUG_MULTIPLIER, log);
}

int GetReportingIntervalFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(REPORTING_INTERVAL_SECONDS, jsonString, DEFAULT_REPORTING_INTERVAL, MIN_REPORTING_INTERVAL, MAX_REPORTING_INTERVAL, log);
}

int GetModelVersionFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(MODEL_VERSION_NAME, jsonString, DEFAULT_DEVICE_MODEL_ID, MIN_DEVICE_MODEL_ID, MAX_DEVICE_MODEL_ID, log);
}

int GetLocalManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(LOCAL_MANAGEMENT, jsonString, 0, 0, 1, log);
}

int GetIotHubProtocolFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(PROTOCOL, jsonString, PROTOCOL_AUTO, PROTOCOL_AUTO, PROTOCOL_MQTT_WS, log);
}

int LoadReportedFromJsonConfig(const char* jsonString, ReportedProperty** reportedProperties, OsConfigLogHandle log)
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
        OsConfigLogError(log, "LoadReportedFromJsonConfig: called with an invalid argument, no properties to report");
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
                    OsConfigLogInfo(log, "LoadReportedFromJsonConfig: found %d %s entries in configuration", (int)numReported, REPORTED_NAME);

                    if (numReported > 0)
                    {
                        bufferSize = numReported * sizeof(ReportedProperty);
                        *reportedProperties = (ReportedProperty*)malloc(bufferSize);
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

                                        OsConfigLogInfo(log, "LoadReportedFromJsonConfig: found report property candidate at position %d of %d: %s.%s", (int)(i + 1),
                                            numReportedProperties, (*reportedProperties)[i].componentName, (*reportedProperties)[i].propertyName);
                                    }
                                    else
                                    {
                                        OsConfigLogError(log, "LoadReportedFromJsonConfig: %s or %s missing at position %d of %d, no property to report",
                                            REPORTED_COMPONENT_NAME, REPORTED_SETTING_NAME, (int)(i + 1), (int)numReported);
                                    }
                                }
                                else
                                {
                                    OsConfigLogError(log, "LoadReportedFromJsonConfig: json_array_get_object failed at position %d of %d, no reported property",
                                        (int)(i + 1), (int)numReported);
                                }
                            }
                        }
                        else
                        {
                            OsConfigLogError(log, "LoadReportedFromJsonConfig: out of memory, cannot allocate %d bytes for %d reported properties",
                                (int)bufferSize, (int)numReported);
                        }
                    }
                }
                else
                {
                    OsConfigLogError(log, "LoadReportedFromJsonConfig: no valid %s array in configuration, no properties to report", REPORTED_NAME);
                }
            }
            else
            {
                OsConfigLogError(log, "LoadReportedFromJsonConfig: json_value_get_object(root) failed, no properties to report");
            }

            json_value_free(rootValue);
        }
        else
        {
            OsConfigLogError(log, "LoadReportedFromJsonConfig: json_parse_string failed, no properties to report");
        }
    }
    else
    {
        OsConfigLogError(log, "LoadReportedFromJsonConfig: no configuration data, no properties to report");
    }

    return numReportedProperties;
}

static char* GetStringFromJsonConfig(const char* valueName, const char* jsonString, OsConfigLogHandle log)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    char* value = NULL;
    char* buffer = NULL;
    size_t valueLength = 0;

    if (NULL == valueName)
    {
        OsConfigLogDebug(log, "GetStringFromJsonConfig: no value name");
        return value;
    }

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                value = (char*)json_object_get_string(rootObject, valueName);
                if (NULL == value)
                {
                    OsConfigLogDebug(log, "GetStringFromJsonConfig: %s value not found or empty", valueName);
                }
                else
                {
                    valueLength = strlen(value);
                    buffer = (char*)malloc(valueLength + 1);
                    if (NULL != buffer)
                    {
                        memcpy(buffer, value, valueLength);
                        buffer[valueLength] = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "GetStringFromJsonConfig: failed to allocate %d bytes for %s", (int)(valueLength + 1), valueName);
                    }
                }
            }
            else
            {
                OsConfigLogDebug(log, "GetStringFromJsonConfig: json_value_get_object(root) failed for %s", valueName);
            }
            json_value_free(rootValue);
        }
        else
        {
            OsConfigLogDebug(log, "GetStringFromJsonConfig: json_parse_string failed for %s", valueName);
        }
    }
    else
    {
        OsConfigLogDebug(log, "GetStringFromJsonConfig: no configuration data for %s", valueName);
    }

    OsConfigLogDebug(log, "GetStringFromJsonConfig(%s): %s", valueName, buffer);

    return buffer;
}

int GetGitManagementFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetIntegerFromJsonConfig(GIT_MANAGEMENT, jsonString, 0, 0, 1, log);
}

char* GetGitRepositoryUrlFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetStringFromJsonConfig(GIT_REPOSITORY_URL, jsonString, log);
}

char* GetGitBranchFromJsonConfig(const char* jsonString, OsConfigLogHandle log)
{
    return GetStringFromJsonConfig(GIT_BRANCH, jsonString, log);
}

int SetLoggingLevelPersistently(LoggingLevel level, OsConfigLogHandle log)
{
    const char* configurationDirectory = "/etc/osconfig";
    const char* configurationFile = "/etc/osconfig/osconfig.json";
    const char* loggingLevelTemplate = "  \"LoggingLevel\": %d\n";
    const char* loggingLevelTemplateWithComma = "  \"LoggingLevel\": %d,\n";
    const char* configurationTemplate = "{\n  \"LoggingLevel\": %d\n}\n";

    char* jsonConfiguration = NULL;
    char* buffer = NULL;
    int result = 0;

    if (false == IsLoggingLevelSupported(level))
    {
        OsConfigLogError(log, "SetLoggingLevelPersistently: requested logging level %u is not supported", level);
        result = EINVAL;
    }
    else
    {
        if (FileExists(configurationFile))
        {
            if (NULL != (jsonConfiguration = LoadStringFromFile(configurationFile, false, log)))
            {
                if (level != GetLoggingLevelFromJsonConfig(jsonConfiguration, log))
                {
                    if (NULL == (buffer = FormatAllocateString(strstr(jsonConfiguration, ",") ? loggingLevelTemplateWithComma : loggingLevelTemplate, level)))
                    {
                        OsConfigLogError(log, "SetLoggingLevelPersistently: out of memory");
                        result = ENOMEM;
                    }
                    else if (0 != (result = ReplaceMarkedLinesInFile(configurationFile, "LoggingLevel", buffer, '#', true, log)))
                    {
                        OsConfigLogError(log, "SetLoggingLevelPersistently: failed to update the logging level to %u in the configuration file '%s'", level, configurationFile);
                    }
                }
            }
            else
            {
                OsConfigLogError(log, "SetLoggingLevelPersistently: cannot read from '%s' (%d, %s)", configurationFile, errno, strerror(errno));
                result = errno ? errno : ENOENT;
            }
        }
        else
        {
            if (NULL == (buffer = FormatAllocateString(configurationTemplate, level)))
            {
                OsConfigLogError(log, "SetLoggingLevelPersistently: out of memory");
                result = ENOMEM;
            }
            else if ((false == DirectoryExists(configurationDirectory)) && (0 != (result = mkdir(configurationDirectory, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))))
            {
                OsConfigLogError(log, "SetLoggingLevelPersistently: failed to create directory '%s'for the configuration file (%d, %s)", configurationDirectory, errno, strerror(errno));
                result = result ? result : (errno ? errno : ENOENT);
            }
            else if (false == SavePayloadToFile(configurationFile, buffer, strlen(buffer), log))
            {
                OsConfigLogError(log, "SetLoggingLevelPersistently: failed to save the new logging level %u to the configuration file '%s'  (%d, %s)", level, configurationFile, errno, strerror(errno));
                result = errno ? errno : ENOENT;
            }

            if (FileExists(configurationFile))
            {
                RestrictFileAccessToCurrentAccountOnly(configurationFile);
            }
        }

        SetLoggingLevel(level);
    }

    FREE_MEMORY(jsonConfiguration);
    FREE_MEMORY(buffer);

    return result;
}
