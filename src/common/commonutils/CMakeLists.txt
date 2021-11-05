# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
project(commonutils)

find_package(RapidJSON REQUIRED)
add_library(commonutils STATIC CommonUtils.c CommonUtils.cpp)
target_compile_options(commonutils PRIVATE -Wno-psabi)
target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAPIDJSON_INCLUDE_DIRS}
)
target_link_libraries(commonutils PRIVATE logging)