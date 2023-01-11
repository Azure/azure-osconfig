# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ansiblelib)

add_library(ansiblelib STATIC Ansible.c)
set_property(TARGET ansiblelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(ansiblelib PRIVATE logging commonutils parsonlib)
target_include_directories(ansiblelib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})