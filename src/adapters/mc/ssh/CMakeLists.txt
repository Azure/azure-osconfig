# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# SSH Posture Control
project(OsConfigResourceSsh)

add_library(OsConfigResourceSsh
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c
        Baseline.c)

target_link_libraries(OsConfigResourceSsh
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        asb)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceSsh PROPERTIES OUTPUT_NAME libOsConfigResource)

# Create the policy artifacts ZIP packages for SSH Posture Control (Get and Set)

# Add telemetry binary if enabled
if(BUILD_TELEMETRY)
    list(APPEND STAGING_COMMANDS_GET
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BINARY_DIR}/exe/telemetry" ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline/Modules/DscNativeResources/OsConfigResource/OSConfigTelemetry)
    list(APPEND STAGING_COMMANDS_SET
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BINARY_DIR}/exe/telemetry" ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline/Modules/DscNativeResources/OsConfigResource/OSConfigTelemetry)
endif()

# For Audit (Get):

add_custom_target(stage_create_ssh_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.mof" ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceSsh> ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    ${STAGING_COMMANDS_GET}
    DEPENDS OsConfigResourceSsh $<$<BOOL:${BUILD_TELEMETRY}>:telemetry>)

add_custom_target(create_ssh_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/LinuxSshServerSecurityBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/LinuxSshServerSecurityBaseline.zip" --format=zip .
    DEPENDS stage_create_ssh_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/StagingLinuxSshServerSecurityBaseline/)

# For Remediation (Set):

add_custom_target(stage_create_ssh_zip_set
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline/SetLinuxSshServerSecurityBaseline.metaconfig.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.mof" ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline/SetLinuxSshServerSecurityBaseline.mof
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceSsh> ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    ${STAGING_COMMANDS_SET}
    DEPENDS OsConfigResourceSsh $<$<BOOL:${BUILD_TELEMETRY}>:telemetry>)

add_custom_target(create_ssh_zip_set ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/SetLinuxSshServerSecurityBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/SetLinuxSshServerSecurityBaseline.zip" --format=zip .
    DEPENDS stage_create_ssh_zip_set
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/StagingSetLinuxSshServerSecurityBaseline/)
