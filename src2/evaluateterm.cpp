#include "astops.h"

#include "evaluateorcompileterm.h"
#include "store.h"

namespace snl {
optional<TermPtr> EvaluateTerm(Store& store, const Context& context, TermPtr term)
{
    using Tag = term::Tag;
    switch (term->tag) {
        case Tag::Abstraction:
            assert(false);  // TODO(BUG)
            return nullopt;
        case Tag::Application:
            assert(false);  // TODO(BUG)
            return nullopt;
        case Tag::Variable: {
            auto* variable = term_cast<term::Variable>(term);
            VAL_FROM_OPT_ELSE_RETURN(value, context.LookUp(variable), nullopt);
            if (value->tag != Tag::DeferredValue) {
                return value;
            }
            auto* deferred_value = term_cast<term::DeferredValue>(value);
            switch (deferred_value->role) {
                case term::DeferredValue::Role::Runtime:  // This is probably an internal error.
                    assert(false);
                    return nullopt;
                case term::DeferredValue::Role::Comptime:  // This happens during unification.
                    return variable;
            }
        }
        case Tag::ForAll:
        case Tag::Cast:
        case Tag::Projection:
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
