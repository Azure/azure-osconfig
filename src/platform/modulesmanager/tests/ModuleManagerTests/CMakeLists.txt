# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanagertests)

cmake_minimum_required(VERSION 3.2.0)

include(CTest)
find_package(GTest REQUIRED)

if(NOT(BUILD_MODULES))
    message(FATAL_ERROR "ModulesManagerTests requires modules to be built. Enable 'BUILD_MODULES' option.")
endif()
set(MODULE_TEST_PATH ${TEST_MODULE_STAGING_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModulesManagerTests.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/ModulesManagerTests.h
    @ONLY)

add_executable(modulesmanagertests src/ModulesManagerTests.cpp src/MpiTests.cpp src/CommonTests.cpp)
target_link_libraries(modulesmanagertests gtest gtest_main gmock gmock_main pthread modulesmanager commonutils)
target_include_directories(modulesmanagertests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../inc/")

gtest_discover_tests(modulesmanagertests)