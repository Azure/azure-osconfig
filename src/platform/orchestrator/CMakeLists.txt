# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(orchestrator)

add_library(orchestrator STATIC ./src/MpiServer.cpp)

target_include_directories(orchestrator PUBLIC
    PUBLIC
        ${PLATFORM_INC_DIR}
        ${MODULES_INC_DIR}
        ${MODULESMANAGER_INC_DIR}
        ${ORCHESTRATOR_INC_DIR})

target_link_libraries(orchestrator pthread logging commonutils parsonlib)
