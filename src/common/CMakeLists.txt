# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;-Wunused-const-variable=2>")

if (BUILD_TESTS)
    add_compile_options(-D TEST_CODE)
    add_subdirectory(tests)
endif()

add_subdirectory(logging)
add_subdirectory(commonutils)
add_subdirectory(mpiclient)
add_subdirectory(parson)