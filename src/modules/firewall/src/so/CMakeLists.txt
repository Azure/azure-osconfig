# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(firewall)

add_library(firewall SHARED FirewallModule.cpp)
target_link_libraries(firewall
    PRIVATE
        commonutils
        logging
    PUBLIC
        firewalllib
)
target_include_directories(firewall
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(firewall
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS firewall DESTINATION ${MODULES_INSTALL_DIR})
