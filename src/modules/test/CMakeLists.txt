# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(testfactorylib)

set(CMAKE_CXX_STANDARD 17)
add_library(testfactorylib MimParser.cpp TestRecipeParser.cpp ManagementModule.cpp RecipeInvoker.cpp)

target_link_libraries(testfactorylib
    ${CMAKE_DL_LIBS}
    commonutils
    pthread
    parsonlib)

target_include_directories(testfactorylib PUBLIC
    ${MODULES_INC_DIR}
    ${PLATFORM_INC_DIR})

add_executable(modulestest main.cpp)
target_link_libraries(modulestest gtest gtest_main pthread testfactorylib)

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
    target_link_libraries(modulestest stdc++fs)
endif()

if (BUILD_TESTS)
    add_subdirectory(unit-tests)
endif()