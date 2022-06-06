# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(valid_module_v2)

set(VALID_MODULE_V2_PATH "${TEST_MODULES_DIR}/${PROJECT_NAME}.so" CACHE FILEPATH "Path to the valid_module_v2.so module")

add_library(valid_module_v2 SHARED ValidModule.cpp)
target_include_directories(valid_module_v2 PUBLIC "${CMAKE_SOURCE_DIR}/modules/inc")

target_link_libraries(valid_module_v2 commonutils)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD VERBATIM
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${VALID_MODULE_V2_PATH}"
    COMMENT "Copying '${PROJECT_NAME}' to test module staging directory '${VALID_MODULE_V2_PATH}'"
)
