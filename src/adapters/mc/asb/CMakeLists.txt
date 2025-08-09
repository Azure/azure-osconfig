# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(ExternalProject)

# Azure Security Baseline for Linux
project(OsConfigResourceAsb)

add_library(OsConfigResourceAsb
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c
        Baseline.c)

target_link_libraries(OsConfigResourceAsb
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        asb)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceAsb PROPERTIES OUTPUT_NAME libOsConfigResource)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
    set(CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_DIR}/gcc-5-toolchain.cmake)
endif()
ExternalProject_Add(
    telemetry
    INSTALL_COMMAND ""  # No install target on telemetry target
    SOURCE_DIR
        ${COMMON_SRC_DIR}/telemetry
    CMAKE_ARGS
        ${CMAKE_ARGS}
        -DCMAKE_ARGS=${CMAKE_ARGS}
        -DEXTERNAL_SRC_DIR=${EXTERNAL_SRC_DIR}
        -DCMAKE_TOOLCHAIN_DIR=${CMAKE_TOOLCHAIN_DIR}
)
ExternalProject_Get_Property(telemetry BINARY_DIR)

# Create the policy artifacts ZIP package for the Azure Security Baseline for Linux

add_custom_target(stage_create_asb_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceAsb> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BINARY_DIR}/exe/telemetry" ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/telemetry
    DEPENDS OsConfigResourceAsb telemetry)

add_custom_target(create_asb_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip" --format=zip .
    DEPENDS stage_create_asb_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)
