# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(networkingtests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(networkingtests NetworkingTests.cpp)
target_link_libraries(networkingtests gtest gtest_main pthread networkinglib networking logging commonutils)

gtest_discover_tests(networkingtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
