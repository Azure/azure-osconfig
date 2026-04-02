# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(osconfig)

set(osconfig_files
    ./Agent.c
    ./Watcher.c)

set(target_name osconfig)

add_executable(${target_name} ${osconfig_files})

target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security -Wno-unused-result)

find_package (Parson REQUIRED)

target_include_directories(${target_name} PUBLIC
    ${SHARED_UTIL_INC_FOLDER}
    ${MACRO_UTILS_INC_FOLDERP}
    ${PLATFORM_INC_DIR})

target_link_libraries(${target_name}
    parson
    logging
    commonutils
    mpiclient
    ${CMAKE_DL_LIBS})

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.conn DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.toml DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/daemon/${target_name}.toml DESTINATION /etc/aziot/identityd/config.d/)
