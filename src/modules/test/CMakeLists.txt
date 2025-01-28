# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(moduletest)

add_library(testlib Module.c)

target_link_libraries(testlib
    ${CMAKE_DL_LIBS}
    commonutils
    asb
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

set(SRC  ${moduletest_SOURCE_DIR}/recipes)
set(MOFS
  ${OsConfigResourceSsh_SOURCE_DIR}/LinuxSshServerSecurityBaseline.mof
  ${OsConfigResourceAsb_SOURCE_DIR}/AzureLinuxBaseline.mof
)

add_custom_command(
    OUTPUT ${SRC}/SecurityBaselineTests.json
    DEPENDS ${SRC}/create-asb-json.sh ${SRC}/mof-to-json.awk ${SRC}/SecurityBaselineTests.json-header ${SRC}/SecurityBaselineTests.json-mid ${SRC}/SecurityBaselineTests.json-footer ${MOFS}
    COMMAND ./create-asb-json.sh ${MOFS} >${SRC}/SecurityBaselineTests.json
    WORKING_DIRECTORY ${SRC}
)
add_custom_target(generate-asb-test-json
    DEPENDS ${SRC}/SecurityBaselineTests.json
)
