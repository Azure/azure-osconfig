# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(src)

option(BUILD_TESTS "Build test collateral" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()