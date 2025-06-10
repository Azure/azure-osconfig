# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deliveryoptimizationtests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(deliveryoptimizationtests DeliveryOptimizationTests.cpp)
target_link_libraries(deliveryoptimizationtests gtest gtest_main pthread deliveryoptimizationlib logging commonutils)

gtest_discover_tests(deliveryoptimizationtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
