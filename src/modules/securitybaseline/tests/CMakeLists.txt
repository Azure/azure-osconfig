# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(securitybaselinetests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(securitybaselinetests SecurityBaselineTests.cpp)
target_link_libraries(securitybaselinetests GTest::gtest GTest::gtest_main pthread securitybaselinelib logging commonutils)

gtest_discover_tests(securitybaselinetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
