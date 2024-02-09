# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;>")

if (CMAKE_COMPILER_IS_GNUCC)
    # GCC 5.3.0 and later support -Wunused-const-variable
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3.0")
        add_compile_options("-Wunused-const-variable=2")
    endif()
endif()

if (BUILD_TESTS)
    add_compile_options(-D TEST_CODE)
    add_subdirectory(tests)
endif()

add_subdirectory(logging)
add_subdirectory(commonutils)
add_subdirectory(mpiclient)
add_subdirectory(parson)