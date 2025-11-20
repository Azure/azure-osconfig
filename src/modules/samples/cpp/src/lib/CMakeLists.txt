# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(cppsamplelib)

find_package(nlohmann_json CONFIG REQUIRED)

add_library(cppsamplelib STATIC Sample.cpp)
target_link_libraries(cppsamplelib
    PRIVATE
        logging
        commonutils
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_include_directories(cppsamplelib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cppsamplelib PUBLIC -fsigned-char)

set_target_properties(cppsamplelib PROPERTIES POSITION_INDEPENDENT_CODE ON)
