# vcpkg Port Overlays

This directory contains vcpkg port overlays that override the default port versions for specific build configurations.

## gtest

The `gtest` overlay provides Google Test version 1.10.0, which is used specifically when building with the `x64-linux-gcc5` triplet for compatibility with older GCC 5 compilers (e.g., Ubuntu 14.04, centos-7, rhel-7. oraclelinux-7).

### Usage

When building with the GCC-5 triplet (See the [triplets concept documentation](https://learn.microsoft.com/vcpkg/users/triplets) for a high-level view of triplet capabilities), add the overlay ports parameter to your CMake command:

```bash
cmake ../src \
  -DVCPKG_OVERLAY_TRIPLETS=../src/triplets \
  -DVCPKG_TARGET_TRIPLET=x64-linux-gcc5 \
  -DVCPKG_OVERLAY_PORTS=../src/ports-overlay
```

For all other builds (modern compilers), omit the `VCPKG_OVERLAY_PORTS` parameter to use the default gtest version specified in `vcpkg.json` (1.12.0).

### Why gtest 1.10.0?

Google Test versions 1.11.0 and later require C++11 features that are not fully supported by GCC 5. Version 1.10.0 is the last release that compiles cleanly with GCC 5.
