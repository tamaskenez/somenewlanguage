cmake_minimum_required(VERSION 3.5)
project(forrest)

message(STATUS "CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")

set(CMAKE_CXX_STANDARD 17)

find_package(microlib REQUIRED)
find_package(absl REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)
