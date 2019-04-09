# forrestlang
Forrest, the new computer language

*Forrest* is a good name because this language is supposed to be simple yet capable of all the things one would expect from a language.

## Main design goals

- System language, but with smart choice of defaults it should serve as a scripting language, too.
- Does not prevent the programmer to do anything but the verbosity/explicitness is proportional to the safety of the features. Easy to see or prohibit heap allocation / mutability / divergence / sharing / etc...
- The goal is to provide some kind of mix of (1) programming with high-level intents while (2) staying close to the hardware.

## Status

Nothing is working. I'm building an intermediate AST-language parser and working out the details on the go.
More design ideas in [https://github.com/tamaskenez/forrestlang/blob/master/doc/design-ideas.md]
