// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.hpp"
#include "ParameterSets.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

using namespace MAT;

namespace Telemetry
{

TelemetryManager::TelemetryManager(bool enableDebug, int teardownTime)
    : m_log(nullptr)
    , m_logger(nullptr)
{
    OsConfigLogInfo(m_log, "Initializing telemetry...");

    try
    {
        m_log = OpenLog("/var/log/osconfig_telemetry.log", NULL);

        ILogConfiguration logConfig;
        logConfig["name"] = INSTANCE_NAME;
        logConfig["version"] = TELEMETRY_VERSION;
        logConfig["config"]["host"] = INSTANCE_NAME;
        logConfig["primaryToken"] = API_KEY;
        logConfig[CFG_BOOL_ENABLE_TRACE] = enableDebug;
        logConfig[CFG_INT_TRACE_LEVEL_MIN] = 0;
        logConfig[CFG_INT_MAX_TEARDOWN_TIME] = teardownTime;
        logConfig[CFG_STR_START_PROFILE_NAME] = TRANSMITPROFILE_REALTIME;

        status_t status = STATUS_SUCCESS;
        m_logger = LogManagerProvider::CreateLogManager(logConfig, status);
        if (STATUS_SUCCESS != status)
        {
            throw std::runtime_error("Failed to initialize MAT");
        }

        OsConfigLogInfo(m_log, "Telemetry initialized successfully.");
    }
    catch (const std::exception& e)
    {
        if (m_log)
        {
            OsConfigLogError(m_log, "Failed to initialize telemetry: %s", e.what());
            CloseLog(&m_log);
        }
        throw;
    }
}

TelemetryManager::~TelemetryManager() noexcept
{
    OsConfigLogInfo(m_log, "Shutting down telemetry...");

    try
    {
        m_logger->UploadNow();
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Without sleep, the upload may not complete
        m_logger->FlushAndTeardown();
        OsConfigLogInfo(m_log, "Telemetry shutdown complete.");
        CloseLog(&m_log);
    }
    catch (const std::exception& e)
    {
        if (m_log)
        {
            OsConfigLogError(m_log, "Exception during shutdown: %s", e.what());
            CloseLog(&m_log);
        }
    }
    if (m_log)
    {
        CloseLog(&m_log);
    }
}

void TelemetryManager::EventWrite(Microsoft::Applications::Events::EventProperties event)
{
    if (!m_logger)
    {
        OsConfigLogError(m_log, "Telemetry not initialized");
        return;
    }

    m_logger->LogEvent(event);
}

bool TelemetryManager::ProcessJsonFile(const std::string& filePath)
{
    if (!m_logger)
    {
        OsConfigLogError(m_log, "TelemetryManager not initialized");
        return false;
    }

    OsConfigLogInfo(m_log, "Processing JSON file: %s", filePath.c_str());

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        OsConfigLogError(m_log, "Failed to open file: %s", filePath.c_str());
        return false;
    }

    bool success = true;
    try
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty())
            {
                continue;
            }
            ProcessJsonLine(line);
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(m_log, "Error processing JSON file '%s': %s", filePath.c_str(), e.what());
        success = false;
    }

    OsConfigLogInfo(m_log, "Completed processing JSON file: %s", filePath.c_str());

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
