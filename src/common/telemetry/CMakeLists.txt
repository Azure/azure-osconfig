# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.16)
project(telemetry-bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
    message(FATAL_ERROR "GCC version must be 5.5 or greater to enable telemetry")
endif()

add_subdirectory(lib)
add_subdirectory(exe)
