# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

# Set the project name and version
project(osconfig-fuzzer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer")

add_executable(osconfig-fuzzer
    target.cpp)
target_link_libraries(osconfig-fuzzer PRIVATE
    complianceenginelib
    commonutils
)
