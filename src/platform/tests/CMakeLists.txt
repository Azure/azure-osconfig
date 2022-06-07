# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanagertests)

set(modulesmanagertests_files
    ../Log.c
    ../ManagementModule.cpp
    ../ModulesManager.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModulesManagerTests.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/ModulesManagerTests.h
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

add_library(modulesmanagermocks
    CommonTests.cpp
    MockManagementModule.cpp
    MockModulesManager.cpp)
target_link_libraries(modulesmanagermocks gtest gtest_main gmock gmock_main pthread logging commonutils parsonlib ${CMAKE_DL_LIBS})
target_include_directories(modulesmanagermocks PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${MODULES_INC_DIR}" "${PLATFORM_INC_DIR}")

add_executable(managementmoduletests ${modulesmanagertests_files} ManagementModuleTests.cpp)
target_link_libraries(managementmoduletests modulesmanagermocks)
gtest_discover_tests(managementmoduletests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

add_executable(modulesmanagertests ${modulesmanagertests_files} ModulesManagerTests.cpp)
target_link_libraries(modulesmanagertests modulesmanagermocks)
gtest_discover_tests(modulesmanagertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

add_executable(mpitests ${modulesmanagertests_files} MpiTests.cpp)
target_link_libraries(mpitests modulesmanagermocks)
gtest_discover_tests(mpitests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

add_executable(mpiservertests ${modulesmanagertests_files} MpiServerTests.cpp)
target_link_libraries(mpiservertests modulesmanagermocks)
gtest_discover_tests(mpiservertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})