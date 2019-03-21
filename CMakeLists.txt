cmake_minimum_required(VERSION 3.5)
project(forrest)

set(CMAKE_CXX_STANDARD 17)

find_package(microlib REQUIRED)
find_package(absl REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(util)
add_subdirectory(parser)
