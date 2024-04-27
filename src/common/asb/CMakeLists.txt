# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(asb)

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;-Wno-tautological-compare>")

add_library(asb STATIC
    Asb.c)

target_compile_options(asb PRIVATE -Wno-psabi)

target_include_directories(asb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PLATFORM_INC_DIR})

target_link_libraries(asb PRIVATE 
    logging 
    commonutils 
    parsonlib)