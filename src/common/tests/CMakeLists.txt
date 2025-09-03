# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    SshUtilsUT.cpp
    Helper.c
)

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
