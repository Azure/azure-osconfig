project(telemetry-header)

# Add public header target only
add_library(telemetry-header INTERFACE)
target_include_directories(telemetry-header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetry-header INTERFACE dl logging)
