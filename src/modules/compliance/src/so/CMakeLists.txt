# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliance)

add_library(compliance SHARED ComplianceModule.c)
target_link_libraries(compliance
    PRIVATE
        commonutils
        logging
        asb
    PUBLIC
        compliancelib
)
target_include_directories(compliance
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(compliance
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS compliance DESTINATION ${MODULES_INSTALL_DIR})
