## Goals

- Concise syntax, appealing even for a scientist (as opposed to a programmer or PL researcher)
- Provide control and access to low-level details, predictable assembly output, no magic abstractions between language and hardware (OS or game development)
- Restrictive core language (pure, safe), powerful features are easily accessible but explicit
- Tunable, explicit safety guarantees
- Easy and complete C interop

## Non-goals

- C-like syntax or other populist design choices
- Open-world compilation model. (But, on explicit request, we should be able to generate functions having a fixed ABI and nothing else).

## Function parameters

All functions has fixed number of positional arguments. Actually, in compliance with the lambda calculus, all functions have a single argument but with some syntactic sugar it looks like fixed number of positional arguments. That is:

- No variable-sized argument list
- No overloading with different number of arguments

Additionally, the functions might have named arguments which can be optional or not.

Now:
- What if I want to pass variable number of arguments? Use a list or tuple.
- What if I want to pass different number of arguments (fixed numbers)? Create multiple functions with different names or use named, optional arguments.
- How named arguments translate to the single-arg functional model? There can be an implicit extra parameter (varargs) which
would be a struct with concrete and optional fields.

## Functional composition:

This works in Haskell:

    let twice f x = f(f(x)) in twice(twice)(twice)(twice)(succ)(0)
    
I mean, the nice thing is that `twice` is easily implementable. Maybe we need this expressive power.
[Here](https://users.rust-lang.org/t/higher-order-functions-twice/5896/4) they discuss how cumbersome is it to do the same in Rust. Can we do it like in Haskell?

    twice f x = f $ f x
    twice f x :: (X -> X) -> X -> X

## Error handling

Don't support any scheme aiming to 'recover' from failures. It's either an unrecoverable failure or business as usual:

- Handling unrecovarable failures should look like when an Erlang process dies. The rest of the system is not affected. Diagnostic reports are avaiable. No attempt to restore or gracefully deconstruct objects, reclaim resources, etc. This implies that the subsystem (which crashed) and the caller (which called the subsystem) must not share mutable memory (including the memory manager/allocator).
- All other errors are expected and must be handled explicitly.

So how can we provide the convenience of coding only a linear happy-path without exceptions or monads?

## Computational models

Three important computation models are lambda calculus (as used in Haskell), logic programming (as used in Prolog) and Turing machine (as used in imperative machines).

Let's build a language based on the mechanisms provided by these models.

- Lambda calculus is good at composition. To this it provides 3 devices: variables (free, immutable when bound), abstraction (creating pure functions) and function application. 
- Logic programming is good at declaratively describing the intention. It provides statements (meaning assertions) and ways to assemble statements.  
- Turing machine is good at handling state.

So let's use these, as close to their ideal, mathematical forms as possible. How is this different from other languages?

Conditionals and assignments must be based on a Prolog-like interpretation. `if-then` and `assert()`, `static-if` should all be the same fundamental construct. Even type checking: `f(x: int)` is, in essence, `f(x), is_integer(x) :- true.`

Funtions should be based on lambda calculus. Function are pure (modulo the attached environment, does this make sense?) and can be combined like in Haskell.

State must be explicit, not mixed with any other features (like most languages allow passing mutable reference parameters for functions)

### The Prolog part

- assignments are unifications. No need to distinguish between let-binding and equality-test
- we might need to explicitly indicate the free variable in every expression. For example, with `>`?

    >x = 3 // x must free variable, so this is a unification. Error if x is bound.
    3 = >x // so this is the same
    x = 3 // equality test, x must be bound. It's an error if x is free
    
In Prolog the right side of a rule are subgoals, the whole program consists of subgoals. In our case the default is functions. We need an explicit indication that we changed to subgoal-evaluation mode. Let's have a `fail-unless` operator. Candidates: `--`, `†`, `◻`. `□`, `|`.

So instead of

    assert x > 0
    
we can write

    -- x > 0
    
And instead of

    if x > 0:
        foo(x)
    else:
        assert x < 0
        bar(x)
    end

    if x > 0:
        foo(x)
    else:
        bar(x)
    end
    
write

    (-- x > 0:
        foo(x)
    ;-- x < 0:
        bar(x)
    )
    
    (-- x > 0:
        foo(x)
    ;
        bar(x)
    )
    
let-binding within conditions is simple:

    (-- >x = try_parse('X'), x ≠ ():
        print "so it's an X"
    ;)
    
Switch/case:

    case c
    --= 'a': ...
    --= 'b': ...
    
the special
    
    case c
     --= pattern1: expr1
    ;--= pattern2: expr2
    end
    
desugards to

    (-- c = pattern1: expr1
    ;-- c = pattern2: expr2
    )


## Syntax

- Define a hard-to-read intermediate language first which describes the semantics. Final, front-end syntax is for later.
- Final syntax could be along the lines of breuleux's thoughts about a less strict LISP-like syntax so we can construct AST without using semantic information.
  http://www.earl-grey.io/
  http://breuleux.net/blog/
  
Pipeline-style programming should be supported (`v = myarray | square | filter | sum`, not necessarily this syntax).

Probably we should use `(`, `)` only for grouping, no special meaning like tuples. Then we can allow both functional/shell-like function calls:

    f a b c
    
and parenthesized ones:

    f(a, b, c)
    
without ambiguity about passing a tuple `(a, b, c)` or 3 arguments.

For tuples, lists, sets, maps, we need something else. This is what we can choose from: `{}` and `[]`.
Also, space separated lists should be supported. Maybe a prefix char before the opening symbol? `a = L[2 4 2]`

Space-sep simple list: `[1 2 3 4 5]`.
List with function calls, so space-sep doesn't work: `[, 1, foo 4, boo 3]`.

How does it look like with curly-braces?

    {1 2 3 4 5}
    {, 1, foo 4, boo 3}

## Container names

The C++'s choice of array/vector/list has no mathematical reason. The original mathematical vector is a 2D or 3D directed line segment pointing out from the origin. This was generalized to any ordered set of values. The important is that the number of values does not change, it's part of the meaning, the structure.

Name     | Size       | Dimensions | Notes
---------|------------|------------|------
list     | may change | 1D only    | general name
array    | may change | primarily 2D, can be anything | might imply indexing
sequence | may change | 1D only    | all items might not available at once
vector   | fixed      | 1D         | 1D-vector means a scalar, that's not what we mean here
matrix   | fixed      | 2D         |
tensor   | fixed      | any        |

- size can be static, bounded dynamic, dynamic
- dimension can be 1, 2, ...
- iterable once, iterable multiple times, bidirectionally iterable multiple times, indexable, contiguous
  
## Readability

### Coding/decoding

This guy argues that it's easy to write code in any language, even in the bad ones. That's the _forward_ direction. Subsequent maintenance, debugging, frequently performed by a person not knowing the code requires to decode the code, the assumption. That's the backward direction. And that's what differentiates between good and bad languages (besides other things).

Full reference: https://www.quora.com/Which-programming-language-is-was-the-prettiest-and-or-most-readable, paragraph "Let me point out...".

### Simplicity

The complexity of the code should be proportional to the complexity of the task. It's a no-go if it's overly complex to describe a simple idea (like in many languages it's difficult to express that a function returns an int or a string).

## Predictabilty

Reading the code it must be easy to predict what happens next. `if`s make it hard because there are two choices. Virtual dispatch makes it hard, you don't know which concrete method will be called.

## Code and data

Avoid the trap of many popular languages which encourage the code, the logic to be scattered around fragments of state instead of concentrated in single, pure functions.

## Mental model behind the source code to reduce complexity

Organize the code as if the task in hand would be assigned to a bunch of moderately intelligent people. "You put this there, look up that, ask the guy over there where to find that, yell if something's not right, etc...". This approach gives a very graphical, intuitive picture about what's happening.

- Unlike purely functional approach it allows for some mutability but since it's tied to picture that could happen in the real world, the design cannot explode into an overengineered mess.
- It allows a certain level of task-delegation still your main function of the current module/task (the boss) is more-or-less overseeing and controlling the whole thing so tasks distributed over hundreds of nested calls and hidden behind layers of virtual methods are less likely.

## Guarantees

Most of the restrictions checked and enforced at compile time. The goal is to eliminate the need for coding guidelines. E.g. ensuring a MISRA-level strictness should be no more than banning the use of some keywoards and constructs with compiler flags.

Safe constructs should be easy-to-write, they should be the defaults (immutable, thread-safe, memory-safe, no-hidden-expensive-copies) but allow unsafe ones with some extra, explicit code.

Certain constructs must be able to banned at callsite or module level (like no memalloc from this point, no uninitialized memory). Certain high-risk projects (medical, aerospace) need this while others (games) need flexibility at the cost of robustness.

## Types

Q: what's the difference, which one is better, is the second one possible at all?

    Maybe a = Nothing | Just a
    sqrt = fn x -> Maybe float
    
or

    sqrt = fn x -> float | Nothing

I guess the first one is a sum type (the result of the function), the second one is a union type. Which one should we prefer?

The acrobatics needed to unpack the `Result<T, E>` [here, rust error handling docs](https://doc.rust-lang.org/book/ch09-02-recoverable-errors-with-result.html#matching-on-different-errors) suggests that union type with static type flow analysis is the way to go.

That example would look like this:

Instead of `File::open :: string -> Result<File, Error>` we have `File::open :: string -> File or Error`

    match f = File::open("hello.txt")
    | Error ErrorKind::NotFound
        match f2 = File::create("hello.txt")
        | Error _
            panic!("Problem creating the file: {:?}", f2)
    | Error _
        panic!("Problem opening the file: {:?}", f); // f is Error here
    // f is File here
    
### Co/go-routines

Coroutines and goroutines should be fundamental. Single-channel goroutine with a zero-sized buffer is semantically a coroutine, the  difference is that after a channel operation both goroutines may procceed while with coroutines they're ping-ponging on the same thread. Anyway, they're very similar, should be handled with a similar high-level construct. The same function must be able to be used as a coroutine or goroutine with buffer size 0 or more.

Even the most simple operations like linear character search in a string must be written in a way so it can be called like a coroutine (think find-all) or goroutine. This decision must be made at callsite.

- The syntax must be very simple. Nobody wants to write/call a character search with goroutine channel syntax.
- If callsite decides coroutine the whole function will be optimized into an inline for-loop.
- Callsite may need an actual, materialized string, concise redirect-into-array must be possible.

TODO: This must be studied: https://www.jtolio.com/2016/03/go-channels-are-bad-and-you-should-feel-bad/

Other links about this:
- History of CSP, some related links at the bottom: https://swtch.com/~rsc/thread/

### Extending the concept of mathematical functions with mutable environments.

Functions cannot change their input variables. They can return a value or change something in the which is passed to them. The environment is the only thing where mutation may take place. It also allows functions to see values from enclosing scopes in a controlled way. Environments can be used for the following:

- Providing functions with memory allocators (there's no default, global memory allocator but syntax must provide an implicit way of defining and calling functions as if there were one). If a caller does not explicitly provide a memory allocator for the callee, then the whole call-chain below the callee is guaranteed not to allocate heap-space.
- Permissions to use unlimited, limited or no stack are also things that can be passed in the environment.
- Permission to diverge. If caller does not provide the permission to diverge the callee is compile-time guaranteed to return in bounded time.
- There are no global variables. The local scope of the main function is an environment from where global variables can be passed down to callees. The syntax must allow easy, implicit passing (after an explicit initial permission) so it can be used like a global variable.
- Providing buffers for callee to use for the return value.
- Mutable function arguments.
- IO (like print to console, access to file system)

In unrestricted scope, the usual part of the environment is passed implicitly to callees (memory allocator, IO). In restricted scope everything must be passed explicitly.
Control structures define their own scopes. In those cases, even if the enclosing function is a restricted scope, the inline if-branches, loop-bodies and anonymous functions receive the same context but they don't forward it implicitly to callees.

### Modularity

The default is that a library contains all the necessary information for the caller to create a specialized instantiation of the callee. In situation where a stable ABI is needed to callee can be precompiled, specialized in a permissive way.

TODO: Investigate modularity in other languages, most notably in ML.

## Development Environment

The environment, tools, workflows are more important than the language itself. Areas where the language can help building a productive environment.

- Fast compilation times.
- Ability to run without compiling (interpreted mode), support for REPL-style coding.
- Easy debugging, well-designed debug information, debug builds should run fast. Easily serializable, printable variables (compare how you can explore the program state in Java debugger versus a typical C++ debugger)
- IDE: Easily parsable language, solid language constructs the IDE can explore and follow.

## Features, ideas

- All values, types (also higher-kinded) are handled as sets in a uniform way at compile-time. Common set operations are available to constructs unnamed types (sets) on-the-fly.
- All types are sets of values. There can be values that are elements of multiple types. See next entry.
- Literals have no inherent type: `0` is a value that is element of the set of integers, floats, doubles, etc...
- Type of integer division is a rational number. It still can be an integer if compile-time provable that it's an integer. Otherwise, to coerce it into an integer one must use floor or ceil or round.
- Study [Ada's Steelman Requirements](https://dwheeler.com/steelman/index.html) for ideas.
- Study the [Unison language](https://www.youtube.com/watch?v=gCWtkvDQ2ZI) which applies the git-sha idea & AST-serialization for function storage and retrieval. [Unison website](https://www.unisonweb.org/)
- The [Dark language](https://thenewstack.io/dark-a-new-programming-language-for-deployless-deployments/) also has some interesting ideas about how simplify the backend business, from editor to deployment.

### Interchangeable functions, arrays, maps, records

All of these are mappings. Lookup syntax must be similar, a function accepting one of them must accept all of them. There can be big differences in what is known at compile time but that should not be put forward, the general idea is that they are all the same high-level idea, mappings.

operation           | notation  | domain (values of x)   | codomain type (result type)
--------------------|-----------|------------------------|---------------
function application| f(x) -> y | can be a runtime value | same for all x's
map lookup          | m[x] -> y | can be a runtime value | same for all x's
array lookup        | a[x] -> y | can be a runtime value | same for all x's
vector lookup       | v[x] -> y | compile-time fixed     | same for all x's
tuple lookup        | t[x] -> y | compile-time fixed     | depends on x
field of record     | r.x  -> y | compile-time fixed     | depends on x

Need to think about this: field of record is a binary function: `get-field field-name struct-variable` which can be curried to either e.g. `get-field-foo bar` or to `get-some-field-of-bar foo`. The latter is closer to the usual `r.x` notation while the former is how it's done in functional languages (projection: `first x`). Anyway it would be neccessary to have the same, universal syntax for all the cases above.

Back to the table above: so a function accepting a record with have no reason not to accept a map or a function which returns the same information. A function accepting an array also should accept a function instead of the array:

Let v be a vector `[0, 1, 4, 9]`. We can define a function which expresses the same thing:

    f = function x: int
       where 0 <= x < 4
       -> x^2

`f` must be a drop-in replacement for `v`, accepted by all functions accepting `v`.
Another example, box function, with runtime boundaries:

    get_box_function = function lo (int) hi (int) -> (...):
        return function x (int) -> (int):
                   where lo <= x < hi
                   return 1
        end
    end
    
This is related to type-classes (since an obvious solution is to use some IndexableWithFiniteSupport kind of type class whenever it's like that). Also related to dependent-types since if we have a fixed size vector we'd like to exploit this information in compile-time and prove things. So:
 
 Q: (1) Use type-classes or (2) extend the function concept with queries about the domain of args or (3) require special types for constrained arguments (like NaturalLessThanX)
 Q: (1) Use Idris-like dependent types or (2) use some hacky way to represent numbers that can have different values in compile-time and runtime and erase that info in run-time if not needed.

Note: Constrained types are possible in Ada (define new type, integer with range 0..10) where this is a runtime check.

### Live Coding

Like this: [Eve-style clock demo in Red, livecoded!](https://www.red-lang.org/2016/07/eve-style-clock-demo-in-red-livecoded.html)

### Example of overhelming options:

Matrix multiplication function. The essence of the function is simple:

    mult = fn x y // mult function takes 2 args
    {
      var z = <some uninitialized type like x and y>
      assert #1 x = #0 y // inner dimensions must be the equal
      map 0 :< #0 x // generate numbers 0 :< height(x), call the next function on them
       \i map 0 :< #1 y // generate numbers 0 :< width(y), call the next function on them
        \j map 0 :< #1 x // generate numbers 0 :< width(x), call the next function on them
         \k x_i_k * y_k_j -> r_i_j // the innermost function initializes r_i_j
      return r
    }
    
List of all the things which we should be able to specify and customize here:

- `x` and `y` could be any type (maybe different) which can be indexed by two nonneg integers and have finite runtime size.
  Or, the types can be constrained, if necessary.
- The results of the index operations (`x_i_j`, `y_i_j`) must have a `*` binary operator. Or, it can be constrained to any
  such type or type family.
- The type of `z` must be derived from the compile/runtime characteristics of the dimensions of `x` and `y`. That is:
  - If we know the exact sizes in compile time, use fixed-size (like a C++ `array`) for `z`.
  - If sizes of `x` and `y` are bounded in compile time (like an inlined matrix with bounded runtime sizes) then use
    an inlined matrix.
  - Otherwise, use a dynamically allocated matrix. But which one? A compact, column-major one?
  Also, note that in certain cases the sizes of `x` and `y` can be known in compile-time even if they're dynamic matrices
  of functions.
  And the caller must be able to specify a custom return type. And not only that, we must be able to use a pre-allocated
  variable from the caller's environment. This is needed when we'd like to re-use the same memory in a loop, for example.
- For `z`, we must be able not to initialize it (explicitly). Still we should assert or the compiler should check that it's
  initialized at the end. An alternative solution would be to provide an `fn i j` visitor for the constructor.
- The loops must be unrolled for small, compile-time known sizes. Or customized: we should be able to specity when to
  unroll it, possibly dependending on the nature of the input dimensions

## Example of very simple Haskell code

That should be no problem to reproduce https://youtu.be/Txf7swrcLYs?t=634
