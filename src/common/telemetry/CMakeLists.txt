# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.16)
project(telemetry)

if(BUILD_EXTERNAL)
    enable_testing()
    if(NOT(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5"))
        add_subdirectory(lib)
        add_subdirectory(bin)
    endif()
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
