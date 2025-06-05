# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancetests)

include(CTest)
find_package(GTest REQUIRED)

# Temporary solution, until we stabilize the list of procedures.
FILE(GLOB PROCEDURES procedures/*.cpp)

add_executable(complianceenginetests
    Base64Test.cpp
    CommonContextTest.cpp
    ComplianceEngineTest.cpp
    EngineTest.cpp
    EvaluatorTest.cpp
    NetworkToolsTest.cpp
    OptionalTest.cpp
    RegexFallbackTest.cpp
    RegexTest.cpp
    ResultTest.cpp
    Base64Test.cpp
    ${PROCEDURES}
)
# for ProcedureMap.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(complianceenginetests
    GTest::gtest
    GTest::gtest_main
    gmock
    gmock_main
    pthread
    complianceenginelib
    logging
    commonutils
    parsonlib
)

gtest_discover_tests(complianceenginetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
