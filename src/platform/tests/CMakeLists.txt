# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanagertests)

set(platform_files
    ../MmiClient.c
    ../ModulesManager.c
    ../MpiServer.c)

set(CMAKE_CXX_STANDARD 14)

include(CTest)
find_package(GTest REQUIRED)

set(TEST_MODULES_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_subdirectory(modules)

set(TEST_CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR}/osconfig CACHE FILEPATH "Directory used for test configuration files")
set(OSCONFIG_JSON_INVALID ${TEST_CONFIG_DIR}/osconfig-invalid.json)
set(OSCONFIG_JSON_NONE_REPORTED ${TEST_CONFIG_DIR}/osconfig-none-reported.json)
set(OSCONFIG_JSON_SINGLE_REPORTED ${TEST_CONFIG_DIR}/osconfig-single-reported.json)
set(OSCONFIG_JSON_MULTIPLE_REPORTED ${TEST_CONFIG_DIR}/osconfig-multiple-reported.json)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestModules.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/TestModules.h
    @ONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osconfig-invalid.json
    ${OSCONFIG_JSON_INVALID}
    COPYONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osconfig-none-reported.json
    ${OSCONFIG_JSON_NONE_REPORTED}
    COPYONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osconfig-single-reported.json
    ${OSCONFIG_JSON_SINGLE_REPORTED}
    COPYONLY)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osconfig-multiple-reported.json
    ${OSCONFIG_JSON_MULTIPLE_REPORTED}
    COPYONLY)

add_executable(mpitests MpiTests.cpp ${platform_files})
target_link_libraries(mpitests gtest gtest_main pthread logging commonutils parsonlib ${CMAKE_DL_LIBS})
target_include_directories(mpitests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MODULES_INC_DIR} ${PLATFORM_INC_DIR})
gtest_discover_tests(mpitests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

add_executable(mpiservertests MpiServerTests.cpp ${platform_files})
target_link_libraries(mpiservertests gtest gtest_main pthread logging commonutils parsonlib ${CMAKE_DL_LIBS})
target_include_directories(mpiservertests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MODULES_INC_DIR} ${PLATFORM_INC_DIR})
gtest_discover_tests(mpiservertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})