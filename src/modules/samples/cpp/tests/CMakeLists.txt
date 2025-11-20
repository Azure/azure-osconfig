# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(sampletests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(sampletests SampleTests.cpp)
target_link_libraries(sampletests GTest::gtest GTest::gtest_main pthread cppsamplelib commonutils logging)

gtest_discover_tests(sampletests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
