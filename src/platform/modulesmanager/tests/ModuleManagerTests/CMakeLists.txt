# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanagertests)

cmake_minimum_required(VERSION 3.2.0)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/ModulesManagerTests.h
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
    src/CommonTests.cpp
    src/MockManagementModule.cpp
    src/MockModulesManager.cpp)
target_link_libraries(modulesmanagermocks gtest gtest_main gmock gmock_main pthread modulesmanager commonutils)
target_include_directories(modulesmanagermocks PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../inc/")

add_executable(managementmoduletests src/ManagementModuleTests.cpp)
target_link_libraries(managementmoduletests modulesmanagermocks)
gtest_discover_tests(managementmoduletests)

add_executable(modulesmanagertests src/ModulesManagerTests.cpp)
target_link_libraries(modulesmanagertests modulesmanagermocks)
gtest_discover_tests(modulesmanagertests)

add_executable(mpitests src/MpiTests.cpp)
target_link_libraries(mpitests modulesmanagermocks)
gtest_discover_tests(mpitests)