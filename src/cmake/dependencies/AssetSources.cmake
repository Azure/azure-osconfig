# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include_guard(GLOBAL)

# Read the existing build-environment contract without requiring its former
# package manager. Support unsigned HTTPS read mirrors only; never silently
# ignore an unsupported setting or bypass x-block-origin.
function(osconfig_parse_asset_sources sources mirrors_out blocked_out)
    set(mirrors)
    set(blocked OFF)
    foreach(source IN LISTS sources)
        if(source STREQUAL "")
            continue()
        elseif(source STREQUAL "x-block-origin")
            set(blocked ON)
        elseif(source MATCHES "^x-azurl,(https://[^,;`?#]+/)(,,read)?$")
            list(APPEND mirrors "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR
                "Unsupported asset-source setting. Supported entries are "
                "x-azurl,https://host/container/ (optionally followed by ,,read) "
                "and x-block-origin. No source configuration was ignored.")
        endif()
    endforeach()
    if(blocked AND NOT mirrors)
        message(FATAL_ERROR "Origin downloads are blocked but no readable asset mirror is configured")
    endif()
    set(${mirrors_out} "${mirrors}" PARENT_SCOPE)
    set(${blocked_out} "${blocked}" PARENT_SCOPE)
endfunction()

# Accept the old environment name during the independent pipeline migration.
# Conflicting settings must not silently weaken origin-blocking restrictions.
if(DEFINED ENV{OSCONFIG_ASSET_SOURCES})
    if(DEFINED ENV{X_VCPKG_ASSET_SOURCES} AND
        NOT "$ENV{OSCONFIG_ASSET_SOURCES}" STREQUAL "$ENV{X_VCPKG_ASSET_SOURCES}")
        message(FATAL_ERROR "OSCONFIG_ASSET_SOURCES and X_VCPKG_ASSET_SOURCES conflict; configure only one")
    endif()
    set(_osconfig_asset_sources "$ENV{OSCONFIG_ASSET_SOURCES}")
else()
    set(_osconfig_asset_sources "$ENV{X_VCPKG_ASSET_SOURCES}")
endif()
osconfig_parse_asset_sources("${_osconfig_asset_sources}"
    OSCONFIG_ASSET_MIRRORS OSCONFIG_ASSET_ORIGIN_BLOCKED)
unset(_osconfig_asset_sources)

function(osconfig_asset_download result origin sha512)
    set(urls)
    foreach(mirror IN LISTS OSCONFIG_ASSET_MIRRORS)
        list(APPEND urls "${mirror}${sha512}")
    endforeach()
    if(NOT OSCONFIG_ASSET_ORIGIN_BLOCKED)
        list(APPEND urls "${origin}")
    endif()
    # Hash-addressed mirror URLs have no archive extension.
    get_filename_component(filename "${origin}" NAME)
    set(${result}
        URL ${urls}
        URL_HASH "SHA512=${sha512}"
        DOWNLOAD_NAME "${filename}"
        TLS_VERIFY TRUE
        PARENT_SCOPE)
endfunction()
