# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9")
        add_compile_options("-Wunused-const-variable=2")
    else()
        add_compile_options("-Wunused-variable")
    endif()
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options("-Wunused-variable")
endif()

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(osconfig-platform)

set(osconfig_platform_files
    ./Main.c
    ./MmiClient.c
    ./ModulesManager.c
    ./MpiServer.c)

set(target_name osconfig-platform)

add_executable(${target_name} ${osconfig_platform_files})

target_include_directories(${target_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MODULES_INC_DIR}
    ${PLATFORM_INC_DIR})

target_link_libraries(${target_name}
    ${CMAKE_DL_LIBS}
    pthread
    logging
    commonutils
    parsonlib)

# Keep frame pointers (-fno-omit-frame-pointer) so the common crash handler can walk the
# stack reliably, and export symbols (-rdynamic) so backtrace_symbols_fd() can resolve this
# executable's own function names in crash stack traces.
target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
target_link_options(${target_name} PRIVATE -rdynamic)

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
