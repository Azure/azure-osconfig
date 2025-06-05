# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(networking)

add_library(networking SHARED NetworkingModule.cpp)
target_link_libraries(networking
    PRIVATE
        commonutils
        logging
    PUBLIC
        networkinglib
)
target_include_directories(networking
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(networking
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS networking DESTINATION ${MODULES_INSTALL_DIR})
