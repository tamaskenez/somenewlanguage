file(GLOB HEADERS *.h)

add_executable(fc ${HEADERS}
    ast_builder.cpp
    ast_syntax.cpp
    ast.cpp
    command_line.cpp
    common.cpp
    cppgen.cpp
    errors.cpp
    main.cpp
    shell.cpp
    builtinnames.cpp
)

target_link_libraries(fc
    absl::base
    absl::str_format
    absl::strings
    forrest::util
    microlib::microlib
)
