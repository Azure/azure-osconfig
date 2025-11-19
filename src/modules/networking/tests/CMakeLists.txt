# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(networkingtests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(networkingtests NetworkingTests.cpp)
target_link_libraries(networkingtests GTest::gtest GTest::gtest_main pthread networkinglib networking logging commonutils)

gtest_discover_tests(networkingtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
