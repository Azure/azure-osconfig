// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.hpp"
#include "ParameterSets.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

using namespace MAT;

LOGMANAGER_INSTANCE

namespace Telemetry
{

OsConfigLogHandle TelemetryManager::m_log = NULL;
MAT::ILogger* TelemetryManager::m_logger = nullptr;
bool TelemetryManager::m_initialized = false;

TelemetryManager::~TelemetryManager() noexcept
{
    Shutdown();
}

void TelemetryManager::SetupConfiguration(bool enableDebug, int teardownTime)
{
    auto& logConfig = LogManager::GetLogConfiguration();

    logConfig[CFG_BOOL_ENABLE_TRACE] = enableDebug;
    logConfig[CFG_INT_TRACE_LEVEL_MIN] = 0;
    logConfig[CFG_INT_MAX_TEARDOWN_TIME] = teardownTime;

    m_logger = LogManager::Initialize(API_KEY);
}

bool TelemetryManager::Initialize(bool enableDebug, int teardownTime)
{
    if (m_initialized)
    {
        return true;
    }

        OsConfigLogInfo(m_log, "Initializing telemetry...");

    try
    {
        if (NULL == m_log)
        {
            m_log = OpenLog("/var/log/osconfig_telemetry.log", NULL);
        }

        SetupConfiguration(enableDebug, teardownTime);

        if (!m_logger)
        {
            return false;
        }

        LogManager::SetTransmitProfile(TransmitProfile::TransmitProfile_RealTime);
        m_initialized = true;
        OsConfigLogInfo(m_log, "Telemetry initialized successfully.");
        return true;
    }
    catch (const std::exception& e)
    {
        if (m_log)
        {
            OsConfigLogError(m_log, "Failed to initialize telemetry: %s", e.what());
        }
        return false;
    }
}

void TelemetryManager::EventWrite(Microsoft::Applications::Events::EventProperties event)
{
    if ((!m_initialized) || (!m_logger))
    {
        return;
    }

    m_logger->LogEvent(event);

}

void TelemetryManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    OsConfigLogInfo(m_log, "Shutting down telemetry...");

    try
    {
        LogManager::UploadNow();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Without sleep, the upload may not complete
        LogManager::FlushAndTeardown();
        m_initialized = false;
        OsConfigLogInfo(m_log, "Telemetry shutdown complete.");
        CloseLog(&m_log);
    }
    catch (const std::exception& e)
    {
        if (m_log)
        {
            OsConfigLogError(m_log, "Exception during shutdown: %s", e.what());
        }
    }
}

