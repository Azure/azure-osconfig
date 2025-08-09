#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

#include <telemetry.h>

bool file_exists(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    return file.good();
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <json_file_path> [teardown_time_seconds]" << std::endl;
    std::cout << "  json_file_path         - Path to the JSON file to process" << std::endl;
    std::cout << "  teardown_time_seconds  - Optional: Teardown time in seconds (default: 5s)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --verbose          - Enable debug mode" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Check if minimum arguments are provided
    if (argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    bool verbose = false;
    std::string filepath;
    int teardown_time = -1; // Use -1 to indicate default should be used
    int arg_index = 1;

    // Parse verbose flag
    if (argc > 1 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--verbose")) {
        verbose = true;
        arg_index = 2;

        if (argc < 3) {
            std::cerr << "Error: JSON file path is required after verbose flag." << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    // Get filepath
    if (arg_index < argc) {
        filepath = argv[arg_index];
        arg_index++;
    } else {
        std::cerr << "Error: JSON file path is required." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // Parse teardown time argument
    if (arg_index < argc) {
        try {
            teardown_time = std::stoi(argv[arg_index]);
            if (teardown_time < 0) {
                std::cerr << "Error: Teardown time must be a non-negative integer." << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid teardown time argument '" << argv[arg_index] << "'. Must be a valid integer." << std::endl;
            return 1;
        }
    }

    // Check if file exists
    if (!file_exists(filepath)) {
        std::cerr << "Error: File '" << filepath << "' does not exist." << std::endl;
        return 1;
    }

    // Initialize telemetry
    auto& telemetry = Telemetry::TelemetryManager::GetInstance();
    std::cout << "Initializing telemetry with verbose=" << (verbose ? "true" : "false");
    if (teardown_time >= 0) {
        std::cout << " and teardown_time=" << teardown_time << "s";
    }
    std::cout << std::endl;

    bool init_success;
    if (teardown_time >= 0) {
        init_success = telemetry.Initialize(verbose, teardown_time);
    } else {
        init_success = telemetry.Initialize(verbose);
    }

    if (init_success) {
        std::cout << "Telemetry initialized successfully!" << std::endl;
        telemetry.ProcessJsonFile(filepath);
        std::cout << "Processed telemetry JSON file: " << filepath << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        telemetry.Shutdown();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Telemetry shutdown successfully! [" << duration.count() << " ms]" << std::endl;
        // Delete the JSON file
        if (std::remove(filepath.c_str()) != 0) {
            std::cerr << "Warning: Failed to delete JSON file: " << filepath << std::endl;
        }
    } else {
        std::cerr << "Error: Failed to initialize telemetry." << std::endl;
        return 1;
    }

    return 0;
}
