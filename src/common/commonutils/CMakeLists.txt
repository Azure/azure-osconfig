# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project(commonutils)

add_library(commonutils STATIC
    CommandUtils.c
    ConfigUtils.c
    DaemonUtils.c
    FileUtils.c
    SocketUtils.c
    StringUtils.c)

target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(commonutils
    PUBLIC
        logging
        telemetry
        parsonlib
    PRIVATE
        m
)
