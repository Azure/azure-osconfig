# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SSH Posture Control
project(OsConfigResourceSsh)

add_library(OsConfigResourceSsh 
    SHARED 
        ../nrp/module.c
        ../nrp/schema.c
        ../nrp/OsConfigResource.c)

target_link_libraries(OsConfigResourceSsh
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceSsh PROPERTIES OUTPUT_NAME libOsConfigResource)

# Create the policy artifacts ZIP package for SSH Posture Control

add_custom_target(stage_create_zip_ssh
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OsConfigPolicy.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceSsh> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResourceSsh)

add_custom_target(create_zip_ssh ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/OsConfigPolicy.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/LinuxSshServerSecurityBaselinePolicy.zip" --format=zip .
    DEPENDS stage_create_zip_ssh
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)