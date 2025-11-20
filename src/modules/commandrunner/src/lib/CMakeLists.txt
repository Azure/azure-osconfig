# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunnerlib)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(commandrunnerlib STATIC CommandRunner.cpp Command.cpp)
target_link_libraries(commandrunnerlib PRIVATE logging commonutils nlohmann_json::nlohmann_json)
target_include_directories(commandrunnerlib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(commandrunnerlib PUBLIC -fsigned-char)
set_target_properties(commandrunnerlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
