# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deviceinfo)

add_library(deviceinfo SHARED DeviceInfoModule.c)
target_link_libraries(deviceinfo
    PRIVATE
        commonutils
        logging
    PUBLIC
        deviceinfolib
)
target_include_directories(deviceinfo
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(deviceinfo
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS deviceinfo DESTINATION ${MODULES_INSTALL_DIR})
