# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(pmclib)

add_library(pmclib STATIC ExecutionState.cpp PmcBase.cpp Pmc.cpp)
target_link_libraries(pmclib PRIVATE logging commonutils)

target_include_directories(pmclib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(pmclib PUBLIC -fsigned-char)

set_target_properties(pmclib PROPERTIES POSITION_INDEPENDENT_CODE ON)
