# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetry)

add_library(telemetry
    STATIC
	    Telemetry.c)

target_include_directories(telemetry
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(telemetry
    PRIVATE
        commonutils
	PUBLIC
		dl)
