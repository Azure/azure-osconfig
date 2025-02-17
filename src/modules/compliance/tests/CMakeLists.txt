# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancetests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(compliancetests
    Base64Test.cpp
    ComplianceTest.cpp
    EngineTest.cpp
    EvaluatorTest.cpp
    OptionalTest.cpp
    ResultTest.cpp
)

target_link_libraries(compliancetests gtest gtest_main pthread compliancelib logging commonutils parsonlib)

gtest_discover_tests(compliancetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
