# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(pmctests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(pmctests PmcTests.cpp)
target_link_libraries(pmctests GTest::gtest GTest::gtest_main pthread pmclib commonutils logging)

gtest_discover_tests(pmctests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
