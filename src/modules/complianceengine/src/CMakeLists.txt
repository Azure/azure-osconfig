# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(lib)
add_subdirectory(so)

if (BUILD_COMPLIANCE_ENGINE_ASSESSOR)
    add_subdirectory(assessor)
    if(BUILD_TESTS)
        add_subdirectory(lua-evaluator)
    endif(BUILD_TESTS)
endif(BUILD_COMPLIANCE_ENGINE_ASSESSOR)
