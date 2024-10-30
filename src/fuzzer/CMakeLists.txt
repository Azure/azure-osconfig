cmake_minimum_required(VERSION 3.2)

# Set the project name and version
project(osconfig-fuzzer VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(LLVM REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer")

add_executable(osconfig-fuzzer target.cpp)
target_link_libraries(osconfig-fuzzer PRIVATE mpiclient asb pthread securitybaselinelib)
