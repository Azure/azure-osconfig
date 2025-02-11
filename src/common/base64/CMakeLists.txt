# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(base64)

add_library(base64 STATIC base64.c)
target_compile_options(base64 PRIVATE -Wno-psabi)
target_include_directories(base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
