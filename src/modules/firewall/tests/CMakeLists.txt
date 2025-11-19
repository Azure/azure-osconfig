# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunnertests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(firewalltests FirewallTests.cpp)
target_link_libraries(firewalltests GTest::gtest GTest::gtest_main pthread firewalllib commonutils logging nlohmann_json::nlohmann_json)
gtest_discover_tests(firewalltests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
