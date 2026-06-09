// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Internal.h"

#define LOGGING_LEVEL "LoggingLevel"
#define MAX_LOG_SIZE "MaxLogSize"
#define MAX_LOG_SIZE_DEBUG_MULTIPLIER "MaxLogSizeDebugMultiplier"

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
