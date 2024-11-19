# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(securitybaselinetests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(securitybaselinetests SecurityBaselineTests.cpp)
target_link_libraries(securitybaselinetests gtest gtest_main pthread securitybaselinelib logging commonutils)

gtest_discover_tests(securitybaselinetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
