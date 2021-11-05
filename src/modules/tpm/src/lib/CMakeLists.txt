# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(tpmlib)
add_library(tpmlib STATIC Tpm.cpp)
target_link_libraries(tpmlib PRIVATE logging commonutils)
target_include_directories(tpmlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(tpmlib PUBLIC -fsigned-char)
set_target_properties(tpmlib PROPERTIES POSITION_INDEPENDENT_CODE ON)