cmake_minimum_required(VERSION 3.2)

# Set the project name and version
project(osconfig-fuzzer)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(LLVM REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer")

include_directories(${CMAKE_SOURCE_DIR}/common/commonutils)
add_executable(osconfig-fuzzer target.cpp)
target_link_libraries(osconfig-fuzzer PRIVATE mpiclient asb pthread securitybaselinelib)
