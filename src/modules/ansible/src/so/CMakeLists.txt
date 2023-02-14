# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ansible)

add_library(ansible SHARED AnsibleModule.c)
target_link_libraries(ansible
    PRIVATE
        commonutils
        logging
    PUBLIC
        ansiblelib
)
target_include_directories(ansible
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(ansible
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS ansible DESTINATION ${MODULES_INSTALL_DIR})