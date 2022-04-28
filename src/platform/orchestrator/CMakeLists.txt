# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(orchestrator)

add_executable(mpiapi
    ./src/Http.cpp
    ./src/MpiApi.cpp)

target_include_directories(mpiapi PUBLIC
    PUBLIC
        ./src
        ./inc
        ${PLATFORM_INC_DIR})

target_link_libraries(mpiapi pthread logging commonutils modulesmanager)
