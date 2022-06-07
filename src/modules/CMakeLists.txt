# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(test)

# Function add_module/add_module_custom_target is used to add a module to the build and add
# the appropriate dependencies in order for module testrecipe configuration to be generated. 
function(add_module_custom_target moduleDir targetName)
    add_subdirectory(${moduleDir})
    add_dependencies(modulerecipeconfiguration ${targetName})
endfunction()
function(add_module moduleDir)
    add_module_custom_target(${moduleDir} ${moduleDir})
endfunction()

set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(MODULES_INSTALL_DIR "/usr/lib/osconfig" CACHE FILEPATH "Directory used for installing modules")

add_module(commandrunner)
add_module(deviceinfo)
add_module(firewall)
add_module(hostname)
add_module(networking)
add_module(pmc)
add_module(settings)
add_module(tpm)
add_module(ztsi)

if (BUILD_SAMPLES)
    add_module_custom_target(samples cppsample)
endif()