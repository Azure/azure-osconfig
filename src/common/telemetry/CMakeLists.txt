# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetry)
add_library(telemetry STATIC telemetry_wrapper.cpp)
target_compile_options(telemetry PRIVATE -Wno-psabi)
target_link_libraries(telemetry PRIVATE ${OPENTELEMETRY_CPP_LIBRARIES})
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
