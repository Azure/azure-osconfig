# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetryjsonlib)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

add_library(telemetryjsonlib STATIC
    telemetryjson.cpp
)

target_link_libraries(telemetryjsonlib
    PUBLIC
        parsonlib
)

target_include_directories(telemetryjsonlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
