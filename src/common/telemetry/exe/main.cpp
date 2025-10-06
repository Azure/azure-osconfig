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

struct CommandLineArgs
{
    bool verbose;
    std::string filepath;
    int teardown_time;
};

bool parse_command_line_args(int argc, char* argv[], CommandLineArgs& args, OsConfigLogHandle log)
{
    args.verbose = false;
    args.teardown_time = -1; // Use -1 to indicate default should be used

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
                args.verbose = true;
                break;
            default:
                print_usage(argv[0]);
                return false;
        }
    }

    // Get filepath (first non-option argument)
    if (optind >= argc)
    {
        OsConfigLogError(log, "Error: JSON file path is required.");
        print_usage(argv[0]);
        return false;
    }
    args.filepath = argv[optind++];

    // Parse optional teardown time argument
    if (optind < argc)
    {
        try
        {
            args.teardown_time = std::stoi(argv[optind]);
            if (args.teardown_time < 0)
            {
                OsConfigLogError(log, "Error: Teardown time must be a non-negative integer.");
                return false;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(log, "Error: Invalid teardown time argument '%s'. Must be a valid integer.", argv[optind]);
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    LogHandle log("/var/log/osconfig_telemetry_exe.log");

    CommandLineArgs args;
    if (!parse_command_line_args(argc, argv, args, log.get()))
    {
        OsConfigLogError(log.get(), "Error: Failed to parse command line arguments.");
        return 1;
    }

    try
    {
        std::string init_message = "Initializing telemetry with verbose=" + std::string(args.verbose ? "true" : "false");
        if (args.teardown_time >= 0)
        {
            init_message += " and teardown_time=" + std::to_string(args.teardown_time) + "s";
        }
        OsConfigLogInfo(log.get(), "%s", init_message.c_str());
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(log.get(), "Error: Failed to create initialization message: %s", e.what());
        return 1;
    }

    try
    {
        OsConfigLogInfo(log.get(), "Telemetry initialized successfully!");

        Telemetry::TelemetryManager::SetupConfiguration(args.verbose, args.teardown_time);
        Telemetry::TelemetryManager::ProcessJsonFile(args.filepath);
        OsConfigLogInfo(log.get(), "Processed telemetry JSON file: %s", args.filepath.c_str());

        auto start_time = std::chrono::high_resolution_clock::now();
        Telemetry::TelemetryManager::Shutdown();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::string duration_str = std::to_string(duration.count());
        OsConfigLogInfo(log.get(), "Telemetry shutdown successfully! [%s ms]", duration_str.c_str());

        if (std::remove(args.filepath.c_str()) != 0)
        {
            OsConfigLogError(log.get(), "Warning: Failed to delete JSON file: %s", args.filepath.c_str());
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(log.get(), "Error: Telemetry operation failed: %s", e.what());
        return 1;
    }

    return 0;
}
