file(GLOB HEADERS *.h)

add_executable(parser ${HEADERS}
    parser
    run_parser
    command_line
)

target_link_libraries(parser
    absl::base absl::strings absl::str_format
    microlib::microlib
)
