cmake_minimum_required(VERSION 3.5)
project(forrest)

set(CMAKE_CXX_STANDARD 20)

find_package(microlib REQUIRED)
find_package(absl REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)
