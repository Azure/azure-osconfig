# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(lib)
add_subdirectory(so)

if (BUILD_COMPLIANCE_ENGINE_ASSESSOR)
    add_subdirectory(assessor)
endif()
