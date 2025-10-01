# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.16)
project(telemetry)

if(BUILD_EXTERNAL)
    if(NOT(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5"))
        add_subdirectory(lib)
        add_subdirectory(exe)
    endif()
endif()
