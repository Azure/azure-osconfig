# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

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

add_mc_package(
    TARGET_NAME OsConfigResourceAsb
    MOF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.mof
    META_CONFIG_JSON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.metaconfig.json
    DESTINATION_FILE AzureLinuxBaseline)
