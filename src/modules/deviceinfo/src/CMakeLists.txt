# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-nonnull;-Wno-unused-result;-Wunused-const-variable=2;-Wunused-macros>")

add_subdirectory(lib)
add_subdirectory(so)