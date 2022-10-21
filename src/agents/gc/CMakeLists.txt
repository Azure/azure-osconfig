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

add_custom_target(stage_create_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OSConfig_Proto_Policy.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OSConfig_Proto_Policy.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OSConfig_PrototypeResource> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OSConfig_PrototypeResource/libOSConfig_PrototypeResource.so
    DEPENDS OSConfig_PrototypeResource)

add_custom_target(create_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/OSConfig_Proto_Policy.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/OSConfig_Proto_Policy.zip" --format=zip .
    DEPENDS stage_create_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)