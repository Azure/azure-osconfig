# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
)

if (CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9")
    target_sources(commontests PRIVATE
        SshUtilsUT.cpp
        Helper.c
    )
endif()

target_link_libraries(commontests
    gtest
    gtest_main
    gmock
    gmock_main
    pthread
    logging
    commonutils
    asb
    parsonlib
)

gtest_discover_tests(commontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
