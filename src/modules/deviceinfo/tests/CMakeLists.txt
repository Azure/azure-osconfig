# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deviceinfotests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(deviceinfotests DeviceInfoTests.cpp)
target_link_libraries(deviceinfotests gtest gtest_main pthread deviceinfolib logging commonutils)

gtest_discover_tests(deviceinfotests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
