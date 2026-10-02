# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include_guard(GLOBAL)

if(NOT BUILD_TESTS)
    return()
endif()

if(NOT DEFINED OSCONFIG_DEPENDENCY_PREFIX)
    set(OSCONFIG_DEPENDENCY_PREFIX "${CMAKE_BINARY_DIR}/dependencies/install")
    set(_dependency_build_type "${CMAKE_BUILD_TYPE}")
    if(NOT _dependency_build_type)
        set(_dependency_build_type Release)
    endif()
    set(_dependency_toolchain_args)
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND _dependency_toolchain_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()
    # Only the test framework is downloaded; telemetry uses OS runtime facilities.
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -S "${CMAKE_CURRENT_LIST_DIR}/dependencies"
            -B "${CMAKE_BINARY_DIR}/dependencies"
            -G "${CMAKE_GENERATOR}"
            "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=${_dependency_build_type}"
            "-DCMAKE_INSTALL_PREFIX=${OSCONFIG_DEPENDENCY_PREFIX}"
            "-DBUILD_TESTS=${BUILD_TESTS}"
            ${_dependency_toolchain_args}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}/dependencies" --config "${_dependency_build_type}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

list(PREPEND CMAKE_PREFIX_PATH "${OSCONFIG_DEPENDENCY_PREFIX}")
if(BUILD_TESTS)
    # The checksum-pinned release-1.12.0 archive reports 1.11.0 in its CMake package.
    find_package(GTest 1.11.0 EXACT CONFIG REQUIRED
        PATHS "${OSCONFIG_DEPENDENCY_PREFIX}/lib/cmake/GTest" NO_DEFAULT_PATH)
endif()
