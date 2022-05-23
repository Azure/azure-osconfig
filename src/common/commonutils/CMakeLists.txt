# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commonutils)

find_package(RapidJSON REQUIRED)

add_library(commonutils STATIC 
    CommandUtils.c
    DaemonUtils.c
    DeviceInfoUtils.c
    FileUtils.c
    OtherUtils.c
    ProxyUtils.c
    SocketUtils.c
    UrlUtils.c
    CommonUtils.cpp)

target_compile_options(commonutils PRIVATE -Wno-psabi)

target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAPIDJSON_INCLUDE_DIRS}
)

target_link_libraries(commonutils PRIVATE logging)