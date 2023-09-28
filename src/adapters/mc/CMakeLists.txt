# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(OsConfigResource)

add_library(OsConfigResource 
    SHARED 
        module.c
        schema.c
        OsConfigResource.c)

target_include_directories(OsConfigResource 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(OsConfigResource
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)

# Create the ZIP package 

add_custom_target(stage_create_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResource> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResource)

add_custom_target(create_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/OsConfigPolicy.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/OsConfigPolicy.zip" --format=zip .
    DEPENDS stage_create_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)