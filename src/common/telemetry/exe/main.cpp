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

static OsConfigLogHandle g_log;
static const char* g_logFile = "/var/log/osconfig_telemetry_exe.log";
static const char* g_rolledLogFile = "/var/log/osconfig_telemetry_exe.bak";

void __attribute__((constructor)) Init(void)
{
    g_log = OpenLog(g_logFile, g_rolledLogFile);
    assert(NULL != g_log);
}

void __attribute__((destructor)) Destroy(void)
{
    OsConfigLogInfo(g_log, "Telemetry shutdown successfully!");
    CloseLog(&g_log);
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
    opterr = 0; // Suppress getopt error messages
    optind = 1; // Reset option index
    while ((opt = getopt_long(argc, argv, "+v", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'v':
                args.verbose = true;
                break;
            case '?':
                // Unknown option or missing argument - ignore and continue
                break;
            default:
                print_usage(argv[0]);
                return false;
        }
    }

    // Collect non-option arguments by scanning all args
    std::vector<std::string> non_option_args;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg != "-v" && arg != "--verbose")
        {
            non_option_args.push_back(arg);
        }
    }

    // Get filepath (first non-option argument)
    if (non_option_args.empty())
    {
        OsConfigLogError(log, "Error: JSON file path is required.");
        print_usage(argv[0]);
        return false;
    }
    args.filepath = non_option_args[0];

    // Parse optional teardown time argument (second non-option argument)
    if (non_option_args.size() > 1)
    {
        try
        {
            args.teardown_time = std::stoi(non_option_args[1]);
            if (args.teardown_time < 0)
            {
                OsConfigLogError(log, "Error: Teardown time must be a non-negative integer.");
                return false;
            }
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(log, "Error: Invalid teardown time argument '%s'. Must be a valid integer.", non_option_args[1].c_str());
            return false;
        }
    }

    if (non_option_args.size() > 2)
    {
        OsConfigLogError(log, "Error: Too many arguments provided.");
        print_usage(argv[0]);
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    CommandLineArgs args;
    if (!parse_command_line_args(argc, argv, args, g_log))
    {
        OsConfigLogError(g_log, "Error: Failed to parse command line arguments.");
        return 1;
    }

    std::string init_message = "Initializing telemetry with verbose=" + std::string(args.verbose ? "true" : "false");
    if (args.teardown_time >= 0)
    {
        init_message += " and teardown_time=" + std::to_string(args.teardown_time) + "s";
    }
    OsConfigLogInfo(g_log, "%s", init_message.c_str());

    try
    {
        int teardownTime = (args.teardown_time >= 0) ? args.teardown_time : Telemetry::TelemetryManager::CONFIG_DEFAULT_TEARDOWN_TIME;
        auto start_init = std::chrono::high_resolution_clock::now();
        Telemetry::TelemetryManager telemetryManager(args.verbose, teardownTime);
        auto end_init = std::chrono::high_resolution_clock::now();
        auto init_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_init - start_init);
        OsConfigLogInfo(g_log, "Telemetry initialized successfully! [%s ms]", std::to_string(init_duration.count()).c_str());
        telemetryManager.ProcessJsonFile(args.filepath);
        OsConfigLogInfo(g_log, "Processed telemetry JSON file: %s", args.filepath.c_str());

        if (std::remove(args.filepath.c_str()) != 0)
        {
            OsConfigLogError(g_log, "Warning: Failed to delete JSON file: %s", args.filepath.c_str());
        }
    }
    catch (const std::exception& e)
    {
        OsConfigLogError(g_log, "Error: Telemetry operation failed: %s", e.what());
        return 1;
    }

    return 0;
}
