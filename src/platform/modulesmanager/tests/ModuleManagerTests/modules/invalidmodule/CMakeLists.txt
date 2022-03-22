# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(invalid_module)

set(INVALID_MODULE_PATH "${TEST_MODULES_DIR}/${PROJECT_NAME}.so" CACHE FILEPATH "Path to the invalid_module.so module")

add_library(invalid_module SHARED InvalidModule.cpp)
target_include_directories(invalid_module PUBLIC "${CMAKE_SOURCE_DIR}/modules/inc")

target_link_libraries(invalid_module commonutils)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD VERBATIM
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${INVALID_MODULE_PATH}"
    COMMENT "Copying '${PROJECT_NAME}' to test module staging directory '${INVALID_MODULE_PATH}'"
)
