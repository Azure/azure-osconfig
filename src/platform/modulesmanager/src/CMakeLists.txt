# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulesmanager)
set(MODULE_MANAGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../inc/)
set(MODULES_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/inc)

add_library(modulesmanager STATIC ModulesManager.cpp ManagementModule.cpp)
target_link_libraries(modulesmanager
    PUBLIC
        ${CMAKE_DL_LIBS}
        logging
        commonutils
)
target_include_directories(modulesmanager
    PUBLIC
        ${MODULE_MANAGER_INCLUDE_DIR}
        ${ORCHESTRATOR_INC_DIR}
        ${MODULES_INCLUDE_DIR}
        ${PLATFORM_INC_DIR}
)