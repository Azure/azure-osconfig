// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <Logging.h>

#include <Telemetry.hpp>

bool file_exists(const std::string& filepath)
{
    std::ifstream file(filepath.c_str());
    return file.good();
}

int shutdown(OsConfigLogHandle log, int exit_code)
{
    CloseLog(&log);
    return exit_code;
}

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
    OsConfigLogHandle g_log = NULL;
    g_log = OpenLog("/var/log/osconfig_telemetry_exe.log", NULL);

    // Check if minimum arguments are provided
    if (argc < 2 || argc > 4)
    {
        print_usage(argv[0]);
        return shutdown(g_log, 1);
    }

    bool verbose = false;
    std::string filepath;
    int teardown_time = -1; // Use -1 to indicate default should be used
    int arg_index = 1;

    // Parse verbose flag
    if (argc > 1 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--verbose"))
    {
        verbose = true;
        arg_index = 2;

        if (argc < 3)
        {
            OsConfigLogError(g_log, "Error: JSON file path is required after verbose flag.");
            print_usage(argv[0]);
            return shutdown(g_log, 1);
        }
    }

    // Get filepath
    if (arg_index < argc)
    {
        filepath = argv[arg_index];
        arg_index++;
    }
    else
    {
        OsConfigLogError(g_log, "Error: JSON file path is required.");
        print_usage(argv[0]);
        return shutdown(g_log, 1);
    }

    // Parse teardown time argument
    if (arg_index < argc)
    {
        try
        {
            teardown_time = std::stoi(argv[arg_index]);
            if (teardown_time < 0)
            {
                OsConfigLogError(g_log, "Error: Teardown time must be a non-negative integer.");
                return shutdown(g_log, 1);
            }
            arg_index++;
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(g_log, "Error: Invalid teardown time argument '%s'. Must be a valid integer.", argv[arg_index]);
            return shutdown(g_log, 1);
        }
    }

    // Check if file exists
    if (!file_exists(filepath))
    {
        OsConfigLogError(g_log, "Error: File '%s' does not exist.", filepath.c_str());
        return shutdown(g_log, 1);
    }

    // Initialize telemetry
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();
    std::string init_message = "Initializing telemetry with verbose=" + std::string(verbose ? "true" : "false");
    if (teardown_time >= 0)
    {
        init_message += " and teardown_time=" + std::to_string(teardown_time) + "s";
    }
    OsConfigLogInfo(g_log, "%s", init_message.c_str());

    bool init_success;
    if (teardown_time >= 0)
    {
        init_success = telemetry.Initialize(verbose, teardown_time);
    }
    else
    {
        init_success = telemetry.Initialize(verbose);
    }

    if (init_success)
    {
        OsConfigLogInfo(g_log, "Telemetry initialized successfully!");

        telemetry.ProcessJsonFile(filepath);
        OsConfigLogInfo(g_log, "Processed telemetry JSON file: %s", filepath.c_str());

        auto start_time = std::chrono::high_resolution_clock::now();
        telemetry.Shutdown();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        OsConfigLogInfo(g_log, "Telemetry shutdown successfully! [%lld ms]", duration.count());

        // Delete the JSON file
        if (std::remove(filepath.c_str()) != 0)
        {
            OsConfigLogError(g_log, "Warning: Failed to delete JSON file: %s", filepath.c_str());
        }
    }
    else
    {
        OsConfigLogError(g_log, "Error: Failed to initialize telemetry.");
        return shutdown(g_log, 1);
    }

    CloseLog(&g_log);

    return 0;
}
