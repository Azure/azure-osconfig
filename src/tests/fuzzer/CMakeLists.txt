# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

# Set the project name and version
project(kompli-fuzzer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer")

add_executable(kompli-fuzzer
    target.cpp)
target_link_libraries(kompli-fuzzer PRIVATE
    complianceenginelib
    commonutils
)
