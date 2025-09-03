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

add_mc_package(
    TARGET_NAME OsConfigResourceComplianceEngine
    MOF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineShell.mof
    META_CONFIG_JSON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineShell.metaconfig.json
    DESTINATION_FILE ComplianceEngineShell
    ADDITIONAL_COMMANDS_GET
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:complianceengineschema,BINARY_DIR>/$<TARGET_PROPERTY:complianceengineschema,OUTPUT> ${PROJECT_BINARY_DIR}/GetStaging/
    ADDITIONAL_COMMANDS_SET
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:complianceengineschema,BINARY_DIR>/$<TARGET_PROPERTY:complianceengineschema,OUTPUT> ${PROJECT_BINARY_DIR}/SetStaging/
    ADDITIONAL_DEPENDS
        complianceengineschema ComplianceEngineShell.mof)

add_subdirectory(example)
