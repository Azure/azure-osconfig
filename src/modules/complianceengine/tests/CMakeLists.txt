# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_compile_options("-Wno-unused-const-variable")

project(compliancetests)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

set(PROCEDURES
    procedures/AuditdRulesTest.cpp
    procedures/NoShellAccountsLockedTest.cpp
    procedures/ApparmorProfileStateTest.cpp
    procedures/DconfValueTest.cpp
    procedures/ShellTimeoutTest.cpp
    procedures/DefaultUmaskTest.cpp
    procedures/FileExistsTest.cpp
    procedures/FilePermissionsTest.cpp
    procedures/FilesystemMountOptionTest.cpp
    procedures/FirewalldZoneTargetsTest.cpp
    procedures/FirewallOpenPortsTest.cpp
    procedures/KernelModuleTest.cpp
    procedures/LogFilePermissionsTest.cpp
    procedures/LoginDefsOptionTest.cpp
    procedures/UniqueGroupIdTest.cpp
    procedures/GsettingsValueTest.cpp
    procedures/MountPointExistsTest.cpp
    procedures/MtaLocalOnlyTest.cpp
    procedures/NoDuplicateEntriesTest.cpp
    procedures/NoWorldWritableFilesTest.cpp
    procedures/PasswordChangeDateTest.cpp
    procedures/NoUnownedFilesTest.cpp
    procedures/RootPathSecurityTest.cpp
    procedures/SshKeyPermissionsTest.cpp
    procedures/SshdOptionTest.cpp
    procedures/SysctlValueTest.cpp
    procedures/SystemAccountShellTest.cpp
    procedures/UniqueUserIdTest.cpp
    procedures/WirelessDisabledTest.cpp
    procedures/XdmcpDisabledTest.cpp
    procedures/CommandOutputMatchTest.cpp
    procedures/FileRegexMatchTest.cpp
    procedures/PackageInstalledTest.cpp
    procedures/SystemdConfigTest.cpp
    procedures/SystemdUnitStateTest.cpp
    procedures/ufwStatusTest.cpp
    procedures/ShadowFieldTest.cpp
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
    DistributionInfoTest.cpp
    EngineTest.cpp
    EvaluatorTest.cpp
    FilesystemScannerTest.cpp
    LuaEvaluatorTest.cpp
    LuaProceduresTest.cpp
    NetworkToolsTest.cpp
    OptionalTest.cpp
    RegexFallbackTest.cpp
    RegexTest.cpp
    ResultTest.cpp
    StringToolsTest.cpp
    UsersIteratorTest.cpp
    ${PROCEDURES}
)
# for ProcedureMap.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/lib/procedures)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(complianceenginetests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
    complianceenginelib
    logging
    commonutils
    parsonlib
)

gtest_discover_tests(complianceenginetests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
