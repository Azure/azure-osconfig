# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(settings)
add_library(settings SHARED SettingsModule.cpp)
target_compile_options(settings PRIVATE -fPIC)
target_link_libraries(settings
    PRIVATE
        commonutils
        logging
    PUBLIC
        settingslib
        configfileutils
)
target_include_directories(settings PUBLIC ${MODULES_INC_DIR})
set_target_properties(settings
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS settings DESTINATION ${MODULES_INSTALL_DIR})