# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commandrunner)

find_package(nlohmann_json CONFIG REQUIRED)
add_library(commandrunner SHARED CommandRunnerModule.cpp)
target_link_libraries(commandrunner
    PRIVATE
        logging
        commonutils
        nlohmann_json::nlohmann_json
    PUBLIC
        commandrunnerlib
)
target_include_directories(commandrunner
    PUBLIC
        ${MODULES_INC_DIR}
)
set_target_properties(commandrunner
    PROPERTIES
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
)
install(TARGETS commandrunner DESTINATION ${MODULES_INSTALL_DIR})
