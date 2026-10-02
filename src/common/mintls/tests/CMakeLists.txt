# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)
add_executable(mintlstests MinTlsUT.cpp)
target_link_libraries(mintlstests PRIVATE mintls GTest::gtest_main pthread rt)
target_include_directories(mintlstests PRIVATE ../core/include)
target_compile_definitions(mintlstests PRIVATE MBEDTLS_CONFIG_FILE="MinTlsConfig.h")
gtest_discover_tests(mintlstests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
