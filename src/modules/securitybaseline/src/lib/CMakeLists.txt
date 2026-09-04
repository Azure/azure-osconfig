# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(securitybaselinelib)

add_library(securitybaselinelib STATIC
    SecurityBaseline.c)

set_property(TARGET securitybaselinelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(securitybaselinelib PRIVATE
    logging
    commonutils
    parsonlib
    asb)

target_include_directories(securitybaselinelib PUBLIC
    ${MODULES_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})
