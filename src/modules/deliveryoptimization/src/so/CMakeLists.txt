# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deliveryoptimization)

add_library(deliveryoptimization SHARED DeliveryOptimizationModule.c)
target_link_libraries(deliveryoptimization
    PRIVATE
        commonutils
        logging
    PUBLIC
        deliveryoptimizationlib
)
target_include_directories(deliveryoptimization
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(deliveryoptimization
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS deliveryoptimization DESTINATION ${MODULES_INSTALL_DIR})
