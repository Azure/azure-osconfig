# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Compliance module for Linux
project(OsConfigResourceComplianceEngine)
if (BUILD_FUZZER)
    add_link_options("LINKER:-z,defs -shared-libasan")
else (BUILD_FUZZER)
    add_link_options("LINKER:-z,defs")
endif (BUILD_FUZZER)
add_library(OsConfigResourceComplianceEngine
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c
        Baseline.c)

target_link_libraries(OsConfigResourceComplianceEngine
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        complianceenginelib)

add_custom_target(stage_create_compliance_engine_shell_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineShell.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineShell.mof" ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceComplianceEngine> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:complianceengineschema,BINARY_DIR>/$<TARGET_PROPERTY:complianceengineschema,OUTPUT> ${PROJECT_BINARY_DIR}/Staging/
    DEPENDS OsConfigResourceComplianceEngine ComplianceEngineShell.mof complianceengineschema)

add_custom_target(create_compliance_engine_shell_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/ComplianceEngineShell.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/ComplianceEngineShell.zip" --format=zip .
    DEPENDS stage_create_compliance_engine_shell_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)

add_subdirectory(example)
