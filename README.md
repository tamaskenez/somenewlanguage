# forrestlang
Forrest, the new computer language

*Forrest* is a good name because this language is supposed to be simple yet much more effective than the sophisticated solutions.

## Syntax
- Define a hard-to-read intermediate language first which describes the semantics. Final, front-end syntax is for later.
- Final syntax could be along the lines of breuleux's thoughts about a less strict LISP-like syntax so we can construct AST without using semantic information.
  http://www.earl-grey.io/
  http://breuleux.net/blog/
  
Pipeline-style programming should be supported (`v = myarray | square | filter | sum`, not necessarily this syntax).

## Container names

The C++'s choice of array/vector/list has no mathematical reason. The original mathematical vector is a 2D or 3D directed line segment pointing out from the origin. This was generalized to any ordered set of values. The important is that the number of values does not change, it's part of the meaning, the structure.

Name | Size | Dimensions | Notes
-----|------|------------|------
list | may change | 1D only | general name
array| may change | primarily 2D, can be anything | might imply indexing
sequence | may change | 1D only | all items might not available at once
vector| fixed | 1D | 1D-vector means a scalar, that's not what we mean here
matrix| fixed | 2D|
tensor| fixed | any|

- size can be static, bounded dynamic, dynamic
- dimension can be 1, 2, ...
- iterable once, iterable multiple times, bidirectionally iterable multiple times, indexable, contiguous
  
## Readability

### Coding/decoding
This guy argues that it's easy to write code in any language, even in the bad ones. That's the _forward_ direction. Subsequent maintenance, debugging, frequently performed by a person not knowing the code requires to decode the code, the assumption. That's the backward direction. And that's what differentiates between good and bad languages (besides other things).

Full reference: https://www.quora.com/Which-programming-language-is-was-the-prettiest-and-or-most-readable, paragraph "Let me point out...".

### Simplicity

The complexity of the code should be proportional to the complexity of the task. It's a no-go if it's overly complex to describe a simple task (e.g. return select the second node from a breadth-first-search performed by this function which satisfies this condition).
In other words, the less the complexity and abstraction level of the actual task should be raised when implementinging it the better.

## Predictabilty

Reading the code it must be easy to predict what happens next. `if`s make it hard because there are two choices. Virtual dispatch makes it hard, you don't know which concrete method will be called.

## Relation to the real world

The idea that the program should be a model of the real world can be benefitial (probably not always). Another, somewhat orthogonal idea could be the organize the code as if the task would be assigned to a bunch of moderately intelligent people. "You put this there, look up that, ask the guy over there where to find that, yell if something's not right, etc...". This approach gives a very graphical, intuitive picture about what's happening.

- Unlike purely functional approach it allows for some mutability but since it's tied to picture that could happen in the real world, the design cannot explode into an overengineered mess.
- It allows a level of task-delegation still your main function of the current module/task (the boss) is more-or-less overseeing and controlling the whole thing so tasks distributed over hundreds of nested calls and hidden behind layers of virtual methods are less likely.

## Guarantees

Most of the restrictions checked and enforced by e.g. a C++ static analyzer should be built into the language.
The less coding guidelines, tips, rule-of-thumbs there are, the better.

Safe constructs should be easy-to-write, they should be the defaults (immutable, thread-safe, memory-safe, no-hidden-expensive-copies) but allow unsafe ones with some extra, explicit code.

Certain constructs must be able to banned at callsite or module level (like no memalloc from this point, no uninitialized memory). Certain high-risk projects (medical, aerospace) need this while others (games) need flexibility at the cost of robustness.

## Semantics

The semantics of the language is not expected to be very sophisticated because (1) I don't have the education for that. (2)
The usefulness of languages doesn't seem to be proportional to how sophisticated they are. By sophisticated I mean OCaml, Idris kind of languages. The main, official computer languages used at Google (2019) are Shell, C, C++, Python, Java, Javascript, Go (and Kotlin, maybe Dart). Neither of them is sophisticated (maybe the Java generics are, C++ is not sophisticated but complicated). So if that level is enough to be a Google top-5 language then it's fine.

### Co/go-routines

Coroutines and goroutines should be fundamental. Single-channel goroutine with a zero-sized buffer is semantically a coroutine, the  difference is that after channel op both goroutines may procceed while with coroutines they're ping-ponging the same thread. Anyway, they're very similar, should be handled with a similar high-level construct. The same function must be able to be used as a coroutine or goroutine with buffer size 0 or more.

Even the most simple operations like linear character search in a string must be written in a way so it can be called like a coroutine (think find-all) or goroutine. This decision must be made at callsite.

- The syntax must be very simple. Nobody wants to write/call a character search with goroutine channel syntax.
- If callsite decides coroutine the whole function will be optimized into an inline for-loop.
- Callsite may need an actual, materialized string, concise redirect-into-array must be possible.

TODO: This must be studied: https://www.jtolio.com/2016/03/go-channels-are-bad-and-you-should-feel-bad/

Other links about this:
- History of CSP, some related links at the bottom: https://swtch.com/~rsc/thread/

### Extending the concept of mathematical functions with mutable contexts.

Function can only have immutable input arguments, output arguments and a third thing called a context. The context is the only thing where mutation may take place. It can be viewed as the group of the function's mutable input arguments but also provides
convenience by allowing functions to see values from enclosing scopes in a controlled way. Contexts can be used for the following:

- Providing functions with memory allocators (there's no default, global memory allocator but syntax must provide an implicit way of defining and calling functions as if there were one). If a caller explicitly does not provide a memory allocator for the callee, then the whole call-chain within the callee is guaranteed not to allocate heap-space.
- Permissions to use unlimited, limited or no stack is also part of the context.
- Permission to diverge. If caller does not provide the permission to diverge the callee is compile-time guaranteed to return in bounded time.
- Access to global variables.
- Providing buffers for callee to use for the return value.
- Mutable function arguments.
- IO (like print to console, access to file system)

In unrestricted scope, the usual part of the context is passed implicitly to callees (memory allocator, IO). In restricted scope everything must be passed explicitly.
Control structures define their own scopes. In those cases, even if the enclosing function is a restricted scope, the inline if-branches, loop-bodies and anonymous functions receive the same context but they don't forward it implicitly to callees.

### Modularity

We may allow the compiler at a callsite to have full access to the callee's source code or at least to the important characteristics of the callee. But rigid interfaces (even ABI) should also be possible.

TODO: Investigate modularity in other languages, most notably in ML.

## Development Environment

The environment, tools, workflows is more important than the language itself. Areas where the language can help building a productive environment.

- Fast compilation times.
- Ability to run without compiling (interpreted mode), support for REPL-style coding.
- Easy debugging, well-designed debug information, debug builds should run fast. Easily serializable, printable variables (compare how you can explore the program state in Java debugger versus a typical C++ debugger)
- IDE: Easily parsable language, solid language constructs the IDE can explore and follow.

## Features, ideas

- All values, types (also higher-kinded) are handled as sets in a uniform way at compile-time. Common set operations are available to constructs unnamed types (sets) on-the-fly.
- All types are sets of values. There can be values that are elements of multiple types. See next entry.
- Literals have no inherent type: `0` is a value that is element of the set of integers, floats, doubles, etc...
- Type of integer division is a rational number. It still can be an integer if compile-time provable that it's an integer. Otherwise, to coerce it into an integer one must use floor or ceil or round.

#### Interchangeable functions, arrays, maps

This should be possible:

Let v be a vector `[0, 1, 4, 9]`. Define a function which expresses the same thing:

    f = function x: int
       where 0 <= x < 4
       -> x^2

`f` must be a drop-in replacement for `v`, accepted by all functions accepting `v` and support run-time-assigned boundaries even:

    get_box_function = function lo (int) hi (int) -> (...):
        return function x (int) -> (int):
                   where lo <= x < hi
                   return 1
        end
    end
          
