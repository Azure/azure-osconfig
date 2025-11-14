# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetryexetests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

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
    gtest
    gtest_main
    gmock
    gmock_main
    pthread
)

gtest_discover_tests(telemetrybintests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
