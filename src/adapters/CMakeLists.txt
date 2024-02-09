# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Exclude pnp for Ubuntu 14/gcc 4.8 build configuration for the Universal NRP adapter
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9")
    add_subdirectory(pnp)
endif()
add_subdirectory(mc)