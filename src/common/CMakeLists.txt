# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(logging)
add_subdirectory(telemetry/inc)
add_subdirectory(commonutils)
add_subdirectory(mpiclient)
add_subdirectory(parson)
add_subdirectory(asb)
add_subdirectory(telemetry)
