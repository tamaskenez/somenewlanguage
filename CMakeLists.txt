cmake_minimum_required(VERSION 3.5)
project(soneprol)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CLANG TRUE)
endif()

if(CLANG)
	add_compile_options("-Werror=return-type")
endif()

find_package(microlib REQUIRED)
find_package(absl REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)
add_subdirectory(src2)
