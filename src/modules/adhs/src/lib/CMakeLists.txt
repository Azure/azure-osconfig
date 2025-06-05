# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(adhslib)

add_library(adhslib STATIC Adhs.c)
set_property(TARGET adhslib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(adhslib PRIVATE logging commonutils)
target_include_directories(adhslib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
