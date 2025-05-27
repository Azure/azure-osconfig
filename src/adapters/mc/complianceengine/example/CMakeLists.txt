project(OsConfigResourceComplianceExample)

#add_custom_command(
#        OUTPUT ComplianceExample.mof
#        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/MOFGenerator.py ${CMAKE_CURRENT_SOURCE_DIR}/ComplianceExample.json > ComplianceExample.mof
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MOFGenerator.py ${CMAKE_CURRENT_SOURCE_DIR}/ComplianceExample.json
#        VERBATIM)

add_custom_target(stage_create_compliance_engine_example_zip
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Staging
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineExample.metaconfig.json" ${PROJECT_BINARY_DIR}/Staging/
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_BINARY_DIR}/ComplianceEngineExample.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/ComplianceEngineExample.mof" ${PROJECT_BINARY_DIR}/Staging/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:OsConfigResourceComplianceEngine> ${PROJECT_BINARY_DIR}/Staging/Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
    DEPENDS OsConfigResourceComplianceEngine ComplianceEngineExample.mof)

add_custom_target(create_compliance_engine_example_zip ALL
    BYPRODUCTS ${OsConfigRootBinaryDir}/ComplianceEngineExample.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${OsConfigRootBinaryDir}/ComplianceEngineExample.zip" --format=zip .
    DEPENDS stage_create_compliance_engine_example_zip
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Staging/)
