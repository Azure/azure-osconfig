# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(compliance-engine-assessor-tests
    CliOptionsTest.cpp
    InputSecurityTest.cpp
    MofTest.cpp
)

target_include_directories(compliance-engine-assessor-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
)

target_link_libraries(compliance-engine-assessor-tests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    pthread
    compliance-engine-assessor-lib
)

gtest_discover_tests(compliance-engine-assessor-tests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
