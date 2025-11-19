# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(hostnametests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(hostnametests HostNameBaseTests.cpp)
target_link_libraries(hostnametests GTest::gtest GTest::gtest_main pthread hostnamelib logging commonutils)

gtest_discover_tests(hostnametests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
