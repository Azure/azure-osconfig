# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commonutils)

add_library(commonutils STATIC 
    CommandUtils.c
    ConfigUtils.c
    DaemonUtils.c
    DeviceInfoUtils.c
    FileUtils.c
    OtherUtils.c
    ProxyUtils.c
    SocketUtils.c
    SshUtils.c
    UrlUtils.c
    UserUtils.c)

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;>")

if (CMAKE_COMPILER_IS_GNUCC)
    # GCC 5.3.0 and later support -Wunused-const-variable
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3.0")
        add_compile_options("-Wunused-const-variable=2")
    endif()
endif()

target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAPIDJSON_INCLUDE_DIRS}
)

target_link_libraries(commonutils PRIVATE 
    logging 
    parsonlib
    m)