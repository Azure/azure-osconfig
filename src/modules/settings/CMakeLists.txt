# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (BUILD_TESTS)
    add_compile_options(-D TEST_CODE)
    add_subdirectory(tests)
endif()
add_subdirectory(src)