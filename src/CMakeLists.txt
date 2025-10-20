# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# VCPKG integration
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VCPKG_ROOT})
        set (CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
        set (VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/vcpkg_installed")
        message (STATUS "VCPKG_ROOT found, using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/scripts/buildsystems/vcpkg.cmake)
            set (CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
            message (STATUS "VCPKG_ROOT not found, using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
        else()
            message (ERROR "Ensure git submodules are initialized or VCPKG_ROOT is set and install the vcpkg ports")
        endif()
    endif()
endif()

cmake_minimum_required(VERSION 3.16)
project(osconfig)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

set(MAJOR_VERSION "1" CACHE STRING "Major version")
set(MINOR_VERSION "0" CACHE STRING "Minor version")
set(PATCH_VERSION "5" CACHE STRING "Patch version")

# External Projects
set(ONEDS_CPP_SDK_VERSION "3.9.22.1" CACHE STRING "1DS CPP SDK version")

option(BUILD_ADAPTERS "Build OSConfig Adapters" ON)
option(BUILD_MODULES "Build OSConfig Modules" ON)
option(BUILD_PLATFORM "Build OSConfig Platform" ON)
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
        set(BUILD_PLATFORM OFF)
        set(BUILD_TESTS OFF)
    endif()
endif()

if(BUILD_TELEMETRY)
    message(STATUS "Telemetry: Enabled")
    if(NOT DEFINED ENV{OsConfigTelemetryApiKey} OR "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
        message(WARNING "Telemetry enabled but missing OsConfigTelemetryApiKey environment variable")
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

project(osconfig)

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

set(OsConfigProjectName "osconfig")
set(OsConfigProjectLongName "Azure OSConfig")
set(OsConfigProjectDescription "Azure OSConfig is a modular security configuration stack for Linux Edge devices that supports multi-authority device management over Azure, GitOps, as well as local management.")
set(OsConfigProjectVendor "Microsoft")
set(OsConfigProjectEmail "osconfigcore@microsoft.com")
set(OsConfigProjectUrl "https://github.com/Azure/azure-osconfig/")
set(OsConfigHashAlgorithm "SHA256")
set(OsConfigVersionString ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}.${CMAKE_PROJECT_VERSION_TWEAK})
set(OsConfigVersionStringFull "${OsConfigVersionString}-g${GIT_COMMIT_ID}")
set(OsConfigRootBinaryDir ${CMAKE_BINARY_DIR})

message(STATUS "${OsConfigProjectName} v${OsConfigVersionString}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/../src/common/commonutils/version.h" @ONLY)

# GNUInstallPaths CMAKE_INSTALL_SYSCONFDIR for Debian and Mariner is /usr/etc, change to /etc
message(STATUS "Setting CMAKE_INSTALL_SYSCONFDIR to /etc")
set(CMAKE_INSTALL_SYSCONFDIR "/etc")

if (COVERAGE)
    message(STATUS "Code coverage: Enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CTEST_CUSTOM_COVERAGE_EXCLUDE "./adapters/pnp/")
else ()
    message(STATUS "Code coverage: Disabled")
endif()

set(PLATFORM_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform/inc)
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
    add_subdirectory(tests)
else()
    message(STATUS "Fuzzer: Disabled")
endif()
add_subdirectory(common)
if (BUILD_ADAPTERS)
    add_subdirectory(adapters)
endif()
if (BUILD_PLATFORM)
    add_subdirectory(platform)
endif()
if (BUILD_MODULES)
    add_subdirectory(modules)
endif()

if (BUILD_TELEMETRY)
    ExternalProject_Get_Property(telemetry-external BINARY_DIR)
    set(TELEMETRY_TESTS_DIR ${BINARY_DIR}/lib/tests)
    set(TELEMETRY_BIN_TESTS_DIR ${BINARY_DIR}/bin/tests)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/ExternalTests.cmake.in
        ${CMAKE_BINARY_DIR}/ExternalTests.cmake
        @ONLY)

    set_property(DIRECTORY APPEND PROPERTY
        TEST_INCLUDE_FILES
            ${CMAKE_BINARY_DIR}/ExternalTests.cmake)
endif()

message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CPACK_PACKAGE_VENDOR ${OsConfigProjectVendor})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${OsConfigProjectEmail})

# Common packaging
set(CPACK_PACKAGE_NAME ${OsConfigProjectName})
set(CPACK_PACKAGE_HOMEPAGE_URL ${OsConfigProjectUrl})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${OsConfigProjectLongName})
set(CPACK_PACKAGE_DESCRIPTION ${OsConfigProjectDescription})
set(CPACK_PACKAGE_VERSION ${OsConfigVersionString})
set(CPACK_PACKAGE_VENDOR ${OsConfigProjectVendor})
set(CPACK_PACKAGE_CONTACT ${OsConfigProjectEmail})
set(CPACK_PACKAGE_CHECKSUM ${OsConfigHashAlgorithm})

set(CPACK_EXCLUDES
    "${CMAKE_SOURCE_DIR}/adapters/pnp/azure-iot-sdk-c/"
    "${CMAKE_SOURCE_DIR}/reference/"
    "${CMAKE_SOURCE_DIR}/build/")

# Source packaging
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_EXCLUDES})
set(CPACK_VERBATIM_VARIABLES YES)

# RPM packaging - see /devops/rpm for postinst/postun/preun scripts
set(CPACK_RPM_BUILDREQUIRES "systemd, cmake, build-essential, tar, gcc, gtest, gtest-devel, gmock, gmock-devel, curl-devel, openssl, rapidjson")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/changelog")
set(CPACK_RPM_PACKAGE_AUTOREQ 1)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_NAME ${OsConfigProjectName})
set(CPACK_RPM_PACKAGE_DESCRIPTION ${OsConfigProjectDescription})
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/postun")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/../devops/rpm/preun")

# Debian packaging - see /devops/debian for postinst/postrm/prerm scripts
# https://www.debian.org/doc/manuals/maint-guide/
set(CPACK_DEBIAN_EXCLUDE_FROM_AUTO_FILELIST ${CPACK_EXCLUDES})
set(CPACK_DEBIAN_FILE_NAME ${OsConfigProjectName}_${OsConfigVersionString}_${LSB_RELEASE_CODENAME_SHORT}_${CMAKE_SYSTEM_PROCESSOR}.deb)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/../devops/debian/postinst;${CMAKE_SOURCE_DIR}/../devops/debian/prerm;${CMAKE_SOURCE_DIR}/../devops/debian/postrm;")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${OsConfigProjectEmail})
set(CPACK_DEBIAN_PACKAGE_NAME ${OsConfigProjectName})
set(CPACK_DEBIAN_PACKAGE_PREDEPENDS "")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "aziot-identity-service (>= 1.2.0)")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
