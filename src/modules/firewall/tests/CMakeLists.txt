# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunnertests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(firewalltests FirewallTests.cpp)
target_link_libraries(firewalltests gtest gtest_main pthread firewalllib commonutils logging)

gtest_discover_tests(firewalltests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
