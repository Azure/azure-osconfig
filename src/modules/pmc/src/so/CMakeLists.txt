# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(pmc)

add_library(pmc SHARED PmcModule.cpp)
target_link_libraries(pmc
    PRIVATE
        commonutils
        logging
    PUBLIC
        pmclib
)
target_include_directories(pmc
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(pmc
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)

install(TARGETS pmc DESTINATION ${MODULES_INSTALL_DIR})
