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

add_mc_package(
    TARGET_NAME OsConfigResourceSsh
    MOF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.mof
    META_CONFIG_JSON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/LinuxSshServerSecurityBaseline.metaconfig.json
    DESTINATION_FILE LinuxSshServerSecurityBaseline)
