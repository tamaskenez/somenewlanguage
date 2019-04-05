#pragma once

namespace forrest {

struct Ast;
struct CommandLineOptions;

bool cppgen(Ast& ast, const CommandLineOptions& clo);

}  // namespace forrest
