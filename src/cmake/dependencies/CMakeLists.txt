# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(osconfig_dependencies LANGUAGES C CXX)

include(ExternalProject)
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
    ExternalProject_Add(openssl
        URL https://github.com/openssl/openssl/releases/download/openssl-3.6.0/openssl-3.6.0.tar.gz
        URL_HASH SHA256=b6a5f44b7eb69e3fa35dbf15524405b44837a481d43d81daddde3ff21fcbb8e9
        TLS_VERIFY TRUE
        CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
            "${PERL_EXECUTABLE}" <SOURCE_DIR>/Configure
            "--prefix=${CMAKE_INSTALL_PREFIX}" --libdir=lib --openssldir=/etc/ssl
            no-shared no-module no-tests -fPIC ${_openssl_mode}
        BUILD_COMMAND "${MAKE_EXECUTABLE}" "-j${_openssl_jobs}" build_sw
        INSTALL_COMMAND "${MAKE_EXECUTABLE}" install_sw
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    ExternalProject_Add(zlib
        URL https://zlib.net/fossils/zlib-1.3.1.tar.gz
        URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
        TLS_VERIFY TRUE
        CMAKE_ARGS ${_cmake_args} -DZLIB_BUILD_EXAMPLES=OFF
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    ExternalProject_Add(sqlite3
        URL https://www.sqlite.org/2024/sqlite-autoconf-3470200.tar.gz
        URL_HASH SHA256=f1b2ee412c28d7472bc95ba996368d6f0cdcf00362affdadb27ed286c179540b
        TLS_VERIFY TRUE
        PATCH_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/SQLiteCMakeLists.txt" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${_cmake_args}
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    ExternalProject_Add_StepDependencies(sqlite3 patch "${CMAKE_CURRENT_LIST_DIR}/SQLiteCMakeLists.txt")
    ExternalProject_Add(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
        TLS_VERIFY TRUE
        CMAKE_ARGS ${_cmake_args} -DJSON_BuildTests=OFF -DJSON_Install=ON
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
    ExternalProject_Add(curl
        DEPENDS openssl zlib
        URL https://curl.se/download/curl-8.16.0.tar.gz
        URL_HASH SHA256=a21e20476e39eca5a4fc5cfb00acf84bbc1f5d8443ec3853ad14c26b3c85b970
        TLS_VERIFY TRUE
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
    ExternalProject_Add(googletest
        URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/release-1.12.0
        URL_HASH SHA256=2a4f11dce6188b256f3650061525d0fe352069e5c162452818efbbf8d0b5fe1c
        TLS_VERIFY TRUE
        CMAKE_ARGS ${_cmake_args} -DBUILD_GMOCK=ON -DINSTALL_GTEST=ON
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON
    )
endif()
