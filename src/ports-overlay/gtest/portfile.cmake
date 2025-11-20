# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# This is a vcpkg port overlay for Google Test (gtest) version 1.10.0.
#
# This overlay is required for builds using the x64-linux-gcc5 triplet, as GCC 5
# (used on Ubuntu 14.04 for 1DS SDK compatibility) does not fully support the C++11
# features required by gtest 1.11.0 and later versions.
#
# Usage: Add -DVCPKG_OVERLAY_PORTS=../src/ports-overlay to your CMake command when
#        building with the x64-linux-gcc5 triplet to use this version instead of
#        the default gtest version specified in vcpkg.json.
#
# Reference: Based on vcpkg gtest portfile from commit 822c2dde6a (pre-1.11.0 update)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/googletest
    REF release-1.10.0
    SHA512 bd52abe938c3722adc2347afad52ea3a17ecc76730d8d16b065e165bc7477d762bce0997a427131866a89f1001e3f3315198204ffa5d643a9355f1f4d0d7b1a9
    HEAD_REF master
)

string(COMPARE EQUAL "${VCPKG_CRT_LINKAGE}" "dynamic" GTEST_FORCE_SHARED_CRT)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DBUILD_GMOCK=ON
        -DBUILD_GTEST=ON
        -DCMAKE_DEBUG_POSTFIX=d
        -Dgtest_force_shared_crt=${GTEST_FORCE_SHARED_CRT}
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/GTest TARGET_PATH share/GTest)

vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
