cmake_minimum_required(VERSION 3.28.0)
project(telemetry)
set (CMAKE_C_STANDARD 11)
# find_package (Threads)
# find_package(PkgConfig REQUIRED)
# pkg_search_module(CURL REQUIRED libcurl STATIC)
# find_package(ZLIB REQUIRED)
# set(CURL_USE_STATIC_LIBS TRUE)
# find_package(CURL REQUIRED)

# check HTTPS
# if(CURL_SUPPORTS_HTTPS)
#   message(STATUS "CURL supports HTTPS")
# else()
#   message(FATAL_ERROR "CURL does NOT support HTTPS")
# endif()

# Set 1DS SDK includes/libs
if(EXISTS "/usr/local/lib/libmat.a")
    set(MAT_SDK_LIB /usr/local/lib/)
else()
    message(FATAL_ERROR "libmat.a not found")
endif()
set(MAT_SDK_INCLUDE	/usr/local/include/mat)
# Prefer linking to more recent local sqlite3
# if(EXISTS "/usr/local/lib/libsqlite3.a")
#     set (SQLITE3_LIB "/usr/local/lib/libsqlite3.a")
# else()
#     set (SQLITE3_LIB "sqlite3")
# endif()

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

add_library(telemetry STATIC
    telemetry.c
)

target_link_libraries(telemetry
    # PRIVATE
    PUBLIC
        ${MAT_SDK_LIB}/libmat.a
        curl
        z
        dl
        ${CMAKE_THREAD_LIBS_INIT}
        SQLite::SQLite3
        # dl
        # CURL::libcurl
        # ZLIB::ZLIB
)

target_include_directories(telemetry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    # PRIVATE
        ${MAT_SDK_INCLUDE}
)
