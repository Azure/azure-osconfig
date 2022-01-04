# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)
project(aptinstall)

add_library(aptinstall SHARED AptInstallModule.cpp)
target_link_libraries(aptinstall
    PRIVATE
        commonutils
        logging
    PUBLIC
        aptinstalllib
)
target_include_directories(aptinstall
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(aptinstall
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)

# To install the shared library, uncomment the following line:
install(TARGETS aptinstall DESTINATION ${MODULES_INSTALL_DIR})