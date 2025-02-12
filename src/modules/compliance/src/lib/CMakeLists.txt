# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(compliancelib)

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

target_link_libraries(compliancelib PRIVATE
    logging
    commonutils
    parsonlib
    )

target_include_directories(compliancelib PUBLIC
    ${MODULES_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})
