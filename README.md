# forrestlang
Forrest, the new computer language

*Forrest* is a good name because this language is supposed to be simple yet much more effective than the sophisticated solutions.

## Syntax
- Define a hard-to-read intermediate language first which describes the semantics. Final, front-end syntax is for later.
- Final syntax could be along the lines of breuleux's thoughts about a less strict LISP-like syntax so we can construct AST without using semantic information.
  http://www.earl-grey.io/
  http://breuleux.net/blog/
  
Pipeline-style programming should be supported (`v = myarray | square | filter | sum`, not necessarily this syntax).
  
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

- Coroutines and goroutines should be fundamental. Goroutine with a zero-sized buffer is a coroutine.
- The semantics of the language is not expected to be very sophisticated because:
  + I don't have the education for that.
  + The usefulness of languages doesn't seem to be proportional to how sophisticated they are. By sophisticated I mean OCaml, Idris kind of languages. The main, official computer languages used at Google (2019) are Shell, C, C++, Python, Java, Javascript, Go (and Kotlin, maybe Dart). Neither of them is sophisticated (maybe the Java generics are, C++ is not sophisticated but complicated). So if that level is enough to be a Google top-5 language then it's fine.

### Modularity

We may allow the compiler at a callsite to have full access to the callee's source code or at least to the important characteristics of the callee. But rigid interfaces (even ABI) should also be possible.

TODO: Investigate modularity in other languages, most notably in ML.

## Development Environment

The environment, tools, workflows is more important than the language itself. Areas where the language can help building a productive environment.

- Fast compilation times.
- Ability to run without compiling (interpreted mode), support for REPL-style coding.
- Easy debugging, well-designed debug information, debug builds should run fast. Easily serializable, printable variables (compare how you can explore the program state in Java debugger versus a typical C++ debugger)
- IDE: Easily parsable language, solid language constructs the IDE can explore and follow.

## Feature list

- Callers must be able to limit the outcomes (effects) of the callee. Caller must be able to ban memory allocation for the callee, enforce non-divergence, limit global variables modified, all with a convenient syntax.
- All values, types (also higher-kinded) are handled as sets in a uniform way at compile-time. Common set operations are available to constructs unnamed types (sets) on-the-fly.
- Functions returning new memory must be able to use caller-supplied buffers in a convenient way.
- Functions have (1) immutable input args, (2) outputs, (3) mutable environment, which includes
  + mutable variables the function can act upon or use them as buffers and return them in the output
  + memory allocators
  + io (console)
  + permissions to diverge
  + permissions to unlimited/limited stack usage
- Part of the environment (e.g. memory allocator) can be implicitly passed to callers from one type of scopes. In the other, restrictive type of scopes everything must be forwarded explicitly. In-line defined scopes such as if-branches, loop-bodies, lambdas inherit the entire environment of the caller implicitly, even with restrictive scopes (but they become restrictive scopes themselves so they can't leak it to functions called by them).
