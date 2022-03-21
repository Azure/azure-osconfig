# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)

project(osinfo)

add_library(osinfo SHARED OsInfoModule.c)
target_link_libraries(osinfo
    PRIVATE
        commonutils
        logging
    PUBLIC
        osinfolib
)
target_include_directories(osinfo
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(osinfo
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS osinfo DESTINATION ${MODULES_INSTALL_DIR})