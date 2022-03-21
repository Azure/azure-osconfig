# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)

project(osinfolib)

add_library(osinfolib STATIC OsInfo.c)
set_property(TARGET osinfolib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(osinfolib PRIVATE logging commonutils)
target_include_directories(osinfolib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})