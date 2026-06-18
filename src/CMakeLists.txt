# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# VCPKG integration
if (DEFINED ENV{VCPKG_ROOT})
    message (FATAL_ERROR "VCPKG_ROOT found, please unset the environment variable to use the git submodule version of vcpkg")
endif()

set(VCPKG_SOURCE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg")
set(VCPKG_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_root")
set(VCPKG_TOOLCHAIN_FILE_PATH "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_EXECUTABLE_PATH "${VCPKG_ROOT_DIR}/vcpkg")

function(initialize_vcpkg_submodule)
    if(NOT EXISTS "${VCPKG_SOURCE_ROOT_DIR}/bootstrap-vcpkg.sh")
        message(STATUS "Initializing external/vcpkg submodule")
        execute_process(
            COMMAND git submodule init external/vcpkg
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
            RESULT_VARIABLE SUBMODULE_INIT_RESULT
        )
        if(NOT SUBMODULE_INIT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed: git submodule init external/vcpkg (exit code: ${SUBMODULE_INIT_RESULT})")
            return()
        endif()

        execute_process(
            COMMAND git submodule update --recursive external/vcpkg
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
            RESULT_VARIABLE SUBMODULE_UPDATE_RESULT
        )
        if(NOT SUBMODULE_UPDATE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed: git submodule update --recursive external/vcpkg (exit code: ${SUBMODULE_UPDATE_RESULT})")
            return()
        endif()
    endif()

    if(NOT EXISTS "${VCPKG_SOURCE_ROOT_DIR}/bootstrap-vcpkg.sh")
        message(FATAL_ERROR "Failed: ${VCPKG_SOURCE_ROOT_DIR}/bootstrap-vcpkg.sh not found after submodule initialization")
        return()
    endif()
endfunction()

function(initialize_vcpkg_submodule_and_bootstrap)
    initialize_vcpkg_submodule()

    if(NOT EXISTS "${VCPKG_ROOT_DIR}")
        message(STATUS "Copying external/vcpkg to ${VCPKG_ROOT_DIR}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${VCPKG_SOURCE_ROOT_DIR}" "${VCPKG_ROOT_DIR}"
            RESULT_VARIABLE VCPKG_COPY_RESULT
        )
        if(NOT VCPKG_COPY_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed: copy external/vcpkg to ${VCPKG_ROOT_DIR} (exit code: ${VCPKG_COPY_RESULT})")
            return()
        endif()
    endif()

    # Bootstrap vcpkg
    execute_process(
        COMMAND bash bootstrap-vcpkg.sh
        WORKING_DIRECTORY "${VCPKG_ROOT_DIR}"
        RESULT_VARIABLE BOOTSTRAP_RESULT
    )
    if(NOT BOOTSTRAP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed: bootstrap-vcpkg.sh (exit code: ${BOOTSTRAP_RESULT})")
        return()
    endif()
endfunction()

set(VCPKG_ROOT "${VCPKG_ROOT_DIR}" CACHE PATH "Build-local vcpkg root")

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (EXISTS ${VCPKG_TOOLCHAIN_FILE_PATH})
        set (CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN_FILE_PATH} CACHE STRING "Vcpkg toolchain file")
        message (STATUS "Using build-local vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message (STATUS "Initializing external/vcpkg, creating build-local vcpkg_root, and bootstrapping vcpkg")
        initialize_vcpkg_submodule_and_bootstrap()

        if (EXISTS ${VCPKG_TOOLCHAIN_FILE_PATH})
            set (CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN_FILE_PATH} CACHE STRING "Vcpkg toolchain file")
        else()
            message(FATAL_ERROR "Failed: ${VCPKG_TOOLCHAIN_FILE_PATH} not found after bootstrap-vcpkg.sh")
            return()
        endif()
    endif()
endif()

# Run vcpkg install only when needed.
set(VCPKG_STATUS_FILE "${CMAKE_BINARY_DIR}/vcpkg_installed/vcpkg/status")
set(VCPKG_MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
set(VCPKG_CONFIGURATION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-configuration.json")
set(RUN_VCPKG_INSTALL OFF)

if (NOT EXISTS ${VCPKG_STATUS_FILE})
    set(RUN_VCPKG_INSTALL ON)
elseif (EXISTS ${VCPKG_MANIFEST_FILE} AND ${VCPKG_MANIFEST_FILE} IS_NEWER_THAN ${VCPKG_STATUS_FILE})
    set(RUN_VCPKG_INSTALL ON)
elseif (EXISTS ${VCPKG_CONFIGURATION_FILE} AND ${VCPKG_CONFIGURATION_FILE} IS_NEWER_THAN ${VCPKG_STATUS_FILE})
    set(RUN_VCPKG_INSTALL ON)
endif()

if (RUN_VCPKG_INSTALL)
    message(STATUS "Running vcpkg install (first run or manifest changed).")
    execute_process(
        COMMAND ${VCPKG_EXECUTABLE_PATH} install
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VCPKG_INSTALL_RESULT
    )
    if(NOT VCPKG_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed: vcpkg install (exit code: ${VCPKG_INSTALL_RESULT})")
        return()
    endif()
else()
    message(STATUS "Skipping initial vcpkg install (dependencies already up to date).")
endif()

cmake_minimum_required(VERSION 3.21)
project(kompli)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

set(MAJOR_VERSION "1" CACHE STRING "Major version")
set(MINOR_VERSION "0" CACHE STRING "Minor version")
set(PATCH_VERSION "5" CACHE STRING "Patch version")

# External Projects
set(ONEDS_CPP_SDK_VERSION "v3.9.309.1" CACHE STRING "1DS CPP SDK version")

option(BUILD_ADAPTERS "Build Kompli Adapters" ON)
option(BUILD_MODULES "Build Kompli Modules" ON)
option(BUILD_TESTS "Build test collateral" ON)
option(BUILD_MODULETEST "Build the moduletest tool" ON)
option(BUILD_TELEMETRY "Build telemetry" ON)
option(BUILD_SAMPLES "Build samples" OFF)
option(COVERAGE "Enable code coverage" OFF)
option(BUILD_FUZZER "Build fuzzer" OFF)
option(BUILD_COMPLIANCE_ENGINE_ASSESSOR "Build the compliance engine assessor tool" ON)

add_compile_options("-Wno-psabi;-fPIC")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Special Ubuntu 14/GNU 4.8 C compiler build configuration for the Universal NRP adapter
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.7" AND CMAKE_C_COMPILER_VERSION VERSION_LESS "4.8.5")
        set(BUILD_TESTS OFF)
    endif()
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5" AND NOT VCPKG_OVERLAY_PORTS MATCHES "src/ports-overlay$")
    message(FATAL_ERROR "GCC version less than 5.5 detected, set VCPKG_OVERLAY_PORTS to '${CMAKE_CURRENT_SOURCE_DIR}/src/ports-overlay'. eg 'cmake ... -DVCPKG_OVERLAY_PORTS=${CMAKE_CURRENT_SOURCE_DIR}/ports-overlay'")
endif()

if(BUILD_TELEMETRY)
    message(STATUS "Telemetry: Enabled")
    if(NOT DEFINED ENV{KompliTelemetryApiKey} OR "$ENV{KompliTelemetryApiKey}" STREQUAL "")
        message(WARNING "Telemetry enabled but missing KompliTelemetryApiKey environment variable")
    endif()
    add_compile_definitions(BUILD_TELEMETRY=1)
else()
    message(STATUS "Telemetry: Disabled")
    set(BUILD_TELEMETRY OFF)
endif()

# Defaults to /usr/local - systemd requires binaries in /usr/(bin)
set(CMAKE_INSTALL_PREFIX /usr)
set(CMAKE_TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/")

function(get_linux_lsb_release_information)
    find_program(LSB_RELEASE_EXEC lsb_release)
    if(LSB_RELEASE_EXEC)
        execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --id OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --release OUTPUT_VARIABLE LSB_RELEASE_VERSION_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --codename OUTPUT_VARIABLE LSB_RELEASE_CODENAME_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)

        set(LSB_RELEASE_ID_SHORT "${LSB_RELEASE_ID_SHORT}" PARENT_SCOPE)
        set(LSB_RELEASE_VERSION_SHORT "${LSB_RELEASE_VERSION_SHORT}" PARENT_SCOPE)
        set(LSB_RELEASE_CODENAME_SHORT "${LSB_RELEASE_CODENAME_SHORT}" PARENT_SCOPE)
    endif()
endfunction()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    get_linux_lsb_release_information()
    if(DEFINED LSB_RELEASE_ID_SHORT)
        message(STATUS "Distro: Linux ${LSB_RELEASE_ID_SHORT} ${LSB_RELEASE_VERSION_SHORT} ${LSB_RELEASE_CODENAME_SHORT}")
    else()
        message(STATUS "Distro: Linux - Unknown")
    endif()
endif()

project(kompli)

set(CMAKE_PROJECT_VERSION_MAJOR ${MAJOR_VERSION})
set(CMAKE_PROJECT_VERSION_MINOR ${MINOR_VERSION})
set(CMAKE_PROJECT_VERSION_PATCH ${PATCH_VERSION})

find_package(Git QUIET)

if(TWEAK_VERSION)
    set(CMAKE_PROJECT_VERSION_TWEAK "${TWEAK_VERSION}")
else()
    string(TIMESTAMP _timestamp "%Y%m%d")
    set(CMAKE_PROJECT_VERSION_TWEAK "${_timestamp}")
endif()

execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT_ID
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

set(KompliProjectName "kompli")
set(KompliProjectLongName "Microsoft Kompli")
set(KompliProjectDescription "Kompli is an open, Linux-focused security compliance project that evaluates and enforces configuration baselines through a modular engine designed to integrate with Azure, and local device management.")
set(KompliProjectVendor "Microsoft")
set(KompliProjectEmail "kompli@microsoft.com")
set(KompliProjectUrl "https://github.com/microsoft/kompli/")
set(KompliHashAlgorithm "SHA256")
set(KompliVersionString ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}.${CMAKE_PROJECT_VERSION_TWEAK})
set(KompliVersionStringFull "${KompliVersionString}-g${GIT_COMMIT_ID}")
set(KompliRootBinaryDir ${CMAKE_BINARY_DIR})

message(STATUS "${KompliProjectName} v${KompliVersionString}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/../src/common/commonutils/version.h" @ONLY)

# GNUInstallPaths CMAKE_INSTALL_SYSCONFDIR for Debian and Mariner is /usr/etc, change to /etc
message(STATUS "Setting CMAKE_INSTALL_SYSCONFDIR to /etc")
set(CMAKE_INSTALL_SYSCONFDIR "/etc")

if (COVERAGE)
    message(STATUS "Code coverage: Enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
else ()
    message(STATUS "Code coverage: Disabled")
endif()

set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules/inc)
set(COMMON_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")

if (BUILD_TESTS)
    message(STATUS "Google Unit Test: Enabled")
    enable_testing()
    add_compile_options(-D TEST_CODE)
    set (GTEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/gtest-output)
else()
    message(STATUS "Google Unit Test: Disabled")
endif()

if (BUILD_FUZZER)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Fuzzer requires Clang")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer-no-link,address,undefined -g -O1 -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer-no-link,address,undefined -g -O1 -fno-omit-frame-pointer")
    message(STATUS "Fuzzer: Enabled")
    message(STATUS "Modified CXX flags: ${CMAKE_CXX_FLAGS}")
    message(STATUS "Modified C flags: ${CMAKE_C_FLAGS}")
else()
    message(STATUS "Fuzzer: Disabled")
endif()

add_subdirectory(common)
if (BUILD_ADAPTERS)
    add_subdirectory(adapters)
endif()
if (BUILD_MODULES)
    add_subdirectory(modules)
endif()

add_subdirectory(tests)

message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CPACK_PACKAGE_VENDOR ${KompliProjectVendor})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${KompliProjectEmail})

# Common packaging
set(CPACK_PACKAGE_NAME ${KompliProjectName})
set(CPACK_PACKAGE_FILE_NAME ${KompliProjectName}-${KompliVersionString}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_PACKAGE_HOMEPAGE_URL ${KompliProjectUrl})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${KompliProjectLongName})
set(CPACK_PACKAGE_DESCRIPTION ${KompliProjectDescription})
set(CPACK_PACKAGE_VERSION ${KompliVersionString})
set(CPACK_PACKAGE_VENDOR ${KompliProjectVendor})
set(CPACK_PACKAGE_CONTACT ${KompliProjectEmail})
set(CPACK_PACKAGE_CHECKSUM ${KompliHashAlgorithm})

set(CPACK_EXCLUDES
    "${CMAKE_SOURCE_DIR}/reference/"
    "${CMAKE_SOURCE_DIR}/build/")

# Source packaging
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_EXCLUDES})
set(CPACK_VERBATIM_VARIABLES YES)

# RPM packaging - see /devops/rpm for postinst/postun/preun scripts
set(CPACK_RPM_BUILDREQUIRES "systemd, cmake, build-essential, tar, gcc, gtest, gtest-devel, gmock, gmock-devel, curl-devel, openssl")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/changelog")
set(CPACK_RPM_PACKAGE_AUTOREQ 1)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_NAME ${KompliProjectName})
set(CPACK_RPM_PACKAGE_DESCRIPTION ${KompliProjectDescription})
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/postun")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/preun")

# Debian packaging - see /devops/debian for postinst/postrm/prerm scripts
# https://www.debian.org/doc/manuals/maint-guide/
set(CPACK_DEBIAN_EXCLUDE_FROM_AUTO_FILELIST ${CPACK_EXCLUDES})
set(CPACK_DEBIAN_FILE_NAME ${KompliProjectName}_${KompliVersionString}_${LSB_RELEASE_CODENAME_SHORT}_${CMAKE_SYSTEM_PROCESSOR}.deb)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/../devops/debian/postinst;${CMAKE_SOURCE_DIR}/../devops/debian/prerm;${CMAKE_SOURCE_DIR}/../devops/debian/postrm;")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${KompliProjectEmail})
set(CPACK_DEBIAN_PACKAGE_NAME ${KompliProjectName})
set(CPACK_DEBIAN_PACKAGE_PREDEPENDS "")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "aziot-identity-service (>= 1.2.0)")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
