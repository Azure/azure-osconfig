// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "telemetryjson.hpp"

#include <chrono>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <parson.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

class TelemetryJsonLogger
{
private:
    std::ofstream logFile;
    std::string filename;
    std::string binaryDirectory;
    bool isOpen;

    std::string getCurrentTimestamp() const
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        char buffer[32];
        struct tm* timeInfo = std::gmtime(&time_t);
        if (timeInfo && strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeInfo) > 0)
        {
            return std::string(buffer) + "Z";
        }
        return "";
    }

    std::string generateRandomFilename() const
    {
        // Create a temporary file
        char temp_template[] = "/tmp/telemetry_XXXXXX";
        int fd = mkstemp(temp_template);
        if (fd == -1)
        {
            return "";
        }

        if (unlink(temp_template) != 0)
        {
            // Handle error - could log it or check if it's acceptable to ignore
            perror("Failed to delete temporary file");
        }

        // Append .json to the unique name
        std::string result = temp_template;
        result += ".json";

        return result;
    }

    void RunTelemetryProxy(const char* telemetryJSONFile)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            if (binaryDirectory.empty())
            {
                exit(ENOENT);
            }

            // Execute telemetry application
            // Usage: telemetry [OPTIONS] <json_file_path> [teardown_time_seconds] [syslog_identifier]
            //     json_file_path         - Path to the JSON file to process
            //     teardown_time_seconds  - Optional: Teardown time in seconds (default: 5s)
            //     syslog_identifier      - Optional: Custom identifier for syslog entries (default: telemetry-exe)

            //     Options:
            //     -v, --verbose          - Enable verbose output
            std::string path = binaryDirectory + std::string("/") + TELEMETRY_BINARY_NAME;
            execl(path.c_str(), TELEMETRY_BINARY_NAME, "-v", telemetryJSONFile, "5", TELEMETRY_SYSLOG_IDENTIFIER, (char*)NULL);
            exit(errno);
        }
        else if (pid > 0)
        {
            // Parent process waits for child to exit
            waitpid(pid, NULL, 0);
        }
    }

public:
    TelemetryJsonLogger() : isOpen(false) {}

    ~TelemetryJsonLogger()
    {
        if (isOpen)
        {
            close();
        }
    }

    bool open()
    {
        if (isOpen)
        {
            return false; // Already open
        }

        filename = generateRandomFilename();
        if (filename.empty())
        {
            return false;
        }

        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open())
        {
            return false;
        }

        isOpen = true;

        return true;
    }

    bool close()
    {
        if (!isOpen)
        {
            return false;
        }

        if (logFile.is_open())
        {
            logFile.close();
        }

        isOpen = false;

        // Call telemetry proxy if binary directory is set
        if (!binaryDirectory.empty())
        {
            RunTelemetryProxy(filename.c_str());
        }
        else
        {
            // Binary directory not set, cannot run telemetry proxy
            return false;
        }

        return true;
    }

    bool logEvent(const std::string& eventName, const char** keyValuePairs, int pairCount)
    {
        if (!isOpen || !logFile.is_open())
        {
            return false;
        }

        // Create JSON value and get root object
        JSON_Value* rootValue = json_value_init_object();
        if (rootValue == nullptr)
        {
            return false;
        }

        JSON_Object* rootObject = json_value_get_object(rootValue);
        if (rootObject == nullptr)
        {
            json_value_free(rootValue);
            return false;
        }

        // Add timestamp
        std::string timestamp = getCurrentTimestamp();
        if (json_object_set_string(rootObject, "Timestamp", timestamp.c_str()) != JSONSuccess)
        {
            json_value_free(rootValue);
            return false;
        }

        // Add event name
        if (json_object_set_string(rootObject, "EventName", eventName.c_str()) != JSONSuccess)
        {
            json_value_free(rootValue);
            return false;
        }

        // Add key-value pairs directly to root object if provided
        if (keyValuePairs != nullptr && pairCount > 0)
        {
            for (int i = 0; i < pairCount; i++)
            {
                const char* key = keyValuePairs[i * 2];
                const char* value = keyValuePairs[i * 2 + 1];

                if (key != nullptr && value != nullptr)
                {
                    // Attempt to deduce the value type and serialize appropriately
                    std::string valueStr(value);
                    JSON_Status result = JSONFailure;

                    // Try to parse as boolean first (exact matches)
                    if (valueStr == "true" || valueStr == "false")
                    {
                        bool boolValue = (valueStr == "true");
                        result = json_object_set_boolean(rootObject, key, boolValue);
                    }
                    // Try to parse as null
                    else if (valueStr == "null")
                    {
                        result = json_object_set_null(rootObject, key);
                    }
                    // Try to parse as integer
                    else
                    {
                        char* endPtr = nullptr;
                        long longValue = strtol(value, &endPtr, 10);

                        // Check if entire string was consumed and no overflow occurred
                        if (endPtr != nullptr && *endPtr == '\0' && endPtr != value)
                        {
                            // Check if it fits in int range
                            if (longValue >= INT_MIN && longValue <= INT_MAX)
                            {
                                result = json_object_set_number(rootObject, key, static_cast<double>(longValue));
                            }
                            else
                            {
                                // Value is too large for int, treat as string
                                result = json_object_set_string(rootObject, key, value);
                            }
                        }
                        else
                        {
                            // Try to parse as double
                            double doubleValue = strtod(value, &endPtr);

                            // Check if entire string was consumed
                            if (endPtr != nullptr && *endPtr == '\0' && endPtr != value)
                            {
                                result = json_object_set_number(rootObject, key, doubleValue);
                            }
                            else
                            {
                                // Not a number, treat as string
                                result = json_object_set_string(rootObject, key, value);
                            }
                        }
                    }

                    if (result != JSONSuccess)
                    {
                        json_value_free(rootValue);
                        return false;
                    }
                }
            }
        }

        // Serialize JSON to string
        char* jsonString = json_serialize_to_string(rootValue);
        json_value_free(rootValue);

        if (jsonString == nullptr)
        {
            return false;
        }

        // Write to file
        logFile << jsonString << std::endl;
        logFile.flush();

        // Free the serialized string
        json_free_serialized_string(jsonString);

        return true;
    }

    // Overloaded method for convenience with initializer list
    bool logEvent(const std::string& eventName,
                  const std::vector<std::pair<std::string, std::string>>& keyValuePairs)
    {
        if (keyValuePairs.empty())
        {
            return logEvent(eventName, nullptr, 0);
        }

        // Convert to C-style array
        std::vector<const char*> cStyleArray;
        cStyleArray.reserve(keyValuePairs.size() * 2);

        for (const auto& pair : keyValuePairs)
        {
            cStyleArray.push_back(pair.first.c_str());
            cStyleArray.push_back(pair.second.c_str());
        }

        return logEvent(eventName, cStyleArray.data(), static_cast<int>(keyValuePairs.size()));
    }

    bool isLoggerOpen() const
    {
        return isOpen;
    }

    const std::string& getFilename() const
    {
        return filename;
    }

    void setBinaryDirectory(const std::string& directory)
    {
        binaryDirectory = directory;
    }

    const std::string& getBinaryDirectory() const
    {
        return binaryDirectory;
    }
};

