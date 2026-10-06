# vcpkg Overlay Triplets

This directory contains custom [vcpkg triplets](https://learn.microsoft.com/vcpkg/users/triplets) that control how dependencies are built for specific toolchains.

## x64-linux-gcc5

Builds all vcpkg dependencies as static libraries for x64 Linux and chain-loads the GCC 5 toolchain at [`../../../cmake/toolchains/linux-gcc-5.cmake`](../../../cmake/toolchains/linux-gcc-5.cmake). This is required on older distributions whose system compiler is GCC 5 (e.g. Ubuntu 14.04, centos-7, rhel-7, oraclelinux-7), so the 1DS telemetry SDK and its dependencies compile cleanly.

### Usage

Select the triplet with `VCPKG_OVERLAY_TRIPLETS` and `VCPKG_TARGET_TRIPLET`. Because GCC 5 also needs an older Google Test, pair it with the matching port overlay (see [`../ports-overlay`](../ports-overlay)):

```bash
cmake ../src \
  -DVCPKG_OVERLAY_TRIPLETS=../src/common/telemetry/triplets \
  -DVCPKG_TARGET_TRIPLET=x64-linux-gcc5 \
  -DVCPKG_OVERLAY_PORTS=../src/common/telemetry/ports-overlay
```

For modern compilers, omit these parameters to use the default vcpkg triplet.
