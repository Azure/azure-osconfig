# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(hostnamelib)

add_library(hostnamelib STATIC HostName.cpp HostNameBase.cpp)
set_property(TARGET hostnamelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(hostnamelib PRIVATE logging commonutils)
target_include_directories(hostnamelib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})