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
        $<$<BOOL:${ENABLE_TELEMETRY}>:telemetryjsonlib>
        $<$<NOT:$<BOOL:${ENABLE_TELEMETRY}>>:telemetrystublib>
    PRIVATE
        dl
        logging
        commonutils
        parsonlib
)
