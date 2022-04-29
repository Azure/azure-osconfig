# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(PLATFORM_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../modules/inc)
set(MODULESMANAGER_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modulesmanager/inc)
set(ORCHESTRATOR_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/orchestrator/inc)

add_subdirectory(modulesmanager)
add_subdirectory(orchestrator)