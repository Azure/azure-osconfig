// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <syslog.h>

#include <telemetry.hpp>

void log_error(const std::string& message) {
    syslog(LOG_ERR, "%s", message.c_str());
}

void log_info(const std::string& message) {
    syslog(LOG_INFO, "%s", message.c_str());
    std::cout << message << std::endl;
}

bool file_exists(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    return file.good();
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <json_file_path> [teardown_time_seconds] [syslog_identifier]" << std::endl;
    std::cout << "  json_file_path         - Path to the JSON file to process" << std::endl;
    std::cout << "  teardown_time_seconds  - Optional: Teardown time in seconds (default: 5s)" << std::endl;
    std::cout << "  syslog_identifier      - Optional: Custom identifier for syslog entries (default: telemetry-exe)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --verbose          - Enable verbose output" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize syslog with default identifier (will be updated later if custom identifier is provided)
    openlog("telemetry-exe", LOG_CONS | LOG_PERROR | LOG_PID, LOG_USER);

    // Check if minimum arguments are provided
    if (argc < 2 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    bool verbose = false;
    std::string filepath;
    int teardown_time = -1; // Use -1 to indicate default should be used
    std::string syslog_identifier = "telemetry-exe"; // Default identifier
    int arg_index = 1;

    // Parse verbose flag
    if (argc > 1 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--verbose")) {
        verbose = true;
        arg_index = 2;

        if (argc < 3) {
            log_error("Error: JSON file path is required after verbose flag.");
            print_usage(argv[0]);
            return 1;
        }
    }

    // Get filepath
    if (arg_index < argc) {
        filepath = argv[arg_index];
        arg_index++;
    } else {
        log_error("Error: JSON file path is required.");
        print_usage(argv[0]);
        return 1;
    }

    // Parse teardown time argument
    if (arg_index < argc) {
        try {
            teardown_time = std::stoi(argv[arg_index]);
            if (teardown_time < 0) {
                log_error("Error: Teardown time must be a non-negative integer.");
                return 1;
            }
            arg_index++;
        } catch (const std::exception& e) {
            log_error("Error: Invalid teardown time argument '" + std::string(argv[arg_index]) + "'. Must be a valid integer.");
            return 1;
        }
    }

    // Parse syslog identifier argument
    if (arg_index < argc) {
        syslog_identifier = argv[arg_index];
        arg_index++;

        // Reinitialize syslog with the custom identifier
        closelog();
        openlog(syslog_identifier.c_str(), LOG_CONS | LOG_PERROR | LOG_PID, LOG_USER);
    }

    // Check if file exists
    if (!file_exists(filepath)) {
        log_error("Error: File '" + filepath + "' does not exist.");
        return 1;
    }

    // Initialize telemetry
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();
    std::string init_message = "Initializing telemetry with verbose=" + std::string(verbose ? "true" : "false");
    if (teardown_time >= 0) {
        init_message += " and teardown_time=" + std::to_string(teardown_time) + "s";
    }
    init_message += " and syslog_identifier=" + syslog_identifier;
    log_info(init_message);

    bool init_success;
    if (teardown_time >= 0) {
        init_success = telemetry.Initialize(verbose, teardown_time);
    } else {
        init_success = telemetry.Initialize(verbose);
    }

    if (init_success) {
        log_info("Telemetry initialized successfully!");

        telemetry.ProcessJsonFile(filepath);
        log_info("Processed telemetry JSON file: " + filepath);

        auto start_time = std::chrono::high_resolution_clock::now();
        telemetry.Shutdown();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        log_info("Telemetry shutdown successfully! [" + std::to_string(duration.count()) + " ms]");

        // Delete the JSON file
        if (std::remove(filepath.c_str()) != 0) {
            log_error("Warning: Failed to delete JSON file: " + filepath);
        }
    } else {
        log_error("Error: Failed to initialize telemetry.");
        return 1;
    }

    closelog();

    return 0;
}
