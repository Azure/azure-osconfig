# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrybin)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

add_executable(telemetrybin main.cpp args.cpp)

target_link_libraries(telemetrybin PRIVATE telemetrylib)
if(OSCONFIG_LEGACY_TELEMETRY)
    # The GCC-5 helper must not require a newer libstdc++ on the target machine.
    target_link_options(telemetrybin PRIVATE -static-libstdc++ -static-libgcc)
endif()
target_include_directories(telemetrybin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../commonutils)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
