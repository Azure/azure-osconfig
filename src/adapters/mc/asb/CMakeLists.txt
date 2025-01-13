# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Azure Security Baseline for Linux
project(OsConfigResourceAsb)

add_library(OsConfigResourceAsb
    SHARED
        ../module.c
        ../schema.c
        ../OsConfigResource.c)

target_link_libraries(OsConfigResourceAsb
    PRIVATE
        commonutils
        logging
        mpiclient
        parsonlib
        asb)

# SO binary name must match the universal class name in MOF with a "lib" prefix added
set_target_properties(OsConfigResourceAsb PROPERTIES OUTPUT_NAME OsConfigResource)

# The binary size limit for the SO binary, in bytes 
set(MAX_FILE_SIZE 762,880)
file(WRITE "${PROJECT_BINARY_DIR}/check_filesize.cmake"
    "file(READ filesize.txt actual_file_size)\n"
    "message(STATUS \"SO binary size: " ${actual_file_size} \" bytes\")\n"
    "if(actual_file_size GREATER ${MAX_FILE_SIZE})\n"
        "message(FATAL_ERROR \"SO binary size exceeds the maximum allowed size of ${MAX_FILE_SIZE} bytes\")\n"
    "endif()\n")

# Create the policy artifacts ZIP package for the Azure Security Baseline for Linux
add_custom_target(stage_create_asb_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AzureLinuxBaseline.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceAsb> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    COMMAND ${CMAKE_COMMAND} -E echo "Checking file size of $<TARGET_FILE:OsConfigResourceAsb>"
    COMMAND stat --format=%s $<TARGET_FILE:OsConfigResourceAsb> > ${PROJECT_BINARY_DIR}/filesize.txt
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/check_filesize.cmake
    BYPRODUCTS filesize.txt
    DEPENDS OsConfigResourceAsb)

add_custom_target(create_asb_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/AzureLinuxBaseline.zip" --format=zip .
    DEPENDS stage_create_asb_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)