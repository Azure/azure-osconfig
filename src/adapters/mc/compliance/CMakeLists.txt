# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Compliance module for Linux
project(OsConfigResourceCompliance)
if (BUILD_FUZZER)
    add_link_options("LINKER:-z,defs -shared-libasan")
else (BUILD_FUZZER)
    add_link_options("LINKER:-z,defs")
endif (BUILD_FUZZER)
add_library(OsConfigResourceCompliance
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c
        Baseline.c)

target_link_libraries(OsConfigResourceCompliance
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        compliancelib)

add_custom_target(stage_create_compliance_shell_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceShell.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceShell.mof" ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceCompliance> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:complianceschema,BINARY_DIR>/$<TARGET_PROPERTY:complianceschema,OUTPUT> ${PROJECT_BINARY_DIR}/Staging/
    DEPENDS OsConfigResourceCompliance ComplianceShell.mof complianceschema)

add_custom_target(create_compliance_shell_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/ComplianceShell.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/ComplianceShell.zip" --format=zip .
    DEPENDS stage_create_compliance_shell_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)

add_subdirectory(example)
