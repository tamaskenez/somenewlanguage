#include "astops.h"

#include "evaluateorcompileterm.h"
#include "store.h"

namespace snl {
optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            <#code #> break;
        case Tag::Application:
            <#code #> break;
        case Tag::ForAll:
        case Tag::Cast:
        case Tag::Projection:
        case Tag::Variable:
        case Tag::UnitLikeValue:
        case Tag::DeferredValue:
        case Tag::ProductValue:
        case Tag::FunctionType:
        case Tag::ProductType:
        case Tag::StringLiteral:
        case Tag::NumericLiteral:
        case Tag::TypeOfTypes:
        case Tag::UnitType:
        case Tag::BottomType:
        case Tag::TopType:
        case Tag::StringLiteralType:
        case Tag::NumericLiteralType:
            return EvaluateOrCompileTermSimpleAndSame(true, store, context, term);
    }
}
}  // namespace snl
