# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(parson-common)

add_library(parson-common STATIC parson.c)
target_compile_options(parson-common PRIVATE -Wno-psabi)
target_include_directories(parson-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})