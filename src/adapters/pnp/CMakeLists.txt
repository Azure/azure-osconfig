# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.7)
    message(FATAL_ERROR "gcc-4.4.7 or newer is needed")
endif()

SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

project(osconfig)

set(osconfig_files
    ./AisUtils.c
    ./PnpAgent.c
    ./PnpUtils.c
    ./Watcher.c)

set(target_name osconfig)

add_executable(${target_name} ${osconfig_files})

if (EXISTS ${PROJECT_SOURCE_DIR}/azure-iot-sdk-c/CMakeLists.txt)
    message(STATUS "Using azure-iot-sdk-c as source")
    add_subdirectory(azure-iot-sdk-c EXCLUDE_FROM_ALL)
else ()
    message(STATUS "Using azure-iot-sdk-c as package")
    #find_package(azure-iot-sdk-c REQUIRED)
endif ()

if (EXISTS ${PROJECT_SOURCE_DIR}/azure-iot-sdk-c/CMakeLists.txt)
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security)

    target_include_directories(${target_name} PUBLIC
        ${azure_iot_sdks_SOURCE_DIR}/deps/parson
        ${azure_iot_sdks_SOURCE_DIR}/iothub_client/samples/pnp/common
        ${MQTT_INC_FOLDER}
        ${SHARED_UTIL_INC_FOLDER}
        ${UMOCK_C_INC_FOLDER}
        ${MACRO_UTILS_INC_FOLDER}
        ${IOTHUB_CLIENT_INC_FOLDER}
        ${UHTTP_C_INC_FOLDER}
        ${PLATFORM_INC_DIR})

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
        mpiclient)
else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security -Wno-unused-result)

    find_package (azure_c_shared_utility REQUIRED)
    find_package (azure_macro_utils_c REQUIRED)
    find_package (IotHubClient REQUIRED)
    find_package (umqtt REQUIRED)
    find_package (Parson REQUIRED)

    target_include_directories(${target_name} PUBLIC
        ${SHARED_UTIL_INC_FOLDER}
        ${MACRO_UTILS_INC_FOLDERP}
        ${PLATFORM_INC_DIR})

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
        mpiclient
        hsm_security_client
        msr_riot
        prov_device_client
        prov_device_ll_client
        prov_mqtt_transport
        utpm
        ${CMAKE_DL_LIBS})
endif()

include(GNUInstallDirs)
install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES daemon/${target_name}.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.conn DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.toml DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/osconfig)
install(FILES daemon/${target_name}.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/daemon/${target_name}.toml DESTINATION /etc/aziot/identityd/config.d/)
