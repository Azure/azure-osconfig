# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Chain-load the GCC-5 toolchain for compatibility with 1DS SDK on Ubuntu 14
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../cmake/toolchains/linux-gcc-5.cmake")
