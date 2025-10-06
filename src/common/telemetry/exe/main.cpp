// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <Logging.h>
#include <string>
#include <unistd.h>

#include <Telemetry.hpp>

class LogHandle
{
private:
    OsConfigLogHandle m_log;

public:
    explicit LogHandle(const char* logPath, const char* rollingFile = nullptr)
        : m_log(OpenLog(logPath, rollingFile))
    {
    }

    ~LogHandle()
    {
        if (m_log != nullptr)
        {
            CloseLog(&m_log);
        }
    }

    OsConfigLogHandle get() const
    {
        return m_log;
    }
};

void print_usage(const char* program_name)
{
    std::cout << "Usage: " << program_name << " [OPTIONS] <json_file_path> [teardown_time_seconds]" << std::endl;
    std::cout << "  json_file_path         - Path to the JSON file to process" << std::endl;
    std::cout << "  teardown_time_seconds  - Optional: Teardown time in seconds (default: 5s)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --verbose          - Enable verbose output" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    LogHandle log("/var/log/osconfig_telemetry_exe.log");

    bool verbose = false;
    std::string filepath;
    int teardown_time = -1; // Use -1 to indicate default should be used

    // Define long options
    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    // Parse options
    int opt;
    while ((opt = getopt_long(argc, argv, "v", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'v':
                verbose = true;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Get filepath (first non-option argument)
    if (optind >= argc)
    {
        OsConfigLogError(log.get(), "Error: JSON file path is required.");
        print_usage(argv[0]);
        return 1;
    }
    filepath = argv[optind++];

    // Parse optional teardown time argument
    if (optind < argc)
    {
        try
        {
            teardown_time = std::stoi(argv[optind]);
            if (teardown_time < 0)
            {
                OsConfigLogError(log.get(), "Error: Teardown time must be a non-negative integer.");
                return 1;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(log.get(), "Error: Invalid teardown time argument '%s'. Must be a valid integer.", argv[optind]);
            return 1;
        }
    }

    try
    {
        std::string init_message = "Initializing telemetry with verbose=" + std::string(verbose ? "true" : "false");
        if (teardown_time >= 0)
        {
            init_message += " and teardown_time=" + std::to_string(teardown_time) + "s";
        }
        OsConfigLogInfo(log.get(), "%s", init_message.c_str());
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(log.get(), "Error: Failed to create initialization message: %s", e.what());
        return 1;
    }

    bool init_success = true;

    if (init_success)
    {
        try
        {
            OsConfigLogInfo(log.get(), "Telemetry initialized successfully!");

            Telemetry::TelemetryManager::SetupConfiguration(verbose, teardown_time);
            Telemetry::TelemetryManager::ProcessJsonFile(filepath);
            OsConfigLogInfo(log.get(), "Processed telemetry JSON file: %s", filepath.c_str());

            auto start_time = std::chrono::high_resolution_clock::now();
            Telemetry::TelemetryManager::Shutdown();
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::string duration_str = std::to_string(duration.count());
            OsConfigLogInfo(log.get(), "Telemetry shutdown successfully! [%s ms]", duration_str.c_str());

            // Delete the JSON file
            if (std::remove(filepath.c_str()) != 0)
            {
                OsConfigLogError(log.get(), "Warning: Failed to delete JSON file: %s", filepath.c_str());
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(log.get(), "Error: Telemetry operation failed: %s", e.what());
            return 1;
        }
    }
    else
    {
        OsConfigLogError(log.get(), "Error: Failed to initialize telemetry.");
        return 1;
    }

    return 0;
}
