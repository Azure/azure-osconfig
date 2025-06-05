# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(tpmtests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

add_executable(tpmtests TpmTests.cpp)
target_link_libraries(tpmtests gtest gtest_main pthread tpmlib tpm logging commonutils)

gtest_discover_tests(tpmtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