// C++ interface implementation
namespace TelemetryJson
{

class Logger::Impl
{
public:
    TelemetryJsonLogger logger;
};

Logger::Logger() : m_impl(std::unique_ptr<Impl>(new Impl())) {}

Logger::~Logger() = default;

Logger::Logger(Logger&& other) noexcept : m_impl(std::move(other.m_impl)) {}

Logger& Logger::operator=(Logger&& other) noexcept
{
    if (this != &other)
    {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

bool Logger::open()
{
    return m_impl ? m_impl->logger.open() : false;
}

bool Logger::close()
{
    return m_impl ? m_impl->logger.close() : false;
}

bool Logger::logEvent(const std::string& eventName, const char** keyValuePairs, int pairCount)
{
    return m_impl ? m_impl->logger.logEvent(eventName, keyValuePairs, pairCount) : false;
}

bool Logger::logEvent(const std::string& eventName,
                      std::initializer_list<std::pair<std::string, std::string>> keyValuePairs)
{
    std::vector<std::pair<std::string, std::string>> pairs(keyValuePairs);
    return m_impl ? m_impl->logger.logEvent(eventName, pairs) : false;
}

bool Logger::logEvent(const std::string& eventName)
{
    return m_impl ? m_impl->logger.logEvent(eventName, nullptr, 0) : false;
}

bool Logger::isOpen() const
{
    return m_impl ? m_impl->logger.isLoggerOpen() : false;
}

const std::string& Logger::getFilename() const
{
    static const std::string empty;
    return m_impl ? m_impl->logger.getFilename() : empty;
}

void Logger::setBinaryDirectory(const std::string& directory)
{
    if (m_impl)
    {
        m_impl->logger.setBinaryDirectory(directory);
    }
}

const std::string& Logger::getBinaryDirectory() const
{
    static const std::string empty;
    return m_impl ? m_impl->logger.getBinaryDirectory() : empty;
}

} // namespace TelemetryJson

// C interface implementations
extern "C"
{

TelemetryHandle Telemetry_Open(void)
{
    try
    {
        auto logger = new TelemetryJsonLogger();
        if (logger->open())
        {
            return reinterpret_cast<TelemetryHandle>(logger);
        }
        else
        {
            delete logger;
            return nullptr;
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

int Telemetry_Close(TelemetryHandle* handle)
{
    if (handle == nullptr || *handle == nullptr)
    {
        return -1;
    }
    auto logger = reinterpret_cast<TelemetryJsonLogger*>(*handle);
    delete logger;
    *handle = nullptr;
    return 0;
}

int Telemetry_LogEvent(TelemetryHandle handle, const char* eventName,
                          const char** keyValuePairs, int pairCount)
{
    if (handle == nullptr || eventName == nullptr)
    {
        return -1;
    }

    try
    {
        auto logger = reinterpret_cast<TelemetryJsonLogger*>(handle);
        bool success = logger->logEvent(eventName, keyValuePairs, pairCount);
        return success ? 0 : -1;
    }
    catch (...)
    {
        return -1;
    }
}

int Telemetry_SetBinaryDirectory(TelemetryHandle handle, const char* directory)
{
    if (handle == nullptr || directory == nullptr)
    {
        return -1;
    }

    try
    {
        auto logger = reinterpret_cast<TelemetryJsonLogger*>(handle);
        logger->setBinaryDirectory(directory);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

const char* Telemetry_GetFilepath(TelemetryHandle handle)
{
    if (handle == nullptr)
    {
        return nullptr;
    }

    try
    {
        auto logger = reinterpret_cast<TelemetryJsonLogger*>(handle);
        return logger->getFilename().c_str();
    }
    catch (...)
    {
        return nullptr;
    }
}

} // extern "C"
