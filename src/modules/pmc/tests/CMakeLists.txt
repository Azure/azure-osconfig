# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(pmctests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(pmctests PmcTests.cpp)
target_link_libraries(pmctests gtest gtest_main pthread pmclib commonutils logging)

gtest_discover_tests(pmctests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
