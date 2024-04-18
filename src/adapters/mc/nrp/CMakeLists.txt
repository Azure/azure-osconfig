# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(osconfignrp)

add_library(osconfignrp
    STATIC 
        module.c
        schema.c
        OsConfigResource.c)

target_include_directories(osconfignrp 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(osconfignrp
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)