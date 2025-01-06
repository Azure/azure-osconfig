# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commonutils)

add_library(commonutils STATIC 
    CommandUtils.c
    ConfigUtils.c
    DaemonUtils.c
    DeviceInfoUtils.c
    FileUtils.c
    MountUtils.c
    OtherUtils.c
    PackageUtils.c
    PassUtils.c
    ProxyUtils.c
    SocketUtils.c
    SshUtils.c
    UrlUtils.c
    UserUtils.c
    PerfUtils.c)

target_include_directories(commonutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAPIDJSON_INCLUDE_DIRS}
)

target_link_libraries(commonutils PRIVATE 
    logging 
    parsonlib
    m)