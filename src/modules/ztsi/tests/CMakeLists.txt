# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ztsitests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(ztsitests ZtsiTests.cpp)
target_link_libraries(ztsitests gtest gtest_main pthread ztsilib commonutils logging)

gtest_discover_tests(ztsitests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
