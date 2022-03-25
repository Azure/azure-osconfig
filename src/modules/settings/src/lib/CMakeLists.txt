# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(settingslib)

#add_subdirectory(configfileutils)
add_library(settingslib STATIC Settings.cpp)
set_property(TARGET settingslib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(settingslib PRIVATE logging configfileutils commonutils)
target_include_directories(settingslib
    PUBLIC
        ${MODULES_INC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)