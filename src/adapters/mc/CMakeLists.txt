# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (BUILD_TELEMETRY)
    include(ExternalProject)
    if(BUILD_FUZZER)
        set(CMAKE_ARGS
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        )
    endif()
    ExternalProject_Add(
        telemetry-external
        INSTALL_COMMAND ""  # No install target on telemetry target
        BUILD_ALWAYS TRUE
        SOURCE_DIR
            ${COMMON_SRC_DIR}/telemetry
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_C_COMPILER=${OSCONFIG_TELEMETRY_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${OSCONFIG_TELEMETRY_CXX_COMPILER}
            ${CMAKE_ARGS}
            -DBUILD_EXTERNAL=1
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DOSCONFIG_DEPENDENCY_PREFIX=${OSCONFIG_DEPENDENCY_PREFIX}
            -DOSCONFIG_LEGACY_TELEMETRY=${OSCONFIG_LEGACY_TELEMETRY}
            -DBUILD_TESTS=${BUILD_TESTS}
            -DGTEST_OUTPUT_DIR=${GTEST_OUTPUT_DIR}
            -DONEDS_CPP_SDK_VERSION=${ONEDS_CPP_SDK_VERSION}
        BUILD_BYPRODUCTS
            <BINARY_DIR>/lib/libtelemetrylib.a
            <BINARY_DIR>/1ds_install/lib/libmat.a
            <BINARY_DIR>/bin/telemetrybin
    )
    ExternalProject_Get_Property(telemetry-external BINARY_DIR)

    set(TELEMETRY_LIB_DIR ${BINARY_DIR}/lib CACHE PATH "Telemetry library directory" FORCE)
    set(MAT_SDK_LIB ${BINARY_DIR}/1ds_install/lib)
    set(MAT_SDK_INCLUDE ${BINARY_DIR}/1ds_install/include/mat)

    include(${CMAKE_SOURCE_DIR}/cmake/TelemetryDependencies.cmake)

    add_library(telemetry-interface INTERFACE)
    add_dependencies(telemetry-interface telemetry-external)
    target_link_libraries(telemetry-interface
        INTERFACE
            ${BINARY_DIR}/lib/libtelemetrylib.a
            ${MAT_SDK_LIB}/libmat.a
            osconfig_telemetry_dependencies
    )
    target_include_directories(telemetry-interface
        INTERFACE
            ${BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/common/telemetry/lib
            ${MAT_SDK_INCLUDE}
            ${TELEMETRY_LIB_DIR}
    )
endif()

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
