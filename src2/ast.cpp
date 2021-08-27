#include "ast.h"

#include <any>
#include <typeinfo>

namespace snl {

namespace ast {

Module MakeSample1()
{
    auto stdio_initializer = new FunctionApplication{
        new Variable{"cimport"}, vector<ExpressionPtr>({new StringLiteral{"#include <stdio>"}})};
    auto stdio_printf = new Projection{new Variable{"stdio"}, "printf"};
    auto main_body_with_stdio = new ExpressionSequence{vector<ExpressionPtr>(
        {new FunctionApplication{stdio_printf,
                                 vector<ExpressionPtr>({new StringLiteral{"Print this\n."}})},
         new NumberLiteral{"0"}})};
    auto main_body = new LetExpression{"stdio", stdio_initializer, main_body_with_stdio};
    auto main_lambda = new LambdaAbstraction{vector<Parameter>(), main_body};
    auto main_def = ToplevelVariableBinding{"main", main_lambda};
    return Module{vector<ModuleStatement>({main_def})};
}

}  // namespace ast

}  // namespace snl
