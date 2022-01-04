# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(aptinstalllib)

add_library(aptinstalllib STATIC AptInstall.cpp)
target_link_libraries(aptinstalllib PRIVATE logging commonutils)

target_include_directories(aptinstalllib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(aptinstalllib PUBLIC -fsigned-char)

set_target_properties(aptinstalllib PROPERTIES POSITION_INDEPENDENT_CODE ON)