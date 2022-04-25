# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(settingstests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(settingstests
    ConfigFileUtilsUT.cpp
    SettingsTests.cpp
    ScopeGuardUT.cpp)

target_link_libraries(settingstests
    gtest
    gtest_main
    gmock
    gmock_main
    pthread
    settingslib
    settings
    logging
    configfileutils
    commonutils)

gtest_discover_tests(settingstests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})