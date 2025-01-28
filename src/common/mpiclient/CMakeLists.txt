# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(mpiclient)

add_library(mpiclient STATIC
    MpiClient.c)

target_compile_options(mpiclient PRIVATE -Wno-psabi)

target_include_directories(mpiclient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PLATFORM_INC_DIR})

target_link_libraries(mpiclient
    PRIVATE
        logging
        commonutils
        parsonlib)
