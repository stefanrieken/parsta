# Parsta
Parsta compiles Pasta code by way of a parse stack (= the budget version of a
parse tree).

## Status, and Build
Parsta can run the basic Pasta testset. Functions that were "primitive" in the
interpreter are recognized and treated as "built-in" by the compiler. Basic
integer, boolean and bitwise operations are compiled directly to their assembly
equivalents where possible.

As with the interpreter, variables are dynamically created and resolved through
some very simple C callbacks, presently without support for lexical scoping.

There currently are two backends: Linux x86-64 (gcc) and MacOS arm64 (clang). I
expect e.g. the combination of arm64 and Linux to be slightly different again.
At any rate, the Makefile selects a file called emit_{os}_{arch}.c, and if your
port is missing, you can try adding it.

Otherwise, typing `make test` should build the `parsta` executable, _and_ let
it compile `test.pasta` into `test.s`, _and_ compile that into the `test`
executable.

Next, you can run the `test` executable:

        ./test

You can do the same for the Pasta testset by typing `make testset`; or just
type `make` to make everything.