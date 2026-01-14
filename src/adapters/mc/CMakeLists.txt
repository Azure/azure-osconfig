# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (BUILD_TELEMETRY)
    include(ExternalProject)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
        find_program(GCC5_FOUND gcc-5)
        if(NOT GCC5_FOUND)
            message(FATAL_ERROR "gcc-5 not found in PATH, but required for 1DS SDK with GCC < 5.5")
        endif()
        find_program(GPP5_FOUND g++-5)
        if(NOT GPP5_FOUND)
            message(FATAL_ERROR "g++-5 not found in PATH, but required for 1DS SDK with GCC < 5.5")
        endif()
        set(CMAKE_ARGS -DCMAKE_C_COMPILER=gcc-5 -DCMAKE_CXX_COMPILER=g++-5)
    elseif(BUILD_FUZZER)
        set(CMAKE_ARGS
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        )
    endif()
    ExternalProject_Add(
        telemetry-external
        INSTALL_COMMAND ""  # No install target on telemetry target
        BUILD_ALWAYS TRUE
        SOURCE_DIR
            ${COMMON_SRC_DIR}/telemetry
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            ${CMAKE_ARGS}
            -DBUILD_EXTERNAL=1
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
            -DVCPKG_INSTALLED_DIR=${CMAKE_BINARY_DIR}/vcpkg_installed
            -DBUILD_TESTS=${BUILD_TESTS}
            -DGTEST_OUTPUT_DIR=${GTEST_OUTPUT_DIR}
            -DONEDS_CPP_SDK_VERSION=${ONEDS_CPP_SDK_VERSION}
    )
    ExternalProject_Get_Property(telemetry-external BINARY_DIR)

    set(TELEMETRY_LIB_DIR ${BINARY_DIR}/lib CACHE PATH "Telemetry library directory" FORCE)
    set(MAT_SDK_LIB ${BINARY_DIR}/1ds_install/lib)
    set(MAT_SDK_INCLUDE ${BINARY_DIR}/1ds_install/include/mat)

    set(VCPKG_PACKAGES_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    find_package(CURL REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
    find_package(OpenSSL REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
    find_package(Threads REQUIRED)
    find_package(unofficial-sqlite3 REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})
    find_package(nlohmann_json REQUIRED NO_DEFAULT_PATH PATHS ${VCPKG_PACKAGES_DIR})

    add_library(telemetry-interface INTERFACE)
    add_dependencies(telemetry-interface telemetry-external)
    target_link_libraries(telemetry-interface
        INTERFACE
            ${BINARY_DIR}/lib/libtelemetrylib.a
            ${MAT_SDK_LIB}/libmat.a
            unofficial::sqlite3::sqlite3
            nlohmann_json::nlohmann_json
            CURL::libcurl
            OpenSSL::SSL
            dl
            ${CMAKE_THREAD_LIBS_INIT}
    )
    target_include_directories(telemetry-interface
        INTERFACE
            ${BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/common/telemetry/lib
            ${MAT_SDK_INCLUDE}
            ${TELEMETRY_LIB_DIR}
    )
endif()

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
add_subdirectory(complianceengine)
