# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

# Preserve the policy packaging location for a separately built executable.
set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/telemetry-external-prefix/src/telemetry-external-build")
if(OSCONFIG_TELEMETRY_EXECUTABLE)
    add_custom_target(telemetry-external
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OSCONFIG_TELEMETRY_EXECUTABLE}" "${BINARY_DIR}/bin/telemetrybin"
        BYPRODUCTS "${BINARY_DIR}/bin/telemetrybin"
        DEPENDS "${OSCONFIG_TELEMETRY_EXECUTABLE}"
        VERBATIM)
else()
    add_custom_target(telemetry-external
        COMMAND ${CMAKE_COMMAND} -E rm -f "${BINARY_DIR}/bin/telemetrybin"
        VERBATIM)
endif()

add_subdirectory(ssh)
add_subdirectory(asb)
