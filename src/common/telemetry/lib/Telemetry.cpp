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
        OsConfigLogError(m_log, "Telemetry not initialized");
        return;
    }

    m_logger->LogEvent(event);

}

void TelemetryManager::Shutdown()
{
    if (!m_initialized)
    {
        OsConfigLogError(m_log, "Telemetry not initialized");
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

    try
    {
        // Parse the JSON string
        nlohmann::json jsonObject = nlohmann::json::parse(jsonLine);

        // Verify it's an object
        if (!jsonObject.is_object())
        {
            OsConfigLogError(m_log, "JSON line is not an object: %s", jsonLine.c_str());
            return;
        }

        // Extract event name - required field
        if (!jsonObject.contains("EventName") || !jsonObject["EventName"].is_string())
        {
            OsConfigLogError(m_log, "JSON object missing 'EventName' field: %s", jsonLine.c_str());
            return;
        }

        std::string eventName = jsonObject["EventName"].get<std::string>();

        // Collect all JSON keys for validation
        std::set<std::string> jsonKeys;
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
        {
            jsonKeys.insert(it.key());
        }

        // Validate parameters against the event's parameter set
        if (!ValidateEventParameters(eventName, jsonKeys))
        {
            OsConfigLogError(m_log, "Parameter validation failed for event '%s': %s", eventName.c_str(), jsonLine.c_str());
            return;
        }

        // Create event with the event name
        OsConfigLogDebug(m_log, "Processing event: %s", eventName.c_str());
        EventProperties event(eventName);

        // Iterate over all key/value pairs in the JSON object
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
        {
            const std::string& key = it.key();
            if (key == "EventName")
            {
                // Skip the EventName since it's already used for the event type
                continue;
            }

            const auto& value = it.value();

            OsConfigLogDebug(m_log, "Processing key: %s", key.c_str());

            // Handle different JSON value types
            if (value.is_string())
            {
                event.SetProperty(key, value.get<std::string>());
            }
            else if (value.is_number_float())
            {
                event.SetProperty(key, value.get<double>());
            }
            else if (value.is_number_integer())
            {
                // Convert integer to double for consistency
                event.SetProperty(key, static_cast<double>(value.get<int64_t>()));
            }
            else if (value.is_boolean())
            {
                event.SetProperty(key, value.get<bool>());
            }
            else if (value.is_null())
            {
                // For null values, we could either skip them or set as empty string
                event.SetProperty(key, std::string(""));
            }
            else if (value.is_object() || value.is_array())
            {
                // For complex types (objects/arrays), serialize them as strings
                event.SetProperty(key, value.dump());
            }
        }

        // Log the event with all properties
        m_logger->LogEvent(event);
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(m_log, "Exception processing JSON line '%s': %s", jsonLine.c_str(), e.what());
    }
}

} // namespace Telemetry
