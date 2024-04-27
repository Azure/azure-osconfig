# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security -Wno-nonnull -Wno-unused-result -Wunused-macros)

add_subdirectory(ssh)
add_subdirectory(asb)