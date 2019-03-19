cmake_minimum_required(VERSION 3.1)
project(forrest)

set(CMAKE_CXX_STANDARD 17)

find_package(microlib REQUIRED)

add_subdirectory(d/abseil)
add_subdirectory(parser)
