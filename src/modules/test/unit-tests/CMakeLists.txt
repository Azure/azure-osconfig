# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(testfactorytests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(testfactorytests MimParserTests.cpp TestRecipeParserTests.cpp)
target_include_directories(testfactorytests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(testfactorytests gtest gtest_main pthread testfactorylib)
gtest_discover_tests(testfactorytests)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../mim/sample.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/mim/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/recipes/test.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/recipes/)