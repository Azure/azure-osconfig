# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (BUILD_TELEMETRY)
    include(ExternalProject)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
        find_program(GCC5_FOUND gcc-5)
        if(NOT GCC5_FOUND)
            message(FATAL_ERROR "gcc-5 not found in PATH, but required for 1DS SDK with GCC < 5.5")
        endif()
        find_program(GPP5_FOUND g++-5)
        if(NOT GPP5_FOUND)
            message(FATAL_ERROR "g++-5 not found in PATH, but required for 1DS SDK with GCC < 5.5")
        endif()
        set(CMAKE_ARGS -DCMAKE_C_COMPILER=gcc-5 -DCMAKE_CXX_COMPILER=g++-5)
    endif()
    ExternalProject_Add(
        telemetry
        INSTALL_COMMAND ""  # No install target on telemetry target
        SOURCE_DIR
            ${COMMON_SRC_DIR}/telemetry
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            ${CMAKE_ARGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
            -DVCPKG_INSTALLED_DIR=${CMAKE_BINARY_DIR}/vcpkg_installed
            -DBUILD_TESTS=${BUILD_TESTS}
            -DGTEST_OUTPUT_DIR=${GTEST_OUTPUT_DIR}
            -DONEDS_CPP_SDK_VERSION=${ONEDS_CPP_SDK_VERSION}
    )
    ExternalProject_Get_Property(telemetry BINARY_DIR)
endif()

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
add_subdirectory(complianceengine)
