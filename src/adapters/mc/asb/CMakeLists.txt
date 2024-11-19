# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Azure Security Baseline for Linux
project(OsConfigResourceAsb)

add_library(OsConfigResourceAsb
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c)

target_link_libraries(OsConfigResourceAsb
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        asb)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceAsb PROPERTIES OUTPUT_NAME libOsConfigResource)

# Create the policy artifacts ZIP package for the Azure Security Baseline for Linux

add_custom_target(stage_create_asb_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceAsb> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResourceAsb)

add_custom_target(create_asb_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip" --format=zip .
    DEPENDS stage_create_asb_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)
