# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(osconfig_dependencies LANGUAGES C CXX)

include(ExternalProject)
include("${CMAKE_CURRENT_LIST_DIR}/AssetSources.cmake")
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

option(BUILD_TESTS "Build test dependencies" ON)

set(_cmake_args
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    "-DBUILD_SHARED_LIBS=OFF"
)
if(CMAKE_TOOLCHAIN_FILE)
    list(APPEND _cmake_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

if(BUILD_TESTS)
    osconfig_asset_download(gtest_download
        https://github.com/google/googletest/archive/release-1.12.0.tar.gz
        6216e76a8c988b6b3739f3988c85f369eef2a8036c4412621a0d3d04ceeada00d35e487363be0a265035ac78f1a5065e1fe054a285c43df23b6abcc69f8bfe3d)
    ExternalProject_Add(googletest
        ${gtest_download}
        CMAKE_ARGS ${_cmake_args} -DBUILD_GMOCK=ON -DINSTALL_GTEST=ON
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
endif()
