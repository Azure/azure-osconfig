cmake_minimum_required(VERSION 3.28.0)
project(telemetry)

# Must be built with GCC 5 or later
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
    message(FATAL_ERROR "GCC version must be 5.5 or later")
endif()
add_subdirectory(lib)
add_subdirectory(exe)
