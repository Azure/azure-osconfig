# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(service)

add_library(service SHARED ServiceModule.c)
target_link_libraries(service
    PRIVATE
        commonutils
        logging
    PUBLIC
        servicelib
)
target_include_directories(service
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(service
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS service DESTINATION ${MODULES_INSTALL_DIR})