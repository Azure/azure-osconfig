# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(deliveryoptimizationlib)

add_library(deliveryoptimizationlib STATIC DeliveryOptimization.c)
set_property(TARGET deliveryoptimizationlib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(deliveryoptimizationlib PRIVATE logging commonutils parsonlib)
target_include_directories(deliveryoptimizationlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
