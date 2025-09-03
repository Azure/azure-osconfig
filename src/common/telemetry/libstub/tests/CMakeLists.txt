# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrystubtests)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(CTest)
find_package(GTest REQUIRED)

add_executable(telemetrystubtests TelemetryStubTests.cpp)

target_link_libraries(telemetrystubtests gtest gtest_main pthread telemetrystublib)

gtest_discover_tests(telemetrystubtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
