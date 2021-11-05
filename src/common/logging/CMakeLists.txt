# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
project(logging)
add_library(logging STATIC Logging.c)
target_compile_options(logging PRIVATE -Wno-psabi)
target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})