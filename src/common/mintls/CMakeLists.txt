# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(mintls STATIC
    MinTls.c
    MinTlsLog.c
    tls.c)
target_include_directories(mintls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mintls PUBLIC logging)
set_target_properties(mintls PROPERTIES C_VISIBILITY_PRESET hidden)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mintls PRIVATE -ffunction-sections -fdata-sections
        "$<$<NOT:$<CONFIG:Debug>>:-Os>")
    target_link_options(mintls INTERFACE -Wl,--gc-sections)
endif()
