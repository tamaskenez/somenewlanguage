#pragma once

#include <memory>

namespace forrest {

class FileReader;
struct Ast;

using std::unique_ptr;

class AstBuilder
{
public:
    static unique_ptr<AstBuilder> new_(FileReader& fr);
    virtual ~AstBuilder() {}

    virtual bool parse(Ast& ast) = 0;
};

}  // namespace forrest
