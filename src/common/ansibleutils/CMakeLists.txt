# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ansibleutils)

add_library(ansibleutils STATIC 
    AnsibleUtils.c)

target_compile_options(ansibleutils PRIVATE -Wno-psabi)

target_include_directories(ansibleutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PLATFORM_INC_DIR})

target_link_libraries(ansibleutils 
    PRIVATE 
        logging 
        commonutils 
        parsonlib)