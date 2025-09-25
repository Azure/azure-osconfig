# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrybin)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

add_executable(telemetry main.cpp)

target_link_libraries(telemetry PRIVATE telemetrylib)
