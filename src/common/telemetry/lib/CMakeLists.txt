project(telemetrylib)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

# 1DS SDK Requires atleast GCC 5.5
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
    set(CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_DIR}/gcc-5-toolchain.cmake)
endif()
include(ExternalProject)
ExternalProject_Add(
    1dscpplib
    GIT_REPOSITORY
        https://github.com/microsoft/cpp_client_telemetry.git
    GIT_TAG
        3.9.22.1
    GIT_SUBMODULES ""           # This disables submodule checkout
    CMAKE_ARGS
        ${CMAKE_ARGS}
        -DBUILD_FUNC_TESTS=OFF
        -DBUILD_UNIT_TESTS=OFF
        -DCMAKE_CXX_FLAGS="-w"    # Suppress all warnings
        -DCMAKE_C_FLAGS="-w"      # Suppress all warnings
)

# Set 1DS SDK includes/libs
set(MAT_SDK_LIB /usr/local/lib/)
set(MAT_SDK_INCLUDE	/usr/local/include/mat)

# set(CURL_USE_STATIC_LIBS TRUE)
# find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS ON)
pkg_check_modules(LIBCURL_PKG REQUIRED IMPORTED_TARGET libcurl)

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

add_subdirectory(../../parson ${CMAKE_CURRENT_BINARY_DIR}/parson)

if(DEFINED ENV{OsConfigTelemetryApiKey} AND NOT "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
    set(OsConfigTelemetryApiKey $ENV{OsConfigTelemetryApiKey})
    message(STATUS "Using OsConfigTelemetryApiKey from environment variable")
else()
    message(WARNING "OsConfigTelemetryApiKey not set, using default value")
    set(OsConfigTelemetryApiKey "999999999999999999999999999999999-999999999-9999-9999-9999-999999999999-9999")
endif()
configure_file(keys.h.in ${CMAKE_CURRENT_SOURCE_DIR}/keys.h @ONLY)

add_library(telemetrylib STATIC
    telemetry.cpp
)
add_dependencies(telemetrylib 1dscpplib)

target_link_libraries(telemetrylib
    PUBLIC
        parsonlib
        dl
        ${CMAKE_THREAD_LIBS_INIT}
    PRIVATE
        ${MAT_SDK_LIB}/libmat.a
        SQLite::SQLite3
        # CURL::libcurl
        PkgConfig::LIBCURL_PKG
        ZLIB::ZLIB
)

target_include_directories(telemetrylib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MAT_SDK_INCLUDE}
)
