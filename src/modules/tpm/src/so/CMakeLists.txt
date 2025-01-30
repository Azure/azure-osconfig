# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(tpm)

add_library(tpm SHARED TpmModule.cpp)
target_link_libraries(tpm
    PRIVATE
        commonutils
        logging
    PUBLIC
        tpmlib
)
target_include_directories(tpm
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(tpm
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS tpm DESTINATION ${MODULES_INSTALL_DIR})
