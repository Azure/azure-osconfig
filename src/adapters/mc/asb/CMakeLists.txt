# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SO binary name must match the class name in MOF with a "lib" prefix added
project(OsConfigResource)

add_library(OsConfigResourceAsb 
    SHARED 
        ../nrp/module.c
        ../nrp/schema.c
        ../nrp/OsConfigResource.c)

target_link_libraries(OsConfigResourceAsb
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)

set_target_properties(OsConfigResourceAsb OUTPUT_NAME libOsConfigResource)

# Create the policy artifacts ZIP package for the Azure Security Baseline for Linux

add_custom_target(stage_create_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResource> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResource)

add_custom_target(create_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/OsConfigPolicy.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/LinuxSecurityBaselinePolicy.zip" --format=zip .
    DEPENDS stage_create_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)