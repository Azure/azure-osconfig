# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.2)
set (CMAKE_CXX_STANDARD 11)
project(commandrunner)

add_library(commandrunner SHARED CommandRunnerModule.cpp)
target_link_libraries(commandrunner
    PRIVATE
        logging
        commonutils
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
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../assets/osconfig_commandrunner.cache DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)