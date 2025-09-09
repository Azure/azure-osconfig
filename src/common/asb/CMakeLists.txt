# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(asb)

add_library(asb STATIC
    Asb.c)

target_compile_options(asb PRIVATE -Wno-psabi)

target_include_directories(asb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PLATFORM_INC_DIR})

target_link_libraries(asb
    PUBLIC
        $<$<BOOL:${BUILD_TELEMETRY}>:$<TARGET_NAME_IF_EXISTS:telemetryjsonlib>>
        $<$<NOT:$<BOOL:${BUILD_TELEMETRY}>>:$<TARGET_NAME_IF_EXISTS:telemetrystublib>>
    PRIVATE
        logging
        commonutils
        parsonlib
)
