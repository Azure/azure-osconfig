# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project(telemetryexetests)

cmake_minimum_required(VERSION 3.2.0)
add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result;-Wno-unused-parameter")

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

# Build the args.cpp as a library for testing
add_library(telemetrybin_args_lib
    ../args.cpp
)

target_include_directories(telemetrybin_args_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(telemetrybin_args_lib
    PUBLIC
        telemetrylib
        logging
)

# Build the test executable
add_executable(telemetrybintests
    TelemetryBinTests.cpp
)

target_link_libraries(telemetrybintests
    telemetrybin_args_lib
    logging
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
)

gtest_discover_tests(telemetrybintests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
