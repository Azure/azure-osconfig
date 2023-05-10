# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(hostnamelib)

add_library(hostnamelib STATIC Hostname.c)
set_property(TARGET hostnamelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(hostnamelib PRIVATE logging commonutils parsonlib)
target_include_directories(hostnamelib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})