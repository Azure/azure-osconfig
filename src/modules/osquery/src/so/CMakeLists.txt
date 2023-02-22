# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(osquery)

add_library(osquery SHARED OSQueryModule.cpp)
target_link_libraries(osquery
    PRIVATE
        commonutils
        logging
    PUBLIC
        osquerylib
)
target_include_directories(osquery
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(osquery
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)

# To install the shared library, uncomment the following line:
# install(TARGETS cppsample DESTINATION ${MODULES_INSTALL_DIR})