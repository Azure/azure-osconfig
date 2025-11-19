# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deviceinfotests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(deviceinfotests DeviceInfoTests.cpp)
target_link_libraries(deviceinfotests GTest::gtest GTest::gtest_main pthread deviceinfolib logging commonutils)

gtest_discover_tests(deviceinfotests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
