# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ztsilib)
add_library(ztsilib STATIC Ztsi.cpp)
target_link_libraries(ztsilib PRIVATE logging commonutils)
target_include_directories(ztsilib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(ztsilib PUBLIC -fsigned-char)
set_target_properties(ztsilib PROPERTIES POSITION_INDEPENDENT_CODE ON)
