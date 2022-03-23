# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(configfileutils)

if (BUILD_TESTS)
    add_compile_options(-D TEST_CODE)
    add_library(configfileutils STATIC ConfigFileUtils.cpp BaseUtils.cpp JsonUtils.cpp TomlUtils.cpp TestingUtils.cpp)
else ()
    add_library(configfileutils STATIC ConfigFileUtils.cpp BaseUtils.cpp JsonUtils.cpp TomlUtils.cpp)
endif ()

set_property(TARGET configfileutils PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(configfileutils
    PRIVATE
        commonutils
)
target_compile_options(configfileutils PRIVATE -Wno-psabi)
target_include_directories(configfileutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)