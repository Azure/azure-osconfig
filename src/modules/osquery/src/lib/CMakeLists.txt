# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(osquerylib)

add_library(osquerylib STATIC osquery.cpp)
target_link_libraries(osquerylib PRIVATE logging commonutils)

target_include_directories(osquerylib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(osquerylib PUBLIC -fsigned-char)

set_target_properties(osquerylib PROPERTIES POSITION_INDEPENDENT_CODE ON)