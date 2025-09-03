# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetry)
add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(logging)
add_subdirectory(commonutils)
add_subdirectory(mpiclient)
add_subdirectory(parson)
if(ENABLE_TELEMETRY)
    add_subdirectory(telemetry/libjson)
else()
    add_subdirectory(telemetry/libstub)
endif()
add_subdirectory(asb)
