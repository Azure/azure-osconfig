# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;-Wunused-const-variable=2;-Wunused-macros>")

add_subdirectory(test)

find_program(JQ_EXEC jq)
find_program(JSONSCHEMA_EXEC jsonschema)

# Function add_module/add_module_custom_target is used to add a module to the build and add
# the appropriate dependencies in order for module testrecipe configuration to be generated.
function(add_module_custom_target moduleDir targetName mimName recipeName)
    message(STATUS "Adding module '${moduleDir}' to the build.")
    string(TOLOWER ${moduleDir} _moduleDir)
    string(TOLOWER ${mimName} _mimName)
    string(TOLOWER ${targetName} _targetName)
    # Check for MIM
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mim/${_mimName}.json)
        # Validate MIM
        if(JSONSCHEMA_EXEC)
            execute_process(
                COMMAND ${JSONSCHEMA_EXEC} --instance ${CMAKE_CURRENT_SOURCE_DIR}/mim/${_mimName}.json ${CMAKE_CURRENT_SOURCE_DIR}/schema/mim.schema.json
                ERROR_VARIABLE JSONSCHEMA_EXEC_ERROR
            )
            if(JSONSCHEMA_EXEC_ERROR)
                message(WARNING "MIM '${_mimName}' is not valid: \n${JSONSCHEMA_EXEC_ERROR}")
            endif()
        else()
            message(WARNING "jsonschema tool not installed. Cannot validate MIM.")
        endif()
        # Check for Recipe
        if(NOT(JQ_EXEC))
            message(WARNING "Unable to find jq tool. Cannot generate test recipe metadata for '${moduleDir}'.")
            add_subdirectory(${_moduleDir})
        elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/recipes/${recipeName}Tests.json)
            add_subdirectory(${_moduleDir})
            add_dependencies(modulerecipeconfiguration ${_targetName})
        else()
            message(FATAL_ERROR "Module ${moduleDir} must have a recipe file at: ${CMAKE_CURRENT_SOURCE_DIR}/test/recipes/${recipeName}Tests.json")
        endif()
    else()
        message(FATAL_ERROR "Module ${moduleDir} must have a MIM file at: ${CMAKE_CURRENT_SOURCE_DIR}/mim/${_mimName}.json.")
    endif()
endfunction()
function(add_module moduleDir)
    add_module_custom_target(${moduleDir} ${moduleDir} ${moduleDir} ${moduleDir})
endfunction()
function(add_module_custom_name moduleDir targetName mimName recipeName)
    add_module_custom_target(${moduleDir} ${targetName} ${mimName} ${recipeName})
endfunction()

set(MODULES_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(MODULES_INSTALL_DIR "/usr/lib/osconfig" CACHE FILEPATH "Directory used for installing modules")

add_module(CommandRunner)
add_module(DeviceInfo)
add_module(Firewall)
add_module(HostName)
add_module(Networking)
add_module(Pmc)
add_module(Settings)
add_module(Tpm)
add_module(Ztsi)

if (BUILD_SAMPLES)
    add_module_custom_target(Samples cppsample Sample Sample)
endif()