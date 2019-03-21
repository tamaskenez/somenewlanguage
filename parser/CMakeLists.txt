file(GLOB HEADERS *.h)

add_executable(parser ${HEADERS}
    parser.cpp
    run_parser.cpp
    command_line.cpp
)

target_link_libraries(parser
    absl::base absl::strings absl::str_format
    microlib::microlib
    forrest::util
)
