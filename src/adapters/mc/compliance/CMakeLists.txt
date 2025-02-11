# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Compliance module for Linux
project(OsConfigResourceCompliance)
add_link_options("LINKER:-z,defs")
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
