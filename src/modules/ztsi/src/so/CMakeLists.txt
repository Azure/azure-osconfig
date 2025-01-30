# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(ztsi)

add_library(ztsi SHARED ZtsiModule.cpp)
target_link_libraries(ztsi
    PRIVATE
        commonutils
        logging
    PUBLIC
        ztsilib
)
target_include_directories(ztsi
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(ztsi
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS ztsi DESTINATION ${MODULES_INSTALL_DIR})
