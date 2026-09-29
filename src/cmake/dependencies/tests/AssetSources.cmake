# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)

# Never use the caller's real mirrors or credentials; these tests make no requests.
unset(ENV{X_VCPKG_ASSET_SOURCES})
unset(ENV{OSCONFIG_ASSET_SOURCES})
if(FAILURE_CASE STREQUAL "blocked-without-mirror")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-block-origin")
elseif(FAILURE_CASE STREQUAL "unsupported-source")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets/;x-script,unsupported")
elseif(FAILURE_CASE STREQUAL "unsupported-auth")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets/,token=example")
elseif(FAILURE_CASE STREQUAL "insecure-mirror")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,http://mirror.invalid/assets/;x-block-origin")
elseif(FAILURE_CASE STREQUAL "missing-trailing-slash")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets;x-block-origin")
elseif(FAILURE_CASE STREQUAL "conflicting-settings")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets/;x-block-origin")
    set(ENV{OSCONFIG_ASSET_SOURCES} "")
endif()
if(ENVIRONMENT_CASE STREQUAL "new" OR ENVIRONMENT_CASE STREQUAL "both")
    set(ENV{OSCONFIG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets/;x-block-origin")
endif()
if(ENVIRONMENT_CASE STREQUAL "legacy" OR ENVIRONMENT_CASE STREQUAL "both")
    set(ENV{X_VCPKG_ASSET_SOURCES} "x-azurl,https://mirror.invalid/assets/;x-block-origin")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/../AssetSources.cmake")
if(DEFINED FAILURE_CASE)
    message(FATAL_ERROR "Invalid asset source was accepted")
endif()
if(DEFINED ENVIRONMENT_CASE)
    if(NOT OSCONFIG_ASSET_ORIGIN_BLOCKED OR
        NOT OSCONFIG_ASSET_MIRRORS STREQUAL "https://mirror.invalid/assets/")
        message(FATAL_ERROR "Environment did not preserve the mirror-only configuration")
    endif()
    return()
endif()

function(expect_download sources expected_urls)
    osconfig_parse_asset_sources("${sources}" OSCONFIG_ASSET_MIRRORS OSCONFIG_ASSET_ORIGIN_BLOCKED)
    osconfig_asset_download(actual "https://origin.invalid/source.tar.gz" "${hash}")
    set(expected URL ${expected_urls} URL_HASH "SHA512=${hash}"
        DOWNLOAD_NAME source.tar.gz TLS_VERIFY TRUE)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected asset download arguments: ${actual}")
    endif()
endfunction()

string(REPEAT a 128 hash)
expect_download("" "https://origin.invalid/source.tar.gz")
expect_download("x-azurl,https://mirror.invalid/assets/"
    "https://mirror.invalid/assets/${hash};https://origin.invalid/source.tar.gz")
expect_download("x-azurl,https://mirror.invalid/assets/;x-block-origin"
    "https://mirror.invalid/assets/${hash}")
expect_download("x-block-origin;x-azurl,https://mirror.invalid/assets/,,read"
    "https://mirror.invalid/assets/${hash}")
expect_download("x-azurl,https://first.invalid/assets/;x-azurl,https://second.invalid/assets/;x-block-origin"
    "https://first.invalid/assets/${hash};https://second.invalid/assets/${hash}")

foreach(case IN ITEMS blocked-without-mirror unsupported-source unsupported-auth insecure-mirror missing-trailing-slash conflicting-settings)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DFAILURE_CASE=${case}" -P "${CMAKE_CURRENT_LIST_FILE}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(result STREQUAL "0")
        message(FATAL_ERROR "${case}: expected rejection")
    endif()
    if(case STREQUAL "blocked-without-mirror")
        set(expected_error "Origin downloads are blocked")
    elseif(case STREQUAL "conflicting-settings")
        set(expected_error "conflict; configure only one")
    else()
        set(expected_error "Unsupported asset-source setting")
    endif()
    if(NOT "${error}" MATCHES "${expected_error}")
        message(FATAL_ERROR "${case}: unexpected failure: ${output}${error}")
    endif()
endforeach()

foreach(case IN ITEMS new legacy both)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DENVIRONMENT_CASE=${case}" -P "${CMAKE_CURRENT_LIST_FILE}"
        COMMAND_ERROR_IS_FATAL ANY)
endforeach()
