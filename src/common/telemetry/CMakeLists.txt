# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetry)

# Add public header target only
add_library(telemetry INTERFACE)
target_include_directories(telemetry INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(telemetry INTERFACE dl)
