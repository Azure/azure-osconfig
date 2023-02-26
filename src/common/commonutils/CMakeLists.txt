# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commonutils)

find_package(RapidJSON REQUIRED)

add_library(commonutils STATIC 
    CommandUtils.c
    ConfigUtils.c
    DaemonUtils.c
    DeviceInfoUtils.c
    FileUtils.c
    OtherUtils.c
    ProxyUtils.c
    SocketUtils.c
    UrlUtils.c
    UserUtils.c
    CommonUtils.cpp)

target_compile_options(commonutils PRIVATE -Wno-psabi -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security -Wno-nonnull -Wno-unused-result -Wunused-const-variable=2 -Wunused-macros)

target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAPIDJSON_INCLUDE_DIRS}
)

target_link_libraries(commonutils PRIVATE 
    logging 
    parsonlib)