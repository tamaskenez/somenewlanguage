file(GLOB HEADERS *.h)

add_executable(parser ${HEADERS}
    ast_builder.cpp
    ast_syntax.cpp
    ast.cpp
    command_line.cpp
    common.cpp
    errors.cpp
    parser_main.cpp
    cppgen.cpp
)

target_link_libraries(parser
    absl::base
    absl::str_format
    absl::strings
    forrest::util
    microlib::microlib
)
