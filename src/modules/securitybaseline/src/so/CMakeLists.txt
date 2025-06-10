# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(securitybaseline)

add_library(securitybaseline SHARED SecurityBaselineModule.c)
target_link_libraries(securitybaseline
    PRIVATE
        commonutils
        logging
        asb
    PUBLIC
        securitybaselinelib
)
target_include_directories(securitybaseline
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(securitybaseline
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS securitybaseline DESTINATION ${MODULES_INSTALL_DIR})
