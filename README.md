# Parsta
Parsta compiles Pasta code by way of a parse stack (= the budget version of a
parse tree).

## Status, and Build
Parsta can run the basic Pasta testset. Functions that were "primitive" in the
interpreter are recognized and treated as "built-in" by the compiler. Basic
integer, boolean and bitwise operations are compiled directly to their assembly
equivalents where possible.

As with the interpreter, variables are dynamically created and resolved through
some very simple C callbacks, with support for lexical scoping.

The currently maintained backends are:
- Linux x86-64 (gcc)
- MacOS arm64 (clang)
- Linux arm32 (gcc)

Generally changes are done in one port at the time, so if a port appears broken
then the Git log may confirm that it is simply behind with recent changes.
To get the right port, the Makefile selects a file called `emit_{os}_{arch}.c`,
and if your port is missing, you can try adding it.

Otherwise, typing `make test` should build the `parsta` executable, _and_ let
it compile `test.pasta` into `test.s`, _and_ compile that into the `test`
executable. 

Next, you can run the `test` executable:

        ./test

You can do the same for the Pasta testset by typing `make testset`; or just
type `make` to make everything.
