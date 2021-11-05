# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

project(multicomponentmodule)
add_library(multicomponentmodule SHARED MultiComponentModule.cpp)
target_include_directories(multicomponentmodule PUBLIC ${MODULES_INC_DIR})
target_link_libraries(multicomponentmodule commonutils)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD VERBATIM
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${TEST_MODULE_STAGING_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMENT "Copying '${PROJECT_NAME}' to test module staging directory '${TEST_MODULE_STAGING_DIR}'"
)