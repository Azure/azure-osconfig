# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(complianceenginelib)
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=local")
endif()

set(PROCEDURES
    procedures/EnsureAllGroupsFromEtcPasswdExistInEtcGroup.cpp
    procedures/EnsureApparmorProfiles.cpp
    procedures/EnsureDconf.cpp
    procedures/EnsureFilePermissions.cpp
    procedures/EnsureFilesystemOption.cpp
    procedures/EnsureFirewallOpenPorts.cpp
    procedures/EnsureGroupIsOnlyGroupWith.cpp
    procedures/EnsureGsettings.cpp
    procedures/EnsureInteractiveUsersDotFilesAccessIsConfigured.cpp
    procedures/EnsureInteractiveUsersHomeDirectoriesAreConfigured.cpp
    procedures/EnsureKernelModule.cpp
    procedures/EnsureLogfileAccess.cpp
    procedures/EnsureMountPointExists.cpp
    procedures/EnsureMTAsLocalOnly.cpp
    procedures/EnsureNoDuplicateEntriesExist.cpp
    procedures/EnsureNoUserHasPrimaryShadowGroup.cpp
    procedures/EnsureRootPath.cpp
    procedures/EnsureSshdOption.cpp
    procedures/EnsureSysctl.cpp
    procedures/EnsureUserIsOnlyAccountWith.cpp
    procedures/EnsureWirelessIsDisabled.cpp
    procedures/EnsureXdmcp.cpp
    procedures/ExecuteCommandGrep.cpp
    procedures/FileRegexMatch.cpp
    procedures/PackageInstalled.cpp
    procedures/SCE.cpp
    procedures/SystemdConfig.cpp
    procedures/SystemdUnitState.cpp
    procedures/TestingProcedures.cpp
    procedures/UfwStatus.cpp
    procedures/EnsureShadowContains.cpp
)
set(SCHEMAS
    procedures/EnsureAllGroupsFromEtcPasswdExistInEtcGroup.schema.json
    procedures/EnsureApparmorProfiles.schema.json
    procedures/EnsureDconf.schema.json
    procedures/EnsureFilePermissions.schema.json
    procedures/EnsureFilesystemOption.schema.json
    procedures/EnsureFirewallOpenPorts.schema.json
    procedures/EnsureGroupIsOnlyGroupWith.schema.json
    procedures/EnsureGsettings.schema.json
    procedures/EnsureInteractiveUsersDotFilesAccessIsConfigured.schema.json
    procedures/EnsureInteractiveUsersHomeDirectoriesAreConfigured.schema.json
    procedures/EnsureKernelModule.schema.json
    procedures/EnsureLogfileAccess.schema.json
    procedures/EnsureMountPointExists.schema.json
    procedures/EnsureMTAsLocalOnly.schema.json
    procedures/EnsureNoDuplicateEntriesExist.schema.json
    procedures/EnsureNoUserHasPrimaryShadowGroup.schema.json
    procedures/EnsureRootPath.schema.json
    procedures/EnsureSshdOption.schema.json
    procedures/EnsureSysctl.schema.json
    procedures/EnsureUserIsOnlyAccountWith.schema.json
    procedures/EnsureWirelessIsDisabled.schema.json
    procedures/EnsureXdmcp.schema.json
    procedures/ExecuteCommandGrep.schema.json
    procedures/FileRegexMatch.schema.json
    procedures/PackageInstalled.schema.json
    procedures/SCE.schema.json
    procedures/SystemdConfig.schema.json
    procedures/SystemdUnitState.schema.json
    procedures/TestingProcedures.schema.json
    procedures/UfwStatus.schema.json
    procedures/EnsureShadowContains.schema.json
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
    CommonContext.cpp
    ComplianceEngineInterface.cpp
    ContextInterface.cpp
    Engine.cpp
    Evaluator.cpp
    GroupsIterator.cpp
    KernelModuleTools.cpp
    FilePermissionsHelpers.cpp
    FileTreeWalk.cpp
    Indicators.cpp
    JsonWrapper.cpp
    NetworkTools.cpp
    Procedure.cpp
    ProcedureMap.cpp
    Result.cpp
    StringTools.cpp
    UsersIterator.cpp
    ${PROCEDURES}
    )

set_property(TARGET complianceenginelib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_custom_target(ProcedureMap
    DEPENDS ProcedureMap.cpp ProcedureMap.h
)
add_custom_command(
    OUTPUT ProcedureMap.cpp ProcedureMap.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/GenProcedureMap.sh ${CMAKE_CURRENT_SOURCE_DIR}/procedures/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GenProcedureMap.sh ${PROCEDURES}
    VERBATIM)

target_link_libraries(complianceenginelib PRIVATE
    logging
    commonutils
    parsonlib
    )

target_include_directories(complianceenginelib PUBLIC
    ${MODULES_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})
