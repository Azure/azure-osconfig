# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(platformtests)

set(CMAKE_CXX_STANDARD 14)

include(CTest)
find_package(GTest REQUIRED)

add_executable(platformtests
    ./PlatformTests.cpp
    ../MmiClient.c
    ../ModulesManager.c
    ../MpiServer.c)

target_link_libraries(platformtests
    gtest
    gtest_main
    pthread
    logging
    commonutils
    parsonlib
    ${CMAKE_DL_LIBS})

target_include_directories(platformtests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MODULES_INC_DIR} ${PLATFORM_INC_DIR})

gtest_discover_tests(platformtests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
