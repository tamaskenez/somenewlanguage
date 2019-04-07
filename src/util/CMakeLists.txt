file(GLOB HEADERS *.h)

add_library(util STATIC
    filereader.cpp
    headeronlies.cpp
    log.cpp
    utf.cpp
    ${HEADERS}
        arena.cpp arena.h)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)

target_include_directories(util
    PUBLIC
        $<BUILD_INTERFACE:${PARENT_DIR}>
)

target_link_libraries(util
    PUBLIC
        microlib::microlib
        absl::core_headers
        fmt::fmt
)

add_library(forrest::util ALIAS util)
