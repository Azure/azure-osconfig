# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(configurationtests)

include(CTest)
find_package(GTest CONFIG REQUIRED)

add_executable(configurationtests ConfigurationTests.cpp)
target_link_libraries(configurationtests GTest::gtest GTest::gtest_main pthread configurationlib logging commonutils)

gtest_discover_tests(configurationtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
