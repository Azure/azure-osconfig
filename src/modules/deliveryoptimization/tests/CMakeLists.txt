# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deliveryoptimizationtests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(deliveryoptimizationtests DeliveryOptimizationTests.cpp)
target_link_libraries(deliveryoptimizationtests GTest::gtest GTest::gtest_main pthread deliveryoptimizationlib logging commonutils)

gtest_discover_tests(deliveryoptimizationtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
