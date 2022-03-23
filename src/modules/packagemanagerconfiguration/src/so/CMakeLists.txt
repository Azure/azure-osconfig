# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

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

install(TARGETS packagemanagerconfiguration DESTINATION ${MODULES_INSTALL_DIR})