# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wall;-Wextra;-Wunused;-Werror;-Wformat;-Wformat-security;-Wno-unused-result")

add_subdirectory(ssh)
add_subdirectory(asb)
# TODO: Re-add once ffaa61254a1ab80ec98f6fe8bf5f1f9fb42335d5 is re-added
# add_subdirectory(complianceengine)
