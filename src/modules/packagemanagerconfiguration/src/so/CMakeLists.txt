# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 17)
project(packagemanagerconfiguration)

add_library(packagemanagerconfiguration SHARED PackageManagerConfigurationModule.cpp)
target_link_libraries(packagemanagerconfiguration
    PRIVATE
        commonutils
        logging
    PUBLIC
        packagemanagerconfigurationlib
)
target_include_directories(packagemanagerconfiguration
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(packagemanagerconfiguration
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)

# To install the shared library, uncomment the following line:
install(TARGETS packagemanagerconfiguration DESTINATION ${MODULES_INSTALL_DIR})