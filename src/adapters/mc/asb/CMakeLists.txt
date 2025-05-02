# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Azure Security Baseline for Linux
project(OsConfigResourceAsb)

include_directories(${CMAKE_SOURCE_DIR}/platform/inc)
include_directories(${CMAKE_SOURCE_DIR}/common/asb)
include_directories(${CMAKE_SOURCE_DIR}/common/commonutils)
include_directories(${CMAKE_SOURCE_DIR}/common/logging)
include_directories(${CMAKE_SOURCE_DIR}/adapters/mc)
include_directories(${CMAKE_SOURCE_DIR}/common/mpiclient)
include_directories(${CMAKE_SOURCE_DIR}/common/parson)

set(ASB_SOURCES
    ${CMAKE_SOURCE_DIR}/common/asb/Asb.c)

set(COMMONUTILS_SOURCES
    ${CMAKE_SOURCE_DIR}/common/commonutils/CommandUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/ConfigUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/DaemonUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/DeviceInfoUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/FileUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/MountUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/OtherUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/PackageUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/PassUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/PerfUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/SshUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/UrlUtils.c
    ${CMAKE_SOURCE_DIR}/common/commonutils/UserUtils.c)

set(LOGGING_SOURCES
    ${CMAKE_SOURCE_DIR}/common/logging/Logging.c)

set(MC_SOURCES
    ${CMAKE_SOURCE_DIR}/adapters/mc/module.c
    ${CMAKE_SOURCE_DIR}/adapters/mc/schema.c
    ${CMAKE_SOURCE_DIR}/adapters/mc/OsConfigResource.c)

set(MPICLIENT_SOURCES
    ${CMAKE_SOURCE_DIR}/common/mpiclient/MpiClient.c)

set(PARSON_SOURCES
    ${CMAKE_SOURCE_DIR}/common/parson/parson.c)

set(ALL_SOURCES
    ${ASB_SOURCES}
    ${COMMONUTILS_SOURCES}
    ${LOGGING_SOURCES}
    ${MC_SOURCES}
    ${MPICLIENT_SOURCES}
    ${PARSON_SOURCES}
    Baseline.c)

add_library(OsConfigResourceAsb SHARED ${ALL_SOURCES})

target_link_libraries(OsConfigResourceAsb)

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