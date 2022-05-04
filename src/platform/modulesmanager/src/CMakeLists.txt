# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanager)

add_library(modulesmanager STATIC ModulesManager.cpp ManagementModule.cpp)
target_link_libraries(modulesmanager
    PUBLIC
        ${CMAKE_DL_LIBS}
        logging
        commonutils
        parsonlib
)
target_include_directories(modulesmanager
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PLATFORM_INC_DIR}
        ${MODULES_INC_DIR}
        ${MODULESMANAGER_INC_DIR}
        ${ORCHESTRATOR_INC_DIR}
)