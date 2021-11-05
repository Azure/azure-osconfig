# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# GenerateSourceDistribution.cmake
# This CMake script exposes the `generate_source_distribution` function which creates a `dist` target.
# The `dist` target creates the needed files required for building source distributions, see
# GenerateSourceDistribution.cmake for more info.

include(CMakeParseArguments)

function(generate_source_distribution)
    cmake_parse_arguments(
        PARSED_ARGS # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "NAME;LONG_NAME;OUTPUT_DIR" # list of names of mono-valued arguments
        "SOURCES" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    if (NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "NAME must be provided.")
    endif (NOT PARSED_ARGS_NAME)
    if (NOT PARSED_ARGS_LONG_NAME)
        message(FATAL_ERROR "LONG_NAME must be provided.")
    endif (NOT PARSED_ARGS_LONG_NAME)
    if (NOT PARSED_ARGS_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR must be provided.")
    endif (NOT PARSED_ARGS_OUTPUT_DIR)
    if (NOT PARSED_ARGS_SOURCES)
        message(FATAL_ERROR "SOURCES must be provided.")
    endif (NOT PARSED_ARGS_SOURCES)

    set(_NAME ${PARSED_ARGS_NAME})
    set(_LONG_NAME ${PARSED_ARGS_LONG_NAME})
    set(_OUTPUT_DIR ${PARSED_ARGS_OUTPUT_DIR})
    set(_SOURCES ${PARSED_ARGS_SOURCES})

    configure_file(
        ${CMAKE_TEMPLATES_DIR}GenerateSourceDistributionDelegate.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/spec/GenerateSourceDistributionDelegate.cmake
        @ONLY)

    add_custom_target(dist
        COMMAND 
            "${CMAKE_COMMAND}" --build . --target package_source
        COMMAND
            "${CMAKE_COMMAND}" -P ${CMAKE_CURRENT_BINARY_DIR}/spec/GenerateSourceDistributionDelegate.cmake
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building source package with specfile"
        VERBATIM
    )
    
endfunction(generate_source_distribution)