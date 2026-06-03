# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project(parsonlib)

add_library(parsonlib STATIC parson.c)
target_compile_options(parsonlib PRIVATE -Wno-psabi)
target_include_directories(parsonlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
