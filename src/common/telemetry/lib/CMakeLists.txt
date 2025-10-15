# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrylib)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

# 1DS SDK Requires atleast GCC 5.5
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

# Force vcpkg-only usage - determine vcpkg installed directory
if (DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/vcpkg_installed")
endif()
set(VCPKG_PACKAGES_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

# Set CMAKE_PREFIX_PATH to vcpkg only
set(CMAKE_PREFIX_PATH "${VCPKG_PACKAGES_DIR}")

include(ExternalProject)
ExternalProject_Add(
    1dscpplib
    GIT_REPOSITORY
        https://github.com/microsoft/cpp_client_telemetry.git
    GIT_TAG
        ${ONEDS_CPP_SDK_VERSION}
    GIT_SUBMODULES
        ""           # This disables submodule checkout
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}  # use the same toolchain
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DBUILD_FUNC_TESTS=OFF
        -DBUILD_UNIT_TESTS=OFF
        -DCMAKE_CXX_FLAGS="-w"    # Suppress all warnings
        -DCMAKE_C_FLAGS="-w"      # Suppress all warnings
        # Pass vcpkg paths to the 1DS SDK build
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        # Install to vcpkg's installed directory instead of system paths
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/1ds_install
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
)

# Get the install directory for 1DS SDK
ExternalProject_Get_Property(1dscpplib BINARY_DIR)
set(MAT_SDK_LIB ${CMAKE_BINARY_DIR}/1ds_install/lib)
set(MAT_SDK_INCLUDE ${CMAKE_BINARY_DIR}/1ds_install/include/mat)

# Use vcpkg-managed dependencies with NO_DEFAULT_PATH to prevent system fallback
find_package(CURL REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
find_package(OpenSSL REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
find_package(Threads REQUIRED)  # Threads can still use system
find_package(unofficial-sqlite3 REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
find_package(nlohmann_json REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})

include_directories(../../telemetry)
add_subdirectory(../../parson ${CMAKE_CURRENT_BINARY_DIR}/parson)
add_subdirectory(../../logging ${CMAKE_CURRENT_BINARY_DIR}/logging)
add_subdirectory(../../commonutils ${CMAKE_CURRENT_BINARY_DIR}/commonutils)

# Add telemetry ingestion key (if defined)
if(DEFINED ENV{OsConfigTelemetryApiKey} AND NOT "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
    set(OsConfigTelemetryApiKey $ENV{OsConfigTelemetryApiKey})
    message(STATUS "Using OsConfigTelemetryApiKey from environment variable")
else()
    message(WARNING "OsConfigTelemetryApiKey not set, using default value")
    set(OsConfigTelemetryApiKey "999999999999999999999999999999999-999999999-9999-9999-9999-999999999999-9999")
endif()
configure_file(Keys.h.in ${CMAKE_CURRENT_BINARY_DIR}/Keys.h @ONLY)

add_library(telemetrylib STATIC
    Telemetry.cpp
)
add_dependencies(telemetrylib 1dscpplib)

# commonutils provides public telemetry targets not available since we are built
# externally. Create an alias for telemetry
add_library(telemetry ALIAS telemetrylib)

target_link_libraries(telemetrylib
    PUBLIC
        commonutils
        nlohmann_json::nlohmann_json
    PRIVATE
        ${MAT_SDK_LIB}/libmat.a
        unofficial::sqlite3::sqlite3
        CURL::libcurl
        OpenSSL::SSL
        dl
        ${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories(telemetrylib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}../
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${MAT_SDK_INCLUDE}
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
