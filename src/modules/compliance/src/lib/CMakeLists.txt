# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancelib)

# Temporary solution, until we stabilize the list of procedures.
FILE(GLOB PROCEDURES procedures/*.cpp)

add_library(compliancelib STATIC
    ComplianceInterface.cpp
    Evaluator.cpp
    ProcedureMap.cpp
    JsonWrapper.cpp
    Base64.cpp
    Engine.cpp
    Procedure.cpp
    ${PROCEDURES}
    )

set_property(TARGET compliancelib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_custom_command(
    OUTPUT ProcedureMap.cpp ProcedureMap.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/GenProcedureMap.sh ${CMAKE_CURRENT_SOURCE_DIR}/procedures/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GenProcedureMap.sh ${PROCEDURES}
    VERBATIM)

target_link_libraries(compliancelib PRIVATE
    logging
    commonutils
    parsonlib
    )

target_include_directories(compliancelib PUBLIC
    ${MODULES_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})
