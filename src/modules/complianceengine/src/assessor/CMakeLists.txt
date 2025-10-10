# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(compliance-engine-assessor)
set(target_name compliance-engine-assessor)

set(SOURCES
    Main.cpp
    Mof.cpp
    BenchmarkFormatter.cpp
    CompactListFormatter.cpp
    DebugFormatter.cpp
    JsonFormatter.cpp
    NestedListFormatter.cpp
)

add_executable(${target_name} ${SOURCES})

target_include_directories(${target_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MODULES_INC_DIR}
    )

target_link_libraries(${target_name}
    ${CMAKE_DL_LIBS}
    logging
    commonutils
    parsonlib
    complianceenginelib
    )

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
