# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(firewalllib)

add_library(firewalllib STATIC Firewall.cpp)
set_property(TARGET firewalllib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(firewalllib PRIVATE logging commonutils)
target_include_directories(firewalllib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
