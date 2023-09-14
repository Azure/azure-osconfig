# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(LinuxOsConfigResource)

add_library(LinuxOsConfigResource 
    SHARED 
        module.c
        schema.c
        LinuxOsConfigResource.c)

target_include_directories(LinuxOsConfigResource 
    PUBLIC
        ${PLATFORM_INC_DIR})

target_link_libraries(LinuxOsConfigResource
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)

# Create the ZIP package 

add_custom_target(stage_create_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxOsConfigPolicy.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxOsConfigPolicy.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LinuxOsConfigResource> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/LinuxOsConfigResource/libLinuxOsConfigResource.so
    DEPENDS LinuxOsConfigResource)

add_custom_target(create_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/LinuxOsConfigPolicy.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/LinuxOsConfigPolicy.zip" --format=zip .
    DEPENDS stage_create_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)