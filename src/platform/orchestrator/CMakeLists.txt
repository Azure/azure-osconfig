# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(orchestrator)

add_library(orchestrator
    ./src/Http.cpp
    ./src/MpiApi.cpp)

target_include_directories(orchestrator PUBLIC
    PUBLIC
        ./src
        ./inc
        ${PLATFORM_INC_DIR})

target_link_libraries(orchestrator pthread logging commonutils modulesmanager)
