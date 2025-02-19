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
