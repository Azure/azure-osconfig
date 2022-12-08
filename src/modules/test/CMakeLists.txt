# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(moduletest)

add_library(testlib Module.c)

target_link_libraries(testlib
    ${CMAKE_DL_LIBS}
    commonutils
    logging
    pthread
    parsonlib)

target_include_directories(testlib PUBLIC ${MODULES_INC_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(moduletest main.c)

target_link_libraries(moduletest testlib)
target_include_directories(moduletest PRIVATE ${MODULES_INC_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET moduletest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:moduletest> ${CMAKE_BINARY_DIR}/moduletest
    DEPENDS $<TARGET_FILE:moduletest>
)

# Copy all the json files under `definitions` to build/test for convenience when using moduletest
file(GLOB_RECURSE JSON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/definitions/*.json)

foreach(JSON_FILE ${JSON_FILES})
    message(${JSON_FILE})
    file(COPY ${JSON_FILE} DESTINATION ${CMAKE_BINARY_DIR}/tests)
endforeach()