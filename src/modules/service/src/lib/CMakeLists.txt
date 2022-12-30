# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(servicelib)

add_library(servicelib STATIC Service.c)
set_property(TARGET servicelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(servicelib PRIVATE logging commonutils parsonlib)
target_include_directories(servicelib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})