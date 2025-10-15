// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Telemetry.hpp"
#include "ParameterSets.hpp"

LOGMANAGER_INSTANCE

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

using namespace MAT;

namespace Telemetry
{

TelemetryManager::TelemetryManager(bool enableDebug, int teardownTime, OsConfigLogHandle logHandle)
    : m_log(logHandle)
    , m_logger(nullptr)
{
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    SetLoggingLevel(LoggingLevelDebug);
#endif

    ILogConfiguration& logConfig = LogManager::GetLogConfiguration();
    logConfig["name"] = "TelemetryModule";
    logConfig["version"] = TELEMETRY_VERSION;
    logConfig["config"] = { { "host", "*" } };
    logConfig["primaryToken"] = API_KEY;
    logConfig[CFG_BOOL_ENABLE_TRACE] = enableDebug;
    logConfig[CFG_INT_TRACE_LEVEL_MIN] = 0;
    logConfig[CFG_INT_MAX_TEARDOWN_TIME] = teardownTime;
    logConfig[CFG_STR_START_PROFILE_NAME] = TRANSMITPROFILE_REALTIME;

    LogManager::Initialize(API_KEY);
    m_logger.reset(LogManager::GetInstance());

    OsConfigLogInfo(m_log, "Telemetry initialized successfully.");
}

TelemetryManager::~TelemetryManager() noexcept
{
    OsConfigLogInfo(m_log, "Telemetry shutting down...");
    m_logger->UploadNow();
    OsConfigLogInfo(m_log, "Telemetry shutdown complete.");
}

void TelemetryManager::EventWrite(Microsoft::Applications::Events::EventProperties event)
{
    ILogger* logger = LogManager::GetLogger();
    if (!logger)
    {
        OsConfigLogError(m_log, "Failed to get logger instance");
        return;
    }
    logger->LogEvent(event);
}

bool TelemetryManager::ProcessJsonFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        OsConfigLogError(m_log, "Failed to open file: %s", filePath.c_str());
        return false;
    }

    OsConfigLogInfo(m_log, "Processing JSON file: %s", filePath.c_str());

    std::string line;
    while (std::getline(file, line) && !line.empty())
    {
        ProcessJsonLine(line);
    }

    return true;
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
    OsConfigLogDebug(m_log, "Processing JSON line: %s", jsonLine.c_str());

    nlohmann::json jsonObject;
    try
    {
        jsonObject = nlohmann::json::parse(jsonLine);
    }
    catch(const nlohmann::json::parse_error& e)
    {
        // Ignore invalid JSON lines
        return;
    }

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

        // Handle different JSON value types
        switch (value.type())
        {
            case nlohmann::json::value_t::string:
                event.SetProperty(key, value.get<std::string>());
                break;

            case nlohmann::json::value_t::number_float:
                event.SetProperty(key, value.get<double>());
                break;

            case nlohmann::json::value_t::number_integer:
            case nlohmann::json::value_t::number_unsigned:
                // Convert integer to double for consistency
                event.SetProperty(key, static_cast<double>(value.get<int64_t>()));
                break;

            case nlohmann::json::value_t::boolean:
                event.SetProperty(key, value.get<bool>());
                break;

            case nlohmann::json::value_t::null:
                event.SetProperty(key, std::string(""));
                break;

            case nlohmann::json::value_t::object:
            case nlohmann::json::value_t::array:
                // For complex types (objects/arrays), serialize them as strings
                event.SetProperty(key, value.dump());
                break;

            default:
                OsConfigLogWarning(m_log, "Unexpected JSON type for key '%s'", key.c_str());
                break;
        }
    }

    // Log the event with all properties
    ILogger* logger = LogManager::GetLogger();
    if (!logger)
    {
        OsConfigLogError(m_log, "Failed to get logger instance");
        return;
    }
    logger->LogEvent(event);
    OsConfigLogDebug(m_log, "Successfully logged event to MAT");
}

} // namespace Telemetry
