# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(networkinglib)
add_library(networkinglib STATIC Networking.cpp)
target_link_libraries(networkinglib PRIVATE logging commonutils)
target_include_directories(networkinglib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(networkinglib PUBLIC -fsigned-char)
set_target_properties(networkinglib PROPERTIES POSITION_INDEPENDENT_CODE ON)
