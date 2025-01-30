# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SSH Posture Control
project(OsConfigResourceSsh)

add_library(OsConfigResourceSsh
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c)

target_link_libraries(OsConfigResourceSsh
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        asb)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceSsh PROPERTIES OUTPUT_NAME libOsConfigResource)

# Create the policy artifacts ZIP package for SSH Posture Control

add_custom_target(stage_create_ssh_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceSsh> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResourceSsh)

add_custom_target(create_ssh_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/LinuxSshServerSecurityBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/LinuxSshServerSecurityBaseline.zip" --format=zip .
    DEPENDS stage_create_ssh_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)
