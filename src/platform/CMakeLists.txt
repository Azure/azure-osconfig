# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(PLATFORM_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../modules/inc)
set(MODULESMANAGER_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modulesmanager/inc)
set(ORCHESTRATOR_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/orchestrator/inc)

add_subdirectory(modulesmanager)
add_subdirectory(orchestrator)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(osconfig-platform)

set(osconfig_platform_files
    ./AisUtils.c
    ./ConfigUtils.c
    ./MpiClient.c
    ./PnpAgent.c
    ./PnpUtils.c)

set(target_name osconfig-platform)

add_executable(${target_name} ${osconfig_platform_files})

target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wformat-security)

target_include_directories(${target_name} PUBLIC)

target_link_libraries(${target_name}
    logging
    commonutils
    modulesmanager
    orchestrator)

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)