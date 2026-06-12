# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

# Set the project name and version
project(osconfig-fuzzer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer")

add_executable(osconfig-fuzzer
    target.cpp
    ../../common/commonutils/UserUtils.c)
target_compile_options(osconfig-fuzzer PRIVATE -ffunction-sections -fdata-sections)
set_source_files_properties(../../common/commonutils/UserUtils.c PROPERTIES COMPILE_DEFINITIONS FUZZ_USERUTILS_ONLY)
target_link_options(osconfig-fuzzer PRIVATE -Wl,--gc-sections)
target_link_libraries(osconfig-fuzzer PRIVATE
    complianceenginelib
    commonutils
)
