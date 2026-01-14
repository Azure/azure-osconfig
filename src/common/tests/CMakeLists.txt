# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    SshUtilsUT.cpp
    TelemetryUT.cpp
    Helper.c
)

target_link_libraries(commontests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
    logging
    commonutils
    asb
    parsonlib
    telemetry
)

gtest_discover_tests(commontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
