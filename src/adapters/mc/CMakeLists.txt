# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (ENABLE_TELEMETRY)
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
    endif()
    ExternalProject_Add(
        telemetry
        INSTALL_COMMAND ""  # No install target on telemetry target
        SOURCE_DIR
            ${COMMON_SRC_DIR}/telemetry
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}  # use the same toolchain
            ${CMAKE_ARGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_TESTS=${BUILD_TESTS}
            -DGTEST_OUTPUT_DIR=${GTEST_OUTPUT_DIR}
            -DONEDS_CPP_SDK_VERSION=${ONEDS_CPP_SDK_VERSION}
    )
    ExternalProject_Get_Property(telemetry BINARY_DIR)
endif()

#[[
Adds a Machine Configuration package (.zip)

Parameters:
    TARGET_NAME - The CMake target name to be used for this package
    MOF_FILE - Path to the MOF file containing the managed object definitions
    META_CONFIG_JSON_FILE - Path to the JSON file containing metadata configuration
    DESTINATION_FILE - The filename for the generated MC package (not including .zip)
    ADDITIONAL_COMMANDS_GET - Optional list of additional COMMAND entries to be added to the GET staging target
    ADDITIONAL_COMMANDS_SET - Optional list of additional COMMAND entries to be added to the SET staging target
    ADDITIONAL_DEPENDS - Optional list of additional dependencies for the staging target

Example:
    add_mc_package(
        TARGET_NAME my_package_target
        MOF_FILE "${CMAKE_SOURCE_DIR}/config/package.mof"
        META_CONFIG_JSON_FILE "${CMAKE_SOURCE_DIR}/config/metadata.json"
        DESTINATION_FILE "my_package"
        ADDITIONAL_COMMANDS_GET
            COMMAND ${CMAKE_COMMAND} -E copy_if_different source dest
        ADDITIONAL_COMMANDS_SET
            COMMAND ${CMAKE_COMMAND} -E copy_if_different source dest
        ADDITIONAL_DEPENDS
            some_other_target
    )
#]]
function(add_mc_package)
    set(options "")
    set(oneValueArgs TARGET_NAME MOF_FILE META_CONFIG_JSON_FILE DESTINATION_FILE)
    set(multiValueArgs ADDITIONAL_COMMANDS_GET ADDITIONAL_COMMANDS_SET ADDITIONAL_DEPENDS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Check required arguments
    if(NOT ARG_TARGET_NAME)
        message(FATAL_ERROR "add_mc_package: TARGET_NAME is required")
    endif()

    if(NOT ARG_MOF_FILE)
        message(FATAL_ERROR "add_mc_package: MOF_FILE is required")
    endif()

    if(NOT ARG_META_CONFIG_JSON_FILE)
        message(FATAL_ERROR "add_mc_package: META_CONFIG_JSON_FILE is required")
    endif()

    if(NOT ARG_DESTINATION_FILE)
        message(FATAL_ERROR "add_mc_package: DESTINATION_FILE is required")
    endif()

    if(NOT TARGET ${ARG_TARGET_NAME})
        message(FATAL_ERROR "add_mc_package: '${ARG_TARGET_NAME}' is not a valid CMake target")
    endif()

    if(NOT EXISTS "${ARG_MOF_FILE}")
        message(FATAL_ERROR "add_mc_package: MOF file '${ARG_MOF_FILE}' does not exist")
    endif()

    if(NOT EXISTS "${ARG_META_CONFIG_JSON_FILE}")
        message(FATAL_ERROR "add_mc_package: Meta config JSON file '${ARG_META_CONFIG_JSON_FILE}' does not exist")
    endif()

    set_target_properties(${ARG_TARGET_NAME} PROPERTIES OUTPUT_NAME libOsConfigResource)

    # Build the command lists
    set(STAGING_COMMANDS_GET
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/GetStaging
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARG_META_CONFIG_JSON_FILE}" ${PROJECT_BINARY_DIR}/GetStaging/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARG_MOF_FILE}" ${PROJECT_BINARY_DIR}/GetStaging/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${ARG_TARGET_NAME}> ${PROJECT_BINARY_DIR}/GetStaging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    )
    set(STAGING_COMMANDS_SET
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/SetStaging
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARG_META_CONFIG_JSON_FILE}" "${PROJECT_BINARY_DIR}/SetStaging/Set${ARG_DESTINATION_FILE}.metaconfig.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARG_MOF_FILE}" "${PROJECT_BINARY_DIR}/SetStaging/Set${ARG_DESTINATION_FILE}.mof"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${ARG_TARGET_NAME}> ${PROJECT_BINARY_DIR}/SetStaging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    )

    # Add telemetry binary if enabled
    if(ENABLE_TELEMETRY)
        list(APPEND STAGING_COMMANDS_GET
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BINARY_DIR}/exe/telemetry" ${PROJECT_BINARY_DIR}/GetStaging/Modules/DscNativeResources/OsConfigResource/telemetry
        )
        list(APPEND STAGING_COMMANDS_SET
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BINARY_DIR}/exe/telemetry" ${PROJECT_BINARY_DIR}/SetStaging/Modules/DscNativeResources/OsConfigResource/telemetry
        )
    endif()

    # Add additional commands if provided
    if(ARG_ADDITIONAL_COMMANDS_GET)
        list(APPEND STAGING_COMMANDS_GET ${ARG_ADDITIONAL_COMMANDS_GET})
    endif()
    if(ARG_ADDITIONAL_COMMANDS_SET)
        list(APPEND STAGING_COMMANDS_SET ${ARG_ADDITIONAL_COMMANDS_SET})
    endif()

    # Build dependencies list
    set(STAGING_DEPENDS ${ARG_TARGET_NAME} ${ARG_ADDITIONAL_DEPENDS})
    if(ENABLE_TELEMETRY)
        list(APPEND STAGING_DEPENDS telemetry)
    endif()

    add_custom_target(stage_create_get_${ARG_TARGET_NAME}_zip
        ${STAGING_COMMANDS_GET}
        DEPENDS ${STAGING_DEPENDS})

    add_custom_target(create_get_${ARG_TARGET_NAME}_zip ALL
        BYPRODUCTS ${OsConfigRootBinaryDir}/${ARG_DESTINATION_FILE}.zip
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/${ARG_DESTINATION_FILE}.zip" --format=zip .
        DEPENDS stage_create_get_${ARG_TARGET_NAME}_zip
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/GetStaging/)

    add_custom_target(stage_create_set_${ARG_TARGET_NAME}_zip
        ${STAGING_COMMANDS_SET}
        DEPENDS ${STAGING_DEPENDS})

    add_custom_target(create_${ARG_TARGET_NAME}_zip ALL
        BYPRODUCTS ${OsConfigRootBinaryDir}/${ARG_DESTINATION_FILE}.zip
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/Set${ARG_DESTINATION_FILE}.zip" --format=zip .
        DEPENDS stage_create_set_${ARG_TARGET_NAME}_zip
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/SetStaging/)
endfunction()

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
add_subdirectory(complianceengine)
