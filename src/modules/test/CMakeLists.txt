# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(modulestest)

set(CMAKE_CXX_STANDARD 17)
add_library(testfactorylib MimParser.cpp TestRecipeParser.cpp ManagementModule.cpp RecipeInvoker.cpp)

target_link_libraries(testfactorylib
    ${CMAKE_DL_LIBS}
    commonutils
    logging
    pthread
    parsonlib)

target_include_directories(testfactorylib PUBLIC
    ${MODULES_INC_DIR})

add_executable(modulestest main.cpp)
target_link_libraries(modulestest gtest gtest_main pthread testfactorylib)

find_program(JQ_EXEC jq)
if(JQ_EXEC)
    add_custom_target(modulerecipeconfiguration
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../devops/scripts/generate_moduletest_metadata.sh
        COMMENT "Generating module recipe configuration"
    )
    add_dependencies(modulestest modulerecipeconfiguration)
else()
    set(jq_error "'jq' application required for generating module recipe configuration")
    message(WARNING ${jq_error})
    add_custom_target(modulerecipeconfiguration
        COMMAND true
        COMMENT ${jq_error}
    )
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
    target_link_libraries(modulestest stdc++fs)
endif()

if (BUILD_TESTS)
    add_subdirectory(unit-tests)
endif()