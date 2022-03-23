# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(osinfotests)

include(CTest)
find_package(GTest REQUIRED)

add_executable(osinfotests OsInfoTests.cpp)
target_link_libraries(osinfotests gtest gtest_main pthread osinfolib logging commonutils)

gtest_discover_tests(osinfotests)