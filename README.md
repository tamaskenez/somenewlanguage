# SOME NEW PROGRAMMING LANGUAGE

# Main design goals

- Non-pure, statically typed functional language equally good from scripting to system programming.
- Try to keep as much as we can from LISP: deriving from small set of constructs, single language for everything (runtime, comptime, normal code, types), keep the freedom of an untyped language, do anything if the types allow it.
- Playing around the "what if Stroustrup had studied in Edinburgh" idea just like Rust but instead of being an imperative language with functional features we aim to be a functional language with imperative features.

## The Big Ideas

- Instead of global type inference let's try a limited version which is still convenient but forces the programmer writing some annotations. In return we get simpler compiler, faster compiler, better error messages.
- Types are terms, that is, a single, unified language for terms and types. Any calculation is allowed but the types in the program must be calculated and monomorphised in compile time.
- That's why there's a big emphasis on the compile-time / runtime concept. Expressions and function parameters can be marked compile-time and any evaluation is supported in compile-time.
- Because of that, interpreted execution is also built-in.
- Besides product and sum types, union and intersection types with flow-sensitive typing.
- No exceptions, hidden control flow, implicit, hidden operations, allocations, no undefined behavior. Think of an ML flavor of Zig.

# Status

Nothing is working, this is a neverending brainstorming for now.
More design ideas in [doc/design-ideas.md](https://github.com/tamaskenez/forrestlang/blob/master/doc/design-ideas.md)
Currently the work is being done on the AST level, the language has no syntax yet.

# The Big Ideas detailed

### Restricted type inference

- The global (bidirectional) nature of the Damas-Hindley-Milner-derived inference method easily allows for the spooky-action-at-a-distance effect: things far downstream can affect the typing of a function resulting in difficult to understand error messages. The idea would be to infer types as if we were executing the program: types would flow from arguments to result in the functions, just like the values themselves.
- We still can infer type in a lot of cases but the cause of every type error must be upstream and any change only affects downstream code.
- The idea is that we give up inferring types for a term until all its type variables are resolved by the context. The price we pay: we can't type functions until the applied parameters determine all the free type variables. Also, return type of recursive functions must be specified.

We can still write functions without explicit annotations but can't query the type.

Interestingly, this type system looks more and more partly like C++ templates, partly like [Lobster's type system](https://aardappel.github.io/lobster/type_checker.html)

### Types are terms

There are two sorts, Values whose type is Type and Types whose type is TypeOfTypes which is also a Type. Both values and types are terms:

- types can be used as values, like using runtime variables holding types
- and values can be used as types, e.g. creating a variable of a type returned by a function

The latter is how type operators and (a limited kind of) dependent types are implemented. E.g. the type of list-of-integers can be produced by a List function that takes an element type.

### Polymorphism

Polymorphic functions can be generated using a `for-all` term which is universal quantification over types. These terms will be monomorphised in compile-time. Polymorphic types can be generated by functions returning types.

Ad-hoc polymorphism is supported by compile-time `if` expressions on a universally quantified parameter type.

Subtyping is supported with intersection types.

### Compile-time and runtime

The types of variables and values (program types) will be calculated in compile-time (of course, runtime variables holding types is a different thing). Apart from that function parameters and expression can be marked compile-time enforcing the compile-time availability of their values.

# Less important details, plans

- Functions have no variable argument lists but named arguments are good.
- Full c-interop will be supported, by directly including a C header.
- Dreaming of AST-level source code.
- Memory management with ideas from Lobster, preferring regions, arena support, manual (but enforced) deallocations.
- ML-style mutability with refs.
- I like APL/K/Q, what can we use from there?
- streams (like C++ ranges), pipeline-style programming and function call (see F#)
- Single language for scipting, applications, system and safety-critical code.
- Same function parameter might receive an actual array or a function, generating the values on-the-fly, also see the Dex language.
- Explicit error handling, similar to Zig (without the global error union)
- Syntax must push users to functional style but state is possible and convenient.
- Support logical paradigm in the small portions (pattern matching), let's try to put some Prolog flavor in control structures.
- Dynamic scoping. Implement OOP-like classes with dynamic scoping: since one big use of OOP classes is writing "small programs" within the program, that is, bunch of functions operate on "global-ish variables": just like standalone functions operate on global variables, methods of a class operate data members which are external to the methods. The `self` could be a dynamic variable set before calling a standalone function operating on the state behind `self`.
- Permission (effect) system on top of dynamic scoping (e.g. if function stops forwarding the memalloc permission to callees then no function called from the scope is able to memalloc). Other interesting permission could be Pure, Diverge (opposite of Total), IO, access to named APIs.
- Easy access to coroutines, merged with Go-like channels.
- Refinement types with intersection types.

# What has already been decided

(Note: all syntax below is ad-hoc, might change from paragraph to paragraph)

## Types

Parametric polymorphism is implemented by functions returning user types constructed with operations on stock types (product, sum, union, intersection).

For example, `Maybe` is a function from universally qualified T parameter to whatever the function returns:

    Maybe = fn T {
		SumType {
			Just: T
			None
		}    
    }

Except that the function above would create a unique `None` for each `T`. Sometimes we do need it:

    FloatWithMeasurementUnit = fn Mu::MeasurementUnit {
    	Float
    }

`Mu` does not appear in the result type, still we want to keep it and not mix values of `FloatWithMeasurementUnit Meter` and `FloatWithMeasurementUnit Liter`.

But we do want to write

	foo = fn x::(Maybe Int) {}
	foo None // instead of foo (Maybe Int).None

We want the type of `None` to be `(Maybe DontCareType).None` which means `forall T (Maybe T).None` so it would unify with (it's a supertype of) the expected type of `x`: `Maybe Int`. This is a syntactic problem. Anyway, what we need a good syntactic solution for is that we need to specify how to export the tags of a `SumType` (here `Just` and `None`) to top level as aliases for `(Maybe T).Just` and `(Maybe DontCareType).None`:

    Maybe = fn T {
		SumType {
			Just: T @create-toplevel-alias // which means (Maybe T).Just by default
			None    @create-toplevel-alias-to (Maybe DontCareType).None
		}    
    }

Which conveniently leads us to GADTs. This Haskell example:

	data Expr a where
	    EBool  :: Bool     -> Expr Bool
	    EInt   :: Int      -> Expr Int
	    EEqual :: Expr Int -> Expr Int  -> Expr Bool

could be specified like this:

	Expr = fn a {
		SumType {
	    	EBool: Bool                           @create-toplevel-alias-to (Expr Bool).EBool
	    	EInt: Int                             @create-toplevel-alias-to (Expr Int).EInt
	    	EEqual: Tuple [(Expr Int) (Expr Int)] @create-toplevel-alias-to (Expr Bool).EEqual
	    }
	}


