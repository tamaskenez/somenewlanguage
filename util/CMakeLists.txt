file(GLOB HEADERS *.h)

add_library(util STATIC
    filereader.cpp
    headeronlies.cpp
    log.cpp
    utf.cpp
    ${HEADERS}
)

target_include_directories(util
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

target_link_libraries(util
    PUBLIC
        microlib::microlib
        absl::core_headers
        fmt::fmt
)

add_library(forrest::util ALIAS util)
