#include "telemetry.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace MAT;

LOGMANAGER_INSTANCE

namespace Telemetry
{

TelemetryManager::TelemetryManager()
    : m_logger(nullptr),
      m_initialized(false)
{
}

TelemetryManager::~TelemetryManager()
{
    if (m_initialized)
    {
        Shutdown();
    }
}

TelemetryManager& TelemetryManager::GetInstance()
{
    static TelemetryManager instance;
    return instance;
}

void TelemetryManager::SetupConfiguration(bool enableDebug, int teardownTime)
{
    auto& logConfig = LogManager::GetLogConfiguration();

    logConfig[CFG_BOOL_ENABLE_TRACE] = enableDebug;
    logConfig[CFG_INT_TRACE_LEVEL_MIN] = 0;
    logConfig[CFG_INT_MAX_TEARDOWN_TIME] = teardownTime;
}

bool TelemetryManager::Initialize(bool enableDebug, int teardownTime)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized)
    {
        return true;
    }

    try
    {
        SetupConfiguration(enableDebug, teardownTime);

        // Initialize the logger
        m_logger = LogManager::Initialize(API_KEY);
        if (!m_logger)
        {
            return false;
        }

        LogManager::SetTransmitProfile(TransmitProfile::TransmitProfile_RealTime);

        m_initialized = true;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to initialize telemetry: " << e.what() << std::endl;
        return false;
    }
}

bool TelemetryManager::IsInitialized() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

void TelemetryManager::LogEvent(const std::string& eventName)
{
    if (!m_initialized || !m_logger)
    {
        return;
    }

    try
    {
        m_logger->LogEvent(eventName);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to log event '" << eventName << "': " << e.what() << std::endl;
    }
}

void TelemetryManager::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        return;
    }

    try
    {
        LogManager::UploadNow();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Without sleep, the upload may not complete
        LogManager::FlushAndTeardown();

        m_initialized = false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error during telemetry shutdown: " << e.what() << std::endl;
    }
}

bool TelemetryManager::ProcessJsonFile(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        std::cerr << "TelemetryManager not initialized" << std::endl;
        return false;
    }

    FILE* file = fopen(filePath.c_str(), "r");
    if (!file)
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
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
        std::cerr << "Error processing JSON file '" << filePath << "': " << e.what() << std::endl;
        success = false;
    }

    // Cleanup
    if (line)
    {
        free(line);
    }
    fclose(file);

    return success;
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
            std::cerr << "Failed to parse JSON line: " << jsonLine << std::endl;
            return;
        }

        jsonObject = json_value_get_object(jsonValue);
        if (!jsonObject)
        {
            std::cerr << "JSON line is not an object: " << jsonLine << std::endl;
            json_value_free(jsonValue);
            return;
        }

        // Extract event name - required field
        const char* eventName = json_object_get_string(jsonObject, "eventName");
        if (!eventName)
        {
            std::cerr << "JSON object missing 'eventName' field: " << jsonLine << std::endl;
            json_value_free(jsonValue);
            return;
        }

        // Create event with the event name
        std::string fullEventName = std::string("OSConfig.") + eventName;
        std::cout << "Processing event: " << fullEventName << std::endl;
        EventProperties event(fullEventName);

        // Iterate over all key/value pairs in the JSON object
        size_t count = json_object_get_count(jsonObject);
        for (size_t i = 0; i < count; i++)
        {
            const char* key = json_object_get_name(jsonObject, i);
            if (!key || strcmp(key, "eventName") == 0)
            {
                // Skip the eventName since it's already used for the event type
                continue;
            }

            JSON_Value* value = json_object_get_value(jsonObject, key);
            if (!value)
            {
                continue;
            }

            std::cout << "Processing key: " << key << std::endl;

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
        std::cerr << "Exception processing JSON line '" << jsonLine << "': " << e.what() << std::endl;
    }

    // Cleanup
    if (jsonValue)
    {
        json_value_free(jsonValue);
    }
}

} // namespace Telemetry
