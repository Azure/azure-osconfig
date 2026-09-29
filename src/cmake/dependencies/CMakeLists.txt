# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(osconfig_dependencies LANGUAGES C CXX)

include(ExternalProject)
include("${CMAKE_CURRENT_LIST_DIR}/AssetSources.cmake")
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

option(BUILD_TELEMETRY "Build telemetry dependencies" ON)
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

if(BUILD_TELEMETRY)
    find_package(Perl REQUIRED)
    find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)
    set(_openssl_jobs 2)
    if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL} AND NOT "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}" STREQUAL "")
        if(NOT "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}" MATCHES "^[1-9][0-9]*$")
            message(FATAL_ERROR "CMAKE_BUILD_PARALLEL_LEVEL must be a positive integer")
        endif()
        set(_openssl_jobs "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}")
    endif()
    set(_openssl_mode)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_openssl_mode --debug)
    endif()
    # Native builds include CI's emulated ARM containers; do not guess cross targets.
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Build dependencies inside the target Linux environment, not with a cross toolchain")
    endif()
    osconfig_asset_download(openssl_download
        https://github.com/openssl/openssl/archive/openssl-3.6.0.tar.gz
        3e1796708155454c118550ba0964b42c0c1055b651fec00cfb55038e8a8abbf5f85df02449e62b50b99d2a4a2f7b47862067f8a965e9c8a72f71dee0153672d9)
    ExternalProject_Add(openssl
        ${openssl_download}
        CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
            "${PERL_EXECUTABLE}" <SOURCE_DIR>/Configure
            "--prefix=${CMAKE_INSTALL_PREFIX}" --libdir=lib --openssldir=/etc/ssl
            no-shared no-module no-tests -fPIC ${_openssl_mode}
        BUILD_COMMAND "${MAKE_EXECUTABLE}" "-j${_openssl_jobs}" build_sw
        INSTALL_COMMAND "${MAKE_EXECUTABLE}" install_sw
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    osconfig_asset_download(zlib_download
        https://zlib.net/fossils/zlib-1.3.1.tar.gz
        580677aad97093829090d4b605ac81c50327e74a6c2de0b85dd2e8525553f3ddde17556ea46f8f007f89e435493c9a20bc997d1ef1c1c2c23274528e3c46b94f)
    ExternalProject_Add(zlib
        ${zlib_download}
        CMAKE_ARGS ${_cmake_args} -DZLIB_BUILD_EXAMPLES=OFF
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    osconfig_asset_download(sqlite3_download
        https://www.sqlite.org/2024/sqlite-autoconf-3470200.tar.gz
        9f311e7834330d112a633a9ae7c211c602b36b6c44f94a7ff7463217da8f09ba2c469d1f2ca38da5ba88358c40e328a8958c1e4ad466b016f3bd2799ef2431e2)
    ExternalProject_Add(sqlite3
        ${sqlite3_download}
        PATCH_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/SQLiteCMakeLists.txt" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${_cmake_args}
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    ExternalProject_Add_StepDependencies(sqlite3 patch "${CMAKE_CURRENT_LIST_DIR}/SQLiteCMakeLists.txt")
    osconfig_asset_download(json_download
        https://github.com/nlohmann/json/archive/v3.11.3.tar.gz
        7df19b621de34f08d5d5c0a25e8225975980841ef2e48536abcf22526ed7fb99f88ad954a2cb823115db59ccc88d1dbe74fe6c281b5644b976b33fb78db9d717)
    ExternalProject_Add(nlohmann_json
        ${json_download}
        CMAKE_ARGS ${_cmake_args} -DJSON_BuildTests=OFF -DJSON_Install=ON
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    osconfig_asset_download(curl_download
        https://github.com/curl/curl/archive/curl-8_16_0.tar.gz
        9530537a2e9b1e22fa75f3816a122f34b46348e462e74bae2eb3aaa8927de44ac1dd4ce9517f38937d32de4add3151071cc01bdd5554a2000aa9f4db18c0de9b)
    ExternalProject_Add(curl
        DEPENDS openssl zlib
        ${curl_download}
        CMAKE_ARGS ${_cmake_args}
            "-DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}"
            "-DOPENSSL_ROOT_DIR=${CMAKE_INSTALL_PREFIX}"
            "-DOPENSSL_INCLUDE_DIR=${CMAKE_INSTALL_PREFIX}/include"
            "-DOPENSSL_SSL_LIBRARY=${CMAKE_INSTALL_PREFIX}/lib/libssl.a"
            "-DOPENSSL_CRYPTO_LIBRARY=${CMAKE_INSTALL_PREFIX}/lib/libcrypto.a"
            "-DZLIB_INCLUDE_DIR=${CMAKE_INSTALL_PREFIX}/include"
            "-DZLIB_LIBRARY=${CMAKE_INSTALL_PREFIX}/lib/libz.a"
            -DOPENSSL_USE_STATIC_LIBS=ON
            -DBUILD_STATIC_LIBS=ON
            -DBUILD_CURL_EXE=OFF
            -DBUILD_TESTING=OFF
            -DBUILD_EXAMPLES=OFF
            -DBUILD_LIBCURL_DOCS=OFF
            -DBUILD_MISC_DOCS=OFF
            -DENABLE_CURL_MANUAL=OFF
            -DCURL_USE_OPENSSL=ON
            -DCURL_USE_PKGCONFIG=OFF
            -DCURL_CA_FALLBACK=ON
            -DCURL_ZLIB=ON
            -DCURL_BROTLI=OFF
            -DCURL_ZSTD=OFF
            -DCURL_USE_LIBPSL=OFF
            -DCURL_USE_LIBSSH2=OFF
            -DCURL_USE_LIBSSH=OFF
            -DCURL_USE_GSSAPI=OFF
            -DCURL_USE_GSASL=OFF
            -DCURL_DISABLE_LDAP=ON
            -DCURL_DISABLE_LDAPS=ON
            -DCURL_DISABLE_WEBSOCKETS=ON
            -DUSE_LIBIDN2=OFF
            -DUSE_NGHTTP2=OFF
            -DUSE_NGTCP2=OFF
            -DUSE_LIBRTMP=OFF
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
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
