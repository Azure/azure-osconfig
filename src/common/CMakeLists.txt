# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(logging)
add_subdirectory(commonutils)
add_subdirectory(mpiclient)
add_subdirectory(parson)
add_subdirectory(telemetry)
