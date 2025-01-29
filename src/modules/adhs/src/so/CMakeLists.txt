# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(adhs)

add_library(adhs SHARED AdhsModule.c)
target_link_libraries(adhs
    PRIVATE
        commonutils
        logging
    PUBLIC
        adhslib
)
target_include_directories(adhs
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(adhs
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS adhs DESTINATION ${MODULES_INSTALL_DIR})
