# Parsta Scopa

In plain Pasta, basic functionality comes in the form of built-in Primitives.
Even so, their resolution an invocation is fully dynamic, with every primitive
reference being a plain variable which can be redefined or shadowed.

In compiled Parsta, we really only built in basic integer operations. While
their execution remains a run-time affair, these operations are no longer
resolved at runtime, nor subject to scope.

Here, we explore the possibility of building in variable and argument
definitions as well. The result may be even more static, in the sense that
their associated expressions may no longer represent any kind of runtime
operation, and it can no longer be denied that these expressions are now part
of static syntax; and their function names have become built-in keywords.

I would advocate against inventing special syntax as a coping mechanism, but
rather to accept that there is now a variant of our language family in which
'define' and 'args' are built-ins, and their quirky quoted-string arguments no
longer imply that a bew variable is being constructed on the spot.

## Lexical Scoping Required
In dynamic scoping, the stack is unpredictable by definition; lexical scoping
fixes this, if not by definition then at least in practice, as it should keep
exact track of which sections of the stack are accessible where.

        define "foo" {
            args "bar";
            bar
        };

        define "baz" {
            args "x", "y";

            define "tempfunc" {
                + x y;
            };

            foo tempfunc
        };

        baz 42 33;

## Keeping Track of (Lexical) Scope
In the above example, 'tempfunc' exists briefly in the 'baz' scope, and is
passed as a callback, then it accesses its parent variables.

Scope analysis would yield the following:

        Pos:    Name:
        0       foo
        1       baz
        --
        2       bar / x
        3       y
        4       tempfunc

Notice that since foo and baz are defined at the same level, it appears
reasonable to re-use any variable space below this point; however, as we
can see, if 'baz' calls 'foo', this reallocation would overwrite 'x'.

The C programming language does not have this issue, because it differentiates
betweeen blocks and functions, where blocks are not invoked out of lexical
order, and the only scope shared between functions is flat and global.
In the above, everything above '--' would be global; and anything below would
be allocated in a stack-relative fashion, so that 'bar' and 'x' do not ever
share the exact same stack space.

Our challenge is that we can have several layers between 'global' and 'local'.
The best solution would be to mimic what we already do dynamically for lexical
scoping, which is to acknowledge a parent tree structure:

        Pos:
        0       closure (see below)
        1       foo
        2       closure (see below)
        3       baz
        -_
        0       parent  parent
        1       bar     x
        2               y
        3               closure (see below)
        3               tempfunc

### Compile time vs Runtime
In runtime, the above tree structure is flattened out on the stack, with the
scope for 'baz' (left) being defined before the scope for 'foo' (right), and
the parent pointer of the latter skipping over the scope of the former, as it
references its point of definition. This way, a lexically scoped stack can be
built up dynamically at runtime so as to support (multiple) recursion and
callbacks to a parent scope.

To statically analyse how to construct this stack at runtime, we make a very
similar model, with one key difference: instead of tracking actual invocations,
we track where blocks are being nested, so that the static stack analysis looks
similar to a runtime stack where each block is invoked exactly once. The
important thing is that this still tells us which variables are accessible from
which part of the parent scope tree.

So while we do not need to keep track of a variable's values at compile time,
we do need to store the (static) target of a parent pointer. More precisely:

- As we encounter a block, we need to model it as a closure, if only to match
  runtime behaviour

## Binding
As we execute a function, we define its 'parent' scope as the (surrounding)
scope at its point of definition.

Pasta simply names the raw function 'closure' to match its implementation to
its runtime point of definition. Any subsequent name given to the function then
indirectly references this 'closure' variable, so that on execution we can point
to its point of definition as the parent scope. Variable resolution is done at
runtime, simply by traversing the stack and skipping to any 'parent' pointer as
we meet it. Without this skipping, we would simply have dynamic scoping (and we
won't need to create closures in the first place).

While this 'dynamic' runtime-based variable resolution is far from optimized
for performance, the Parsta compiler at least recognizes its dynamic nature and
simply translates any variable access to a call to a primitive method:

        + (get "x") (get "y")

If we keep track of our scope tree during compile time, we should be able to
derive in which scope each variable is to be found:

        + (get (get <stackframe> "parent") "x") (get (get <stackframe> "parent") "y")

So that we can immediately index variables (and scopes) by number:

        + (get (get <stackframe> 0) 1) (get (get <stackframe> 0) 2)

Which should eliminate the need for a primitive call altogether, and instead
result in machine code roughly like this:

        add rn, fp, 0  ; locate stackframe.parent,
        add rn, rn, 1  ; locate stackframe.parent.x,
        load r0, [rn]  ; load value stored at that location
        add rn, fp, 0  ; once more, locate stackframe.parent
        add rn, rn, 2  ; locate stackframe.parent.y,
        load r1, [rn]  ; load value stored at that location
        add rd, r0, r1 ; use built-in integer function for '+'

