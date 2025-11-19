# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(adhstests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(adhstests AdhsTests.cpp)
target_link_libraries(adhstests GTest::gtest GTest::gtest_main pthread adhslib logging commonutils)

gtest_discover_tests(adhstests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
