# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)
project(hostname)

add_library(hostname SHARED HostNameModule.cpp)
target_link_libraries(hostname
    PRIVATE
        commonutils
        logging
    PUBLIC
        hostnamelib
)
target_include_directories(hostname
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(hostname
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS hostname DESTINATION ${MODULES_INSTALL_DIR})