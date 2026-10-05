# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

if(BUILD_TELEMETRY)
    # Preserve the original policy packaging contract with the in-tree executable.
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/telemetry-external-prefix/src/telemetry-external-build")
    add_custom_target(telemetry-external
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:telemetryworker>" "${BINARY_DIR}/bin/telemetrybin"
        BYPRODUCTS "${BINARY_DIR}/bin/telemetrybin"
        DEPENDS telemetryworker
        VERBATIM)
endif()

add_subdirectory(ssh)
add_subdirectory(asb)
