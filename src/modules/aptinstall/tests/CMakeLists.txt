# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(aptinstalltests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(aptinstalltests AptInstallTests.cpp)
target_link_libraries(aptinstalltests gtest gtest_main pthread aptinstalllib commonutils logging)

gtest_discover_tests(aptinstalltests)