# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(ce-lua-evaluator)
set(target_name ce-lua-evaluator)

set(SOURCES
    Main.cpp
)

add_executable(${target_name} ${SOURCES})

target_include_directories(${target_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MODULES_INC_DIR}
    )

target_link_libraries(${target_name}
    ${CMAKE_DL_LIBS}
    logging
    commonutils
    parsonlib
    complianceenginelib
    )

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
