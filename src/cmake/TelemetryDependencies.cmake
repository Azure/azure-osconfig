# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include_guard(GLOBAL)
if(NOT OSCONFIG_DEPENDENCY_PREFIX)
    message(FATAL_ERROR "Telemetry requires the dependency prefix prepared by Dependencies.cmake")
endif()

set(OPENSSL_ROOT_DIR "${OSCONFIG_DEPENDENCY_PREFIX}")
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(OPENSSL_INCLUDE_DIR "${OSCONFIG_DEPENDENCY_PREFIX}/include")
set(OPENSSL_SSL_LIBRARY "${OSCONFIG_DEPENDENCY_PREFIX}/lib/libssl.a")
set(OPENSSL_CRYPTO_LIBRARY "${OSCONFIG_DEPENDENCY_PREFIX}/lib/libcrypto.a")
set(ZLIB_INCLUDE_DIR "${OSCONFIG_DEPENDENCY_PREFIX}/include")
set(ZLIB_LIBRARY "${OSCONFIG_DEPENDENCY_PREFIX}/lib/libz.a")
set(SQLite3_INCLUDE_DIR "${OSCONFIG_DEPENDENCY_PREFIX}/include")
set(SQLite3_LIBRARY "${OSCONFIG_DEPENDENCY_PREFIX}/lib/libsqlite3.a")

find_package(Threads REQUIRED)
find_package(OpenSSL 3.6.0 EXACT REQUIRED)
find_package(ZLIB 1.3.1 EXACT REQUIRED)
find_package(SQLite3 3.47.2 EXACT REQUIRED)
find_package(CURL 8.16.0 EXACT CONFIG REQUIRED
    PATHS "${OSCONFIG_DEPENDENCY_PREFIX}/lib/cmake/CURL" NO_DEFAULT_PATH)
find_package(nlohmann_json 3.11.3 EXACT CONFIG REQUIRED
    PATHS "${OSCONFIG_DEPENDENCY_PREFIX}/share/cmake/nlohmann_json" NO_DEFAULT_PATH)

add_library(osconfig_telemetry_dependencies INTERFACE)
target_link_libraries(osconfig_telemetry_dependencies INTERFACE
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
