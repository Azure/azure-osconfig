# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrylib)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(../../../cmake/TelemetryDependencies.cmake)

include(ExternalProject)
set(MAT_SDK_LIB ${CMAKE_BINARY_DIR}/1ds_install/lib)
set(MAT_SDK_INCLUDE ${CMAKE_BINARY_DIR}/1ds_install/include/mat)
ExternalProject_Add(
    1dscpplib
    GIT_REPOSITORY
        https://github.com/microsoft/cpp_client_telemetry.git
    GIT_TAG
        ${ONEDS_CPP_SDK_VERSION}
    GIT_SUBMODULES
        ""           # This disables submodule checkout
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DBUILD_FUNC_TESTS=OFF
        -DBUILD_UNIT_TESTS=OFF
        "-DCMAKE_CXX_FLAGS=-w -I\"${OSCONFIG_DEPENDENCY_PREFIX}/include\""
        "-DCMAKE_C_FLAGS=-w -I\"${OSCONFIG_DEPENDENCY_PREFIX}/include\""
        -DCMAKE_PREFIX_PATH=${OSCONFIG_DEPENDENCY_PREFIX}
        -DOPENSSL_ROOT_DIR=${OSCONFIG_DEPENDENCY_PREFIX}
        -DOPENSSL_USE_STATIC_LIBS=ON
        -DZLIB_INCLUDE_DIR=${OSCONFIG_DEPENDENCY_PREFIX}/include
        -DZLIB_LIBRARY=${OSCONFIG_DEPENDENCY_PREFIX}/lib/libz.a
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/1ds_install
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    BUILD_BYPRODUCTS ${MAT_SDK_LIB}/libmat.a
)

if(BUILD_EXTERNAL)
    include_directories(../../telemetry)
    add_subdirectory(../../parson ${CMAKE_CURRENT_BINARY_DIR}/parson)
    add_subdirectory(../../logging ${CMAKE_CURRENT_BINARY_DIR}/logging)
    add_subdirectory(../../commonutils ${CMAKE_CURRENT_BINARY_DIR}/commonutils)
endif()

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

if(BUILD_EXTERNAL)
    # commonutils is used which uses telemetry, create an alias to avoid name conflict
    add_library(telemetry ALIAS telemetrylib)
endif()

target_link_libraries(telemetrylib
    PUBLIC
        logging
        commonutils
    PRIVATE
        ${MAT_SDK_LIB}/libmat.a
        osconfig_telemetry_dependencies
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
