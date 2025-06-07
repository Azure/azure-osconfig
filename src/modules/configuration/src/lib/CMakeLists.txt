# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(configurationlib)

add_library(configurationlib STATIC Configuration.c)
set_property(TARGET configurationlib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(configurationlib PRIVATE logging commonutils parsonlib)
target_include_directories(configurationlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
