# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancetests)

include(CTest)
find_package(GTest REQUIRED)

set(PROCEDURES
    procedures/EnsureApparmorProfilesTest.cpp
    procedures/EnsureDconfTest.cpp
    procedures/EnsureFilePermissionsTest.cpp
    procedures/EnsureFilesystemOptionTest.cpp
    procedures/EnsureFirewallOpenPortsTest.cpp
    procedures/EnsureKernelModuleTest.cpp
    procedures/EnsureLogfileAccessTest.cpp
    procedures/EnsureGroupIsOnlyGroupWithTest.cpp
    procedures/EnsureGsettingsTest.cpp
    procedures/EnsureMountPointExistsTest.cpp
    procedures/EnsureMTAsLocalOnlyTest.cpp
    procedures/EnsureNoDuplicateEntriesExistTest.cpp
    procedures/EnsurePasswordChangeIsInPastTest.cpp
    procedures/EnsureRootPathTest.cpp
    procedures/EnsureSshdOptionTest.cpp
    procedures/EnsureSysctlTest.cpp
    procedures/EnsureUserIsOnlyAccountWithTest.cpp
    procedures/EnsureWirelessIsDisabledTest.cpp
    procedures/EnsureXdmcpTest.cpp
    procedures/ExecuteCommandGrepTest.cpp
    procedures/FileRegexMatchTest.cpp
    procedures/PackageInstalledTest.cpp
    procedures/SystemdConfigTest.cpp
    procedures/SystemdUnitStateTest.cpp
    procedures/ufwStatusTest.cpp
    procedures/EnsureShadowContainsTest.cpp
)

add_executable(complianceenginetests
    Base64Test.cpp
    CommonContextTest.cpp
    ComplianceEngineTest.cpp
    EngineTest.cpp
    EvaluatorTest.cpp
    NetworkToolsTest.cpp
    OptionalTest.cpp
    RegexFallbackTest.cpp
    RegexTest.cpp
    ResultTest.cpp
    Base64Test.cpp
    ${PROCEDURES}
)
# for ProcedureMap.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(complianceenginetests
    GTest::gtest
    GTest::gtest_main
    gmock
    gmock_main
    pthread
    complianceenginelib
    logging
    commonutils
    parsonlib
)

gtest_discover_tests(complianceenginetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
