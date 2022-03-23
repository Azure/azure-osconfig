# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(packagemanagerconfigurationlib)

add_library(packagemanagerconfigurationlib STATIC PackageManagerConfigurationBase.cpp PackageManagerConfiguration.cpp)
target_link_libraries(packagemanagerconfigurationlib PRIVATE logging commonutils)

target_include_directories(packagemanagerconfigurationlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(packagemanagerconfigurationlib PUBLIC -fsigned-char)

set_target_properties(packagemanagerconfigurationlib PROPERTIES POSITION_INDEPENDENT_CODE ON)