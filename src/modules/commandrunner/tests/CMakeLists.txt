# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunnertests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_library(testutils TestUtils.cpp)
target_include_directories(testutils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(testutils gtest gtest_main)

add_executable(commandrunnertests CommandRunnerTests.cpp)
target_link_libraries(commandrunnertests gtest gtest_main gmock gmock_main pthread commandrunnerlib commonutils logging testutils)
gtest_discover_tests(commandrunnertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
