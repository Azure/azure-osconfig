# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(complianceenginelib)
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=local")
endif()
# Temporary solution, until we stabilize the list of procedures.
file(GLOB PROCEDURES procedures/*.cpp)
file(GLOB SCHEMAS procedures/*.schema.json)

add_custom_target(complianceengineschema_prep
	SOURCES ${SCHEMAS}
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
    KernelModuleTools.cpp
    FilePermissionsHelpers.cpp
    Indicators.cpp
    IterateUsers.cpp
    JsonWrapper.cpp
    Procedure.cpp
    ProcedureMap.cpp
    Result.cpp
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
