# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(cppsample)

add_library(sample SHARED SampleModule.cpp)
target_link_libraries(sample
    PRIVATE
        commonutils
        logging
    PUBLIC
        cppsamplelib
)
target_include_directories(sample
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(sample
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)

# To install the shared library, uncomment the following line:
# install(TARGETS cppsample DESTINATION ${MODULES_INSTALL_DIR})