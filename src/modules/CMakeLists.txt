# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options("-Wunused-const-variable=2")
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options("-Wunused-variable")
endif()

set(MODULES_INSTALL_DIR "/usr/lib/osconfig" CACHE FILEPATH "Directory used for installing modules")
set(MODULES_BUILD_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE FILEPATH "Directory used to collect module binaries for testing")

file(MAKE_DIRECTORY ${MODULES_BUILD_BIN_DIR})

set(MIM_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/schema/mim.schema.json)
find_program(JSONSCHEMA_EXEC jsonschema)

function(add_module directory)
    get_filename_component(MODULE ${directory} NAME)

    set(MODULE_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mim/${MODULE}.json)

    if (EXISTS ${MODULE_INTERFACE})
        if(JSONSCHEMA_EXEC)
            execute_process(
                COMMAND ${JSONSCHEMA_EXEC} -i ${MODULE_INTERFACE} ${MIM_SCHEMA}
                ERROR_VARIABLE SCHEMA_ERROR
            )
            if(SCHEMA_ERROR)
                message(WARNING "${MODULE_INTERFACE} does not match schema: ${SCHEMA_ERROR}")
            endif()
        endif()
    endif()

    add_subdirectory(${directory})

    # Copy the module shared-object to build/modules/bin for convenience when using moduletest
    set(TARGET_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${directory}/src/so/${MODULE}.so)
    set(TARGET_OUTPUT_COPY ${MODULES_BUILD_BIN_DIR}/${MODULE}.so)

    add_custom_command(
        OUTPUT ${TARGET_OUTPUT_COPY}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MODULE}> ${TARGET_OUTPUT_COPY}
        DEPENDS ${MODULE}
    )

    add_custom_target(${MODULE}_so ALL
        DEPENDS ${TARGET_OUTPUT_COPY}
    )
endfunction()

add_module(commandrunner)
add_module(configuration)
add_module(deviceinfo)
add_module(securitybaseline)

if (BUILD_MODULETEST)
    add_subdirectory(test)
endif()

if (BUILD_SAMPLES)
    add_subdirectory(samples/cpp)
endif()