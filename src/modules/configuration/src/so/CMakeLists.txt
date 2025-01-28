# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(configuration)

add_library(configuration SHARED ConfigurationModule.c)
target_link_libraries(configuration
    PRIVATE
        commonutils
        logging
    PUBLIC
        configurationlib
)
target_include_directories(configuration
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(configuration
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS configuration DESTINATION ${MODULES_INSTALL_DIR})
