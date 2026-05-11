# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(complianceenginelib)
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=local")
endif()

# Find Lua via vcpkg
find_package(Lua REQUIRED)

set(PROCEDURES
    procedures/NoShellAccountsLocked.cpp
    procedures/AuditdRules.cpp
    procedures/PasswdGroupsExist.cpp
    procedures/ApparmorProfileState.cpp
    procedures/DconfValue.cpp
    procedures/ShellTimeout.cpp
    procedures/DefaultUmask.cpp
    procedures/FileExists.cpp
    procedures/FilePermissions.cpp
    procedures/FilesystemMountOption.cpp
    procedures/FirewalldZoneTargets.cpp
    procedures/FirewallOpenPorts.cpp
    procedures/UniqueGroupId.cpp
    procedures/GsettingsValue.cpp
    procedures/UserDotFilePermissions.cpp
    procedures/UserHomeDirectoryPermissions.cpp
    procedures/KernelModule.cpp
    procedures/LogFilePermissions.cpp
    procedures/LoginDefsOption.cpp
    procedures/MountPointExists.cpp
    procedures/MtaLocalOnly.cpp
    procedures/NoDuplicateEntries.cpp
    procedures/NoShadowPrimaryGroup.cpp
    procedures/NoUnownedFiles.cpp
    procedures/NoWorldWritableFiles.cpp
    procedures/PasswordChangeDate.cpp
    procedures/RootPathSecurity.cpp
    procedures/SshdOption.cpp
    procedures/SysctlValue.cpp
    procedures/SystemAccountShell.cpp
    procedures/UniqueUserId.cpp
    procedures/WirelessDisabled.cpp
    procedures/XdmcpDisabled.cpp
    procedures/CommandOutputMatch.cpp
    procedures/FileRegexMatch.cpp
    procedures/PackageInstalled.cpp
    procedures/SCE.cpp
    procedures/SystemdConfig.cpp
    procedures/SystemdUnitState.cpp
    procedures/TestingProcedures.cpp
    procedures/UfwStatus.cpp
    procedures/ShadowField.cpp
    procedures/SshKeyPermissions.cpp
)
set(SCHEMAS
    procedures/NoShellAccountsLocked.schema.json
    procedures/AuditdRules.schema.json
    procedures/PasswdGroupsExist.schema.json
    procedures/ApparmorProfileState.schema.json
    procedures/DconfValue.schema.json
    procedures/ShellTimeout.schema.json
    procedures/DefaultUmask.schema.json
    procedures/FileExists.schema.json
    procedures/FilePermissions.schema.json
    procedures/FilesystemMountOption.schema.json
    procedures/FirewalldZoneTargets.schema.json
    procedures/FirewallOpenPorts.schema.json
    procedures/UniqueGroupId.schema.json
    procedures/GsettingsValue.schema.json
    procedures/UserDotFilePermissions.schema.json
    procedures/UserHomeDirectoryPermissions.schema.json
    procedures/KernelModule.schema.json
    procedures/LogFilePermissions.schema.json
    procedures/LoginDefsOption.schema.json
    procedures/MountPointExists.schema.json
    procedures/MtaLocalOnly.schema.json
    procedures/NoDuplicateEntries.schema.json
    procedures/NoShadowPrimaryGroup.schema.json
    procedures/NoUnownedFiles.schema.json
    procedures/NoWorldWritableFiles.schema.json
    procedures/PasswordChangeDate.schema.json
    procedures/RootPathSecurity.schema.json
    procedures/SshdOption.schema.json
    procedures/SysctlValue.schema.json
    procedures/SystemAccountShell.schema.json
    procedures/UniqueUserId.schema.json
    procedures/WirelessDisabled.schema.json
    procedures/XdmcpDisabled.schema.json
    procedures/CommandOutputMatch.schema.json
    procedures/FileRegexMatch.schema.json
    procedures/PackageInstalled.schema.json
    procedures/SCE.schema.json
    procedures/SystemdConfig.schema.json
    procedures/SystemdUnitState.schema.json
    procedures/TestingProcedures.schema.json
    procedures/UfwStatus.schema.json
    procedures/ShadowField.schema.json
    procedures/SshKeyPermissions.schema.json
)

# Check that all globbed procedures are in the PROCEDURES list
file(GLOB GLOBBED_PROCEDURES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} procedures/*.cpp)
foreach(proc ${GLOBBED_PROCEDURES})
    if (NOT proc IN_LIST PROCEDURES)
        message(SEND_ERROR "Globbed procedure ${proc} is not in the PROCEDURES list.")
    endif()
endforeach()

# Check that every procedure has a corresponding schema
foreach(proc ${PROCEDURES})
    get_filename_component(proc_name ${proc} NAME_WE)
    set(schema_name "procedures/${proc_name}.schema.json")
    if(NOT schema_name IN_LIST SCHEMAS)
        message(SEND_ERROR "Procedure ${proc} does not have a corresponding schema file: ${schema_name}")
    endif()
endforeach()

add_custom_target(complianceengineschema_prep
    SOURCES ${SCHEMAS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/schema
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/schema/procedures
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/payload.schema.json ${CMAKE_CURRENT_BINARY_DIR}/schema
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SCHEMAS} ${CMAKE_CURRENT_BINARY_DIR}/schema/procedures)

add_custom_target(complianceengineschema
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/ComplianceEngineSchema.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cvf" ${CMAKE_CURRENT_BINARY_DIR}/ComplianceEngineSchema.zip . --format=zip
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/schema
    DEPENDS complianceengineschema_prep)
set_property(TARGET complianceengineschema PROPERTY OUTPUT ComplianceEngineSchema.zip)

add_library(complianceenginelib STATIC
    Base64.cpp
    BenchmarkInfo.cpp
    BindingParsers.cpp
    CommonContext.cpp
    ComplianceEngineInterface.cpp
    ContextInterface.cpp
    DistributionInfo.cpp
    Engine.cpp
    Evaluator.cpp
    FileTreeWalk.cpp
    FilesystemScanner.cpp
    GroupsIterator.cpp
    Indicators.cpp
    JsonWrapper.cpp
    KernelModuleTools.cpp
    ListValidShells.cpp
    LuaEvaluator.cpp
    LuaProcedures.cpp
    NetworkTools.cpp
    PasswordEntriesIterator.cpp
    Pattern.cpp
    Procedure.cpp
    ProcedureMap.cpp
    Result.cpp
    StringTools.cpp
    SystemdCatConfig.cpp
    Users.cpp
    UsersIterator.cpp
    ${PROCEDURES}
    )

set_property(TARGET complianceenginelib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(complianceenginelib PRIVATE
    logging
    commonutils
    parsonlib
    ${LUA_LIBRARIES}
    )

target_include_directories(complianceenginelib PUBLIC
    ${MODULES_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/procedures
)
