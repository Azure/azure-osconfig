# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(proto_osconfig_nrp)

add_library(proto_osconfig_nrp 
    SHARED 
        module.c
        schema.c
        OSConfig_PrototypeResource.c)

target_include_directories(proto_osconfig_nrp 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(proto_osconfig_nrp
    PRIVATE
        commonutils
        logging
        mpiclient)