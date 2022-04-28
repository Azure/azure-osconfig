# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# The Azure IoT C SDK requires C99.
# TraceLogging requires C11.
# To use TraceLogging this component compiles as C11.
# Without TraceLogging this component can be compiled as C99.

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(osconfig)

set(osconfig_files
    ./AisUtils.c
    ./ConfigUtils.c
    ./MpiProxy.c
    ./PnpAgent.c
    ./PnpUtils.c)

set(target_name osconfig)

add_executable(${target_name} ${osconfig_files})

if (EXISTS ${PROJECT_SOURCE_DIR}/azure-iot-sdk-c/CMakeLists.txt)
    message(STATUS "Using azure-iot-sdk-c as source")
    add_subdirectory(azure-iot-sdk-c EXCLUDE_FROM_ALL)
else ()
    message(STATUS "Using azure-iot-sdk-c as package")
    #find_package(azure-iot-sdk-c REQUIRED)
endif ()

if (EXISTS ${PROJECT_SOURCE_DIR}/tracelogging/CMakeLists.txt)
    message(STATUS "Using tracelogging as source")
    add_subdirectory(tracelogging EXCLUDE_FROM_ALL)
else ()
    message(STATUS "Using tracelogging as package")
    find_package(tracelogging REQUIRED)
endif ()

if (EXISTS ${PROJECT_SOURCE_DIR}/azure-iot-sdk-c/CMakeLists.txt)
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wformat-security)

    target_include_directories(${target_name} PUBLIC
        ${azure_iot_sdks_SOURCE_DIR}/deps/parson
        ${azure_iot_sdks_SOURCE_DIR}/iothub_client/samples/pnp/common
        ${MQTT_INC_FOLDER}
        ${SHARED_UTIL_INC_FOLDER}
        ${UMOCK_C_INC_FOLDER}
        ${MACRO_UTILS_INC_FOLDER}
        ${IOTHUB_CLIENT_INC_FOLDER}
        ${UHTTP_C_INC_FOLDER})

    target_link_libraries(${target_name}
        parson
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
        iothub_client
        prov_auth_client
        umqtt
        uhttp
        aziotsharedutil
        tracelogging
        logging
        commonutils
        modulesmanager
        orchestrator)
else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wformat-security -Wno-nonnull -Wno-unused-result)

    find_package (azure_c_shared_utility REQUIRED)
    find_package (azure_macro_utils_c REQUIRED)
    find_package (IotHubClient REQUIRED)
    find_package (umqtt REQUIRED)
    find_package (Parson REQUIRED)

    target_include_directories(${target_name} PUBLIC
        ${SHARED_UTIL_INC_FOLDER}
        ${MACRO_UTILS_INC_FOLDERP})

    target_link_libraries(${target_name}
        parson
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
        iothub_client
        prov_auth_client
        umqtt
        uhttp
        aziotsharedutil
        logging
        commonutils
        modulesmanager
        lttngh
        hsm_security_client
        msr_riot
        prov_device_client
        prov_device_ll_client
        prov_mqtt_transport
        utpm
        ${CMAKE_DL_LIBS})
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.conn DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
# install osconfig.toml breaks a current install only binplace to final destination (${CMAKE_INSTALL_SYSCONFDIR}/aziot/identityd/config.d) if non-existent
install(FILES daemon/${target_name}.toml DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES telemetryevents/OsConfigAgentTelemetry.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/azure-device-telemetryd)