# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(compliance-engine-assessor-tests
    AssessorResultSchemaTest.cpp
    CliOptionsTest.cpp
    InputSecurityTest.cpp
    JsonFormatterTest.cpp
    JUnitRendererTest.cpp
    MofTest.cpp
)

target_include_directories(compliance-engine-assessor-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
)

# The schema conformance test reads the shipped schema from the source tree.
target_compile_definitions(compliance-engine-assessor-tests PRIVATE
    ASSESSOR_RESULT_SCHEMA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../assessor-result.schema.json"
)

target_link_libraries(compliance-engine-assessor-tests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    pthread
    compliance-engine-assessor-lib
)

gtest_discover_tests(compliance-engine-assessor-tests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
