# SOME NEW PROGRAMMING LANGUAGE

## Main design goals

- Non-pure, statically-typed functional language equally good from scripting to system programming.
- Built upon a small set of rules, like LISP (maybe not that small).
- Playing around the "what if Stroustrup had studied in Edinburgh" idea just like Rust but instead of being an imperative language with functional features we aim to be a functional language with imperative features.

## The Big Ideas

- Hindley-Milner considered harmful, meaning (just like with goto) it's convenient but restriction is better. The restriction would be a strictly unidirectional type inference. The complexity we save here opens the door for the second big idea:
- Types are terms, that is, a single, unified language for terms and types. Any calculation is allowed but the types in the program must be calculated and monomorphised in compile time.
- That's why there's a big emphasis on the compile-time / runtime concept. Expressions and function parameters can be marked compile-time and any evaluation is supported in compile-time.
- Because of that, interpreted execution is also built-in.
- Besides product and sum types, union and intersection types with flow-sensitive typing.
- No exceptions, hidden control flow, implicit, hidden operations, allocations, no undefined behavior. Think of an ML flavor of Zig.

## Status

Nothing is working, this is a neverending brainstorming for now.
More design ideas in [doc/design-ideas.md](https://github.com/tamaskenez/forrestlang/blob/master/doc/design-ideas.md)
Currently the work is being done on the AST level, the language has no syntax yet.

## The Big Ideas detailed

### Hindley-Milner

The global (bidirectional) nature of the Hindley-Milner easily allows for the spooky-action-at-a-distance effect: things far downstream can affect the typing of a function resulting in difficult to understand error messages. We'd like to trade off the magic of Hindley-Milner for a more boring but simpler unidirectional type inference: the types flow downstream just like the values, from function parameters to the output.

Polymorphic terms will be either implicitly monomorphised (by calling a polymorphic function with concrete arguments) or explicitly monomorphised (e.g. escaping polymorphic function stored in a data structure).

### Types are terms

There are two sorts, Values whose type is Type and Types whose type is TypeOfTypes which is also a Type. Both values and types are terms:

- types can be used as values, like using runtime variables holding types
- and values can be used as types, e.g. creating a variable of a type returned by a function

The latter is how type operators and (a limited kind of) dependent types are implemented. E.g. the type of list-of-integers can be produced by a List function that takes an element type.

### Polymorhism

Polymorphic functions can be generated using a `for-all` term which is universal quantification over types. These terms will be monomorphised in compile-time. Polymorphic types can be generated by functions returning types.

Ad-hoc polymorphism is supported by compile-time `if` expressions on a universally quantified parameter type.

Subtyping is supported with intersection types.

### Compile-time and runtime

The types of variables and values (program types) will be calculated in compile-time (of course, runtime variables holding types is a different thing). Apart from that function parameters and expression can be marked compile-time enforcing the compile-time availability of their values.

## Less important details, plans

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

