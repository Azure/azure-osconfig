# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(commandrunnerlib)
add_library(commandrunnerlib STATIC CommandRunner.cpp)
target_link_libraries(commandrunnerlib PRIVATE logging commonutils)
target_include_directories(commandrunnerlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(commandrunnerlib PUBLIC -fsigned-char)
set_target_properties(commandrunnerlib PROPERTIES POSITION_INDEPENDENT_CODE ON)