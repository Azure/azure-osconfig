# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deviceinfolib)

add_library(deviceinfolib STATIC DeviceInfo.c)
set_property(TARGET deviceinfolib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(deviceinfolib PRIVATE logging commonutils)
target_include_directories(deviceinfolib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
