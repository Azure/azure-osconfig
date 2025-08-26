project(telemetryjson)

add_subdirectory(lib)

# Add tests if BUILD_TESTS is enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
