# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrytests)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(telemetrytests TelemetryTests.cpp)

target_link_libraries(telemetrytests GTest::gtest GTest::gtest_main pthread telemetrylib)
gtest_discover_tests(telemetrytests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

ExternalProject_Get_Property(1dscpplib SOURCE_DIR)

add_executable(telemetryencoderparitytests
    TelemetryEncoderParityTests.cpp
    ../../TelemetryEncoder.c
)
add_dependencies(telemetryencoderparitytests 1dscpplib)

target_include_directories(telemetryencoderparitytests
    PRIVATE
        ../..
)
target_include_directories(telemetryencoderparitytests SYSTEM
    PRIVATE
        ${SOURCE_DIR}/lib
        ${SOURCE_DIR}/lib/include
        ${SOURCE_DIR}/lib/include/public
        ${SOURCE_DIR}/lib/include/mat
        ${SOURCE_DIR}/lib/pal
        ${SOURCE_DIR}/lib/utils
)
target_link_libraries(telemetryencoderparitytests
    GTest::gtest
    GTest::gtest_main
    pthread
    logging
)
gtest_discover_tests(telemetryencoderparitytests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
