# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

if (BUILD_TESTS)
    add_compile_options(-D TEST_CODE)
    add_subdirectory(tests)
endif()

add_subdirectory(logging)
add_subdirectory(commonutils)