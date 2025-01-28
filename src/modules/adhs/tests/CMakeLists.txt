# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(adhstests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(adhstests AdhsTests.cpp)
target_link_libraries(adhstests gtest gtest_main pthread adhslib logging commonutils)

gtest_discover_tests(adhstests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
