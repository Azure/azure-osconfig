# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancetests)

include(CTest)
find_package(GTest REQUIRED)

set(PROCEDURES
    procedures/AuditdRulesCheckTest.cpp
    procedures/EnsureAccountsWithoutShellAreLockedTest.cpp
    procedures/EnsureApparmorProfilesTest.cpp
    procedures/EnsureDconfTest.cpp
    procedures/EnsureDefaultShellTimeoutIsConfiguredTest.cpp
    procedures/EnsureDefaultUserUmaskIsConfiguredTest.cpp
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
    procedures/EnsureNoWritablesTest.cpp
    procedures/EnsurePasswordChangeIsInPastTest.cpp
        procedures/EnsureNoUnownedTest.cpp
    procedures/EnsureRootPathTest.cpp
    procedures/EnsureSshKeyPermsTest.cpp
    procedures/EnsureSshdOptionTest.cpp
    procedures/EnsureSysctlTest.cpp
    procedures/EnsureSystemAccountsDoNotHaveValidShellTest.cpp
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

# Check that all globbed procedures are in the PROCEDURES list
file(GLOB GLOBBED_PROCEDURES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} procedures/*.cpp)
foreach(proc ${GLOBBED_PROCEDURES})
    if (NOT proc IN_LIST PROCEDURES)
        message(SEND_ERROR "Globbed procedure ${proc} is not in the PROCEDURES list.")
    endif()
endforeach()

add_executable(complianceenginetests
    Base64Test.cpp
    BenchmarkInfoTest.cpp
    BindingsTest.cpp
    CommonContextTest.cpp
    ComplianceEngineTest.cpp
    FilesystemScannerTest.cpp
    DistributionInfoTest.cpp
    EngineTest.cpp
    EvaluatorTest.cpp
    LuaEvaluatorTest.cpp
    LuaProceduresTest.cpp
    NetworkToolsTest.cpp
    OptionalTest.cpp
    RegexFallbackTest.cpp
    RegexTest.cpp
    ResultTest.cpp
    ${PROCEDURES}
)
# for ProcedureMap.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib/procedures)
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