bool TelemetryManager::ProcessJsonFile(const std::string& filePath)
{
    if (!m_initialized)
    {
        if (!Initialize())
        {
            OsConfigLogError(m_log, "Failed to initialize TelemetryManager");
            return false;
        }
    }

    OsConfigLogInfo(m_log, "Processing JSON file: %s", filePath.c_str());

    FILE* file = fopen(filePath.c_str(), "r");
    if (!file)
    {
        OsConfigLogError(m_log, "Failed to open file: %s", filePath.c_str());
        return false;
    }

    char* line = nullptr;
    size_t lineSize = 0;
    ssize_t lineLength;
    bool success = true;

    try
    {
        // Read file line by line
        while ((lineLength = getline(&line, &lineSize, file)) != -1)
        {
            // Remove trailing newline if present
            if (lineLength > 0 && line[lineLength - 1] == '\n')
            {
                line[lineLength - 1] = '\0';
            }

            // Skip empty lines
            if (strlen(line) == 0)
            {
                continue;
            }

            // Process the JSON line
            ProcessJsonLine(std::string(line));
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(m_log, "Error processing JSON file '%s': %s", filePath.c_str(), e.what());
        success = false;
    }

    OsConfigLogInfo(m_log, "Completed processing JSON file: %s", filePath.c_str());

    // Cleanup
    if (line)
    {
        free(line);
    }
    fclose(file);

    return success;
}

bool TelemetryManager::ValidateEventParameters(const std::string& eventName, const std::set<std::string>& jsonKeys)
{
    auto it = EVENT_PARAMETER_SETS.find(eventName);
    if (it == EVENT_PARAMETER_SETS.end())
    {
        OsConfigLogError(m_log, "Unknown event type: %s", eventName.c_str());
        return false;
    }

    const auto& requiredParams = it->second.first;
    const auto& optionalParams = it->second.second;

    // Check that all required parameters are present
    for (const auto& requiredParam : requiredParams)
    {
        if (jsonKeys.find(requiredParam) == jsonKeys.end())
        {
            OsConfigLogError(m_log, "Missing required parameter '%s' for event '%s'", requiredParam.c_str(), eventName.c_str());
            return false;
        }
    }

    // Check that no unexpected parameters are present
    for (const auto& jsonKey : jsonKeys)
    {
        if (jsonKey == "EventName") continue; // Skip the event name field

        if (requiredParams.find(jsonKey) == requiredParams.end() &&
            optionalParams.find(jsonKey) == optionalParams.end())
        {
            OsConfigLogError(m_log, "Unexpected parameter '%s' for event '%s'", jsonKey.c_str(), eventName.c_str());
            return false;
        }
    }

    return true;
}

void TelemetryManager::ProcessJsonLine(const std::string& jsonLine)
{
    if (!m_logger)
    {
        return;
    }

    JSON_Value* jsonValue = nullptr;
    JSON_Object* jsonObject = nullptr;

    try
    {
        // Parse the JSON string
        jsonValue = json_parse_string(jsonLine.c_str());
        if (!jsonValue)
        {
            OsConfigLogError(m_log, "Failed to parse JSON line: %s", jsonLine.c_str());
            return;
        }

        jsonObject = json_value_get_object(jsonValue);
        if (!jsonObject)
        {
            OsConfigLogError(m_log, "JSON line is not an object: %s", jsonLine.c_str());
            json_value_free(jsonValue);
            return;
        }

        // Extract event name - required field
        const char* eventName = json_object_get_string(jsonObject, "EventName");
        if (!eventName)
        {
            OsConfigLogError(m_log, "JSON object missing 'EventName' field: %s", jsonLine.c_str());
            json_value_free(jsonValue);
            return;
        }

        // Collect all JSON keys for validation
        std::set<std::string> jsonKeys;
        size_t count = json_object_get_count(jsonObject);
        for (size_t i = 0; i < count; i++)
        {
            const char* key = json_object_get_name(jsonObject, i);
            if (key)
            {
                jsonKeys.insert(std::string(key));
            }
        }

        // Validate parameters against the event's parameter set
        if (!ValidateEventParameters(std::string(eventName), jsonKeys))
        {
            OsConfigLogError(m_log, "Parameter validation failed for event '%s': %s", eventName, jsonLine.c_str());
            json_value_free(jsonValue);
            return;
        }

        // Create event with the event name
        OsConfigLogDebug(m_log, "Processing event: %s", eventName);
        EventProperties event(eventName);

        // Iterate over all key/value pairs in the JSON object
        for (size_t i = 0; i < count; i++)
        {
            const char* key = json_object_get_name(jsonObject, i);
            if (!key || strcmp(key, "EventName") == 0)
            {
                // Skip the EventName since it's already used for the event type
                continue;
            }

            JSON_Value* value = json_object_get_value(jsonObject, key);
            if (!value)
            {
                continue;
            }

            OsConfigLogDebug(m_log, "Processing key: %s", key);

            // Handle different JSON value types
            JSON_Value_Type valueType = json_value_get_type(value);
            switch (valueType)
            {
                case JSONString:
                {
                    const char* stringValue = json_value_get_string(value);
                    if (stringValue)
                    {
                        event.SetProperty(key, std::string(stringValue));
                    }
                    break;
                }
                case JSONNumber:
                {
                    double numberValue = json_value_get_number(value);
                    event.SetProperty(key, numberValue);
                    break;
                }
                case JSONBoolean:
                {
                    int boolValue = json_value_get_boolean(value);
                    event.SetProperty(key, boolValue != 0);
                    break;
                }
                case JSONNull:
                {
                    // For null values, we could either skip them or set as empty string
                    event.SetProperty(key, std::string(""));
                    break;
                }
                case JSONObject:
                case JSONArray:
                {
                    // For complex types (objects/arrays), serialize them as strings
                    char* serializedValue = json_serialize_to_string(value);
                    if (serializedValue)
                    {
                        event.SetProperty(key, std::string(serializedValue));
                        json_free_serialized_string(serializedValue);
                    }
                    break;
                }
                default:
                    // Skip unknown types
                    break;
            }
        }

        // Log the event with all properties
        m_logger->LogEvent(event);
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(m_log, "Exception processing JSON line '%s': %s", jsonLine.c_str(), e.what());
    }

    // Cleanup
    if (jsonValue)
    {
        json_value_free(jsonValue);
    }
}

} // namespace Telemetry
