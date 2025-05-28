# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(complianceengine)

add_library(complianceengine SHARED ComplianceEngineModule.c)
target_link_libraries(complianceengine
    PRIVATE
        commonutils
        logging
    PUBLIC
        complianceenginelib
)
target_include_directories(complianceengine
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(complianceengine
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS complianceengine DESTINATION ${MODULES_INSTALL_DIR})
