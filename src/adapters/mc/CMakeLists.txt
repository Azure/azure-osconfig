# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (BUILD_TELEMETRY)
    add_subdirectory(${COMMON_SRC_DIR}/telemetry)
endif()

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
add_subdirectory(complianceengine)
