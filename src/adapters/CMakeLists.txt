# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Special Ubuntu 14/GNU 4.8 C compiler build configuration for the Universal NRP adapter
if (CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9")
    add_subdirectory(pnp)
endif()
add_subdirectory(mc)
