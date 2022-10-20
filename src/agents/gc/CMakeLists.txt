# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(libOSConfig_PrototypeResource)

add_library(libOSConfig_PrototypeResource 
    SHARED 
        module.c
        schema.c
        OSConfig_PrototypeResource.c)

target_include_directories(libOSConfig_PrototypeResource 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(libOSConfig_PrototypeResource
    PRIVATE
        commonutils
        logging
        mpiclient)