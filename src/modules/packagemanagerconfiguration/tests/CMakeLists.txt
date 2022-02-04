# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(packagemanagerconfigurationtests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(packagemanagerconfigurationtests PackageManagerConfigurationTests.cpp)
target_link_libraries(packagemanagerconfigurationtests gtest gtest_main pthread packagemanagerconfigurationlib commonutils logging)

gtest_discover_tests(packagemanagerconfigurationtests)