# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

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

target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security)

target_include_directories(${target_name} PUBLIC
    ${PLATFORM_INC_DIR})

target_link_libraries(${target_name}
    parsonlib
    logging
    commonutils
    mpiclient)

# Keep frame pointers (-fno-omit-frame-pointer) so the common crash handler can walk the
# stack reliably, and export symbols (-rdynamic) so backtrace_symbols_fd() can resolve this
# executable's own function names in crash stack traces.
target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
target_link_options(${target_name} PRIVATE -rdynamic)

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
