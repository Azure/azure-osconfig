// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "main.h"

#include <getopt.h>
#include <iostream>
#include <Logging.h>

void PrintUsage(const char* program_name)
{
    std::cout << "Usage: " << program_name << " [OPTIONS] <json_file_path>" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  json_file_path           Path to the JSON file to process (required)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -f, --file <path>        Specify JSON file path (alternative to positional arg)" << std::endl;
    std::cout << "  -v, --verbose            Enable verbose/debug output" << std::endl;
    std::cout << "  -t, --teardown <seconds> Set teardown time in seconds (default: 5)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " /path/to/events.json" << std::endl;
    std::cout << "  " << program_name << " -v -t 10 /path/to/events.json" << std::endl;
    std::cout << "  " << program_name << " --file /path/to/events.json --verbose" << std::endl;
    std::cout << std::endl;
}

bool ParseCommandLineArgs(int argc, char* argv[], CommandLineArgs& args, OsConfigLogHandle log)
{
    args.verbose = false;
    args.teardown_time = 5; // CONFIG_DEFAULT_TEARDOWN_TIME
    args.filepath.clear();

    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"teardown", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    // Parse options
    // Reset getopt state for multiple calls in the same process (important for tests)
    // GNU getopt requires these to be reset between calls
    optind = 0;  // Forces reinitialization in GNU getopt
    opterr = 0;  // Suppress error messages
    optopt = 0;  // Reset last parsed option

    int opt;
    while ((opt = getopt_long(argc, argv, "f:vt:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'f':
                args.filepath = optarg;
                break;
            case 'v':
                args.verbose = true;
                break;
            case 't':
                if (optarg)
                {
                    try
                    {
                        args.teardown_time = std::stoi(optarg);
                        if (args.teardown_time < 0)
                        {
                            OsConfigLogError(log, "Error: Teardown time must be a non-negative integer.");
                            PrintUsage(argv[0]);
                            return false;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        OsConfigLogError(log, "Error: Invalid teardown time value.");
                        PrintUsage(argv[0]);
                        return false;
                    }
                }
                break;
            case '?':
                OsConfigLogError(log, "Error: Unknown option or missing argument.");
                PrintUsage(argv[0]);
                return false;
            default:
                PrintUsage(argv[0]);
                return false;
        }
    }

    // If filepath not provided via -f option, try to get it from positional argument
    if (args.filepath.empty())
    {
        if (optind < argc)
        {
            args.filepath = argv[optind];
            optind++;
        }
    }

    // Check if filepath was provided either way
    if (args.filepath.empty())
    {
        OsConfigLogError(log, "Error: JSON file path is required. Provide as argument or use -f option.");
        PrintUsage(argv[0]);
        return false;
    }

    // Check for extra non-option arguments
    if (optind < argc)
    {
        OsConfigLogError(log, "Error: Too many arguments provided.");
        PrintUsage(argv[0]);
        return false;
    }

    return true;
}
