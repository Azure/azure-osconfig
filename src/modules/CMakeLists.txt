# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(MODULES_INSTALL_DIR "/usr/lib/osconfig" CACHE FILEPATH "Directory used for installing modules")

add_subdirectory(commandrunner)
add_subdirectory(firewall)
add_subdirectory(networking)
add_subdirectory(settings)
add_subdirectory(hostname)
add_subdirectory(tpm)
add_subdirectory(ztsi)

if (BUILD_SAMPLES)
    add_subdirectory(samples)
endif()