# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrybin)
cmake_minimum_required(VERSION 3.21)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)  # required by 1DS SDK

# --- Key injection (belongs HERE, not in NRP) ---
if(DEFINED ENV{OsConfigTelemetryApiKey} AND NOT "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
    set(OsConfigTelemetryApiKey $ENV{OsConfigTelemetryApiKey})
    message(STATUS "Using OsConfigTelemetryApiKey from environment variable")
else()
    message(WARNING "OsConfigTelemetryApiKey not set")
    set(OsConfigTelemetryApiKey "999...")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../lib/Keys.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Keys.h @ONLY)

# --- SDK dependencies (belongs HERE, not in NRP) ---
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(unofficial-sqlite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

# --- Build ---
add_executable(telemetrybin main.cpp args.cpp)

target_link_libraries(telemetrybin PRIVATE
    telemetrylib
    ${MAT_SDK_LIB}/libmat.a
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
    CURL::libcurl
    OpenSSL::SSL
    dl
    ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(telemetrybin PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # generated Keys.h
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../commonutils
    ${MAT_SDK_INCLUDE})

install(TARGETS telemetrybin
    RUNTIME DESTINATION /var/lib/osconfig/telemetry
    RENAME OSConfigTelemetry)
