# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(OSConfig_PrototypeResource)

set(LIBRARY_OUTPUT_DIRECTORY, ${CMAKE_BINARY_DIR}/Modules/DscNativeResources/OSConfig_PrototypeResource)

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

add_custom_target(create_zip COMMAND
    ${CMAKE_COMMAND} -E tar "cfv" "OSConfig_Proto_Policy.zip" --format=zip
       "${CMAKE_CURRENT_SOURCE_DIR}/OSConfig_Proto_Policy.metaconfig.json"
       "${CMAKE_CURRENT_SOURCE_DIR}/OSConfig_Proto_Policy.mof"
       "${CMAKE_BINARY_DIR}/Modules/DscNativeResources/OSConfig_PrototypeResource")