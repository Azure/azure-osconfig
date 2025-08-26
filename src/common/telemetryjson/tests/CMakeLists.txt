# Include Google Test
find_package(GTest REQUIRED)

# Create test executable
add_executable(TelemetryJsonTests
    TelemetryJsonTests.cpp
)

# Link against the telemetryjson library and required dependencies
target_link_libraries(TelemetryJsonTests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    telemetryjsonlib
    parsonlib
)

# Include directories
target_include_directories(TelemetryJsonTests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    ${CMAKE_SOURCE_DIR}/src/common/parson
)

gtest_discover_tests(TelemetryJsonTests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
