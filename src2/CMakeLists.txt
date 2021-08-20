file(GLOB_RECURSE SOURCES *.cpp *.h)
add_executable(src2 ${SOURCES})
target_compile_definitions(src2 PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(src2 PRIVATE
	fmt::fmt
)


