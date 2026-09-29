# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(telemetry)

if(BUILD_EXTERNAL)
    set(BUILD_TELEMETRY ON)
    include(../../cmake/Dependencies.cmake)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
        message(FATAL_ERROR "The telemetry subprocess requires GCC 5.5 or newer")
    endif()
    if(BUILD_TESTS)
        enable_testing()
    endif()
    add_subdirectory(lib)
    add_subdirectory(bin)
else()
    add_library(telemetry
        STATIC
            Telemetry.c)

    target_include_directories(telemetry
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(telemetry
        PRIVATE
            commonutils
        PUBLIC
            dl)
endif()
