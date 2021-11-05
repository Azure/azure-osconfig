# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunnertests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(commandrunnertests CommandRunnerTests.cpp)
target_link_libraries(commandrunnertests gtest gtest_main gmock gmock_main pthread commandrunnerlib commonutils logging)

gtest_discover_tests(commandrunnertests)