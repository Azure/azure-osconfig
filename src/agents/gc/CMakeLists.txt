# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(OSConfig_PrototypeResource)

add_library(OSConfig_PrototypeResource 
    SHARED 
        module.c
        schema.c
        OSConfig_PrototypeResource.c)

target_include_directories(OSConfig_PrototypeResource 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(OSConfig_PrototypeResource
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)