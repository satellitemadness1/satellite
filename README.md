# satellite

An interpreted language with one reserved word.

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> argz)
{
    satellite.console.display("hello, world!")

    satellite.return(satellite)
}
```

That word is `satellite`, and it is the only one. `variable`, `library`, `list`,
`time`, `file` are ordinary identifiers that mean something special only in the
second position of a satellite-rooted path — so you may name a variable `time`
and nothing breaks. One rule generates the whole surface:

> **A dotted path rooted at `satellite` names something the language owns.
> A bare identifier names something you own.**

Types are the language's, so they are `satellite.variable.number`. Your
variables are yours, so they are `count`. Method names are selectors read
relative to what precedes them, so they are bare: `count.to_string()`.
`satellite.main` is prefixed because the runtime picks that name, not you.

`satellite` as a *value* is the singleton runtime object, not a zero sentinel.
That is why `satellite.include(satellite)` and `satellite.return(satellite)`
both read sensibly: include the runtime, return the runtime — that is, succeed.

## Status

The interpreter runs. It is a **tree-walking interpreter, on purpose**, and it
is not finished.

**Works today:** lexer, parser, resolver and evaluator; capsules (functions)
with forward references and mutual recursion; spacesuits (classes) with single
inheritance, virtual dispatch, constructors and access control; exact
arbitrary-precision decimal numbers; strings, lists, slicing; `if` / `else` /
`while` / `for`; file I/O; a global variable registry with lock-free reads; a
REPL; and a GTK terminal in a separate binary.

**Not built:** `satellite.include` is parsed and then ignored. The parser
accepts any expression and the evaluator skips the node, so
`satellite.include(anything_at_all)` is accepted in silence — including a file
does nothing yet, and saying so plainly beats implying otherwise. There is no
`break` or `continue`. There is no `&&` or `||`: `.and()` and `.or()` exist as
methods, but they are ordinary calls, so both sides evaluate and neither
short-circuits. There is no string-to-number conversion and no map container.
`satellite.returns(T)` is parsed but never enforced at run time. There is no
garbage collector, so a cycle between objects leaks. The GUI window is designed
and unbuilt.

Eleven test binaries cover the above and all pass, including a ThreadSanitizer
build of the registry test.

## What it is trying to be

**One number type, one answer.** `satellite.variable.number` is an exact
arbitrary-precision decimal — a bignum significand times a power of ten. There
is no float in the language, no rounding mode, and no precision question to
answer:

```
0.1 + 0.2         →  0.3
2, squared 8 times →  115792089237316195423570985008687907853269984665640564039457584007913129639936
```

`double` was removed from the value variant deliberately and is not coming back;
a machine-double fast path for "hot" arithmetic is rejected for the same reason,
because it reintroduces the second answer under a different name. Under the
hood it is a `long long` significand with a heap bignum of base-10⁹ limbs behind
it, so ordinary arithmetic never touches the allocator.

Division is the one operation that can be inexact, and only for quotients that
do not terminate — those are carried to 34 significant digits. Everything else
is exact by construction.

**The tree stays the tree.** No bytecode VM, ever. The costs of a tree walk are
known and named — a refcounted allocation per intermediate value, a frame vector
per activation, a type-tag chain per node — and the fixes for them (interning,
non-atomic refcounts, a frame arena) do not require abandoning the tree. If a
compiler is ever written, it emits **C source**, not bytecode and not assembly,
specifically so that nothing of the current implementation survives in the
result.

**The interpreter never links the GUI.** `satl` is the interpreter and links 6
shared objects; `satl-term` is the terminal and links 119. They are separate
binaries because linking GTK made the dynamic loader do 23.4 ms of work before
`main()` on every headless run — on a program whose own front end costs 0.3 ms.
The interpreter is 510 KB.

## Performance

Measured on this machine — Xeon E5-2670 v3, 24 cores, clang 24 `-O2` — with
`make python` and `make compare`, which anyone can rerun. Both directions are
reported because both are true.

**Against CPython 3.14**, whole-process wall clock, mean of 5, with output
verified byte-identical before timing:

| | python3 | satellite | |
|---|---|---|---|
| startup (74-line feature tour) | 0.0214 s | 0.0033 s | **satellite 6.40× faster** |
| execution (300k objects, prints, calls) | 0.2146 s | 0.8780 s | satellite 4.09× slower |

That split is the honest summary of a tree walker. Startup wins because
CPython pays ~19 ms of VM boot before it does anything; execution loses because
CPython compiles to bytecode once and then runs a flat dispatch loop while
satellite re-walks the tree. The 4× ratio is stable across arithmetic, capsule
calls and objects, which is what says it is the walk itself and not one slow
operation.

**Against compiled C++, counting the compiler's own time** — because before
clang's output can run, clang has to produce it — mean of 3, built clean each
time:

| | c++ compile + run | satellite | |
|---|---|---|---|
| hello world | 0.7450 s | 0.0023 s | **satellite 324× faster** |
| 300k objects, prints, transfers | 0.8363 s | 0.9346 s | satellite 1.12× slower |

Break-even for hello world is 2,659 runs. For the object benchmark it is 0.89
runs — C++ is ahead essentially immediately once there is real work. Against the
*already-compiled* binary, ignoring build time, satellite is 36× slower, which is
where an unoptimised tree walker belongs.

## Build, test, run

Needs a C++20 compiler and libstdc++. Do not build with libc++: it has no
`atomic<shared_ptr<T>>` specialization, and the registry's lock-free read path
depends on it twice.

```sh
make                 # builds satl and satl-term
make test            # eleven test binaries, each PASS/FAIL on exit
make TSAN=0 test     # the other ten, for platforms without libtsan

./satl --run FILE [args]   # run a program
./satl                     # REPL on stdin/stdout
./satl --where             # which library directory resolved, and why
```

At the REPL, `help` prints the entire language on one screen — which is
possible only because the language *is* one screen. `:set`, `:get` and `:vars`
poke at the global registry directly.

Installing:

```sh
./install.sh --prefix /usr/local          # -n prints every command and runs none
make install prefix=/usr DESTDIR=/staging # for packagers
dpkg-buildpackage -us -uc -b              # builds satellite and satellite-term
```

The prefix is compiled into the binary as the last-resort library location;
`DESTDIR` is baked into nothing. `debian/README.packaging` is written for a
maintainer who has not done this before.

## A taste

```satellite
satellite.spacesuit item()
{
    satellite.protected
    {
        satellite.variable.string label = "unnamed"
        satellite.variable.number count = 0
    }

    satellite.public
    {
        item(satellite.variable.string name, satellite.variable.number amount)
        {
            label = name
            count = amount
        }

        satellite.capsule describe() satellite.returns(satellite.variable.string)
        {
            satellite.return(label + " x" + count.to_string())
        }
    }
}
```

A spacesuit is a class. `satellite.protected` and `satellite.public` are blocks
rather than per-member annotations, and both tables are flattened at resolve
time, so nothing walks a superclass chain at run time. Objects are a reference
type — value semantics does not survive the first method that mutates a field.

More in [`example/`](example/), including a satellite lexer written in
satellite:

```sh
./satl --run example/bootstrap/lexer.satl 'satellite.variable.number x = 3'
Word(satellite) Punct(.) Word(variable) Punct(.) Word(number) Word(x) Punct(=) Number(3) End()
```

That program exists because the plan is eventually to write a satellite-to-C
compiler in satellite, run it under this implementation, and have the result
compile its own source to a fixpoint — at which point the C++ can go. Writing
the lexer first was the cheapest way to find out what the language could not yet
say. What it found is the "not built" list above.

## Where the design lives

[`DESIGN.md`](DESIGN.md) is authoritative and long. It fixes the syntax, records
what was measured rather than assumed, and names what is still open.
[`plans/todo.txt`](plans/todo.txt) indexes it by section and says what is next,
what is decided, and what must not be redone.

Anything marked **verified** in either was checked by compiling and running code
against this tree, not reasoned about on paper.

An earlier attempt is preserved at the tag
[`v001-llvm-abandoned`](../../tree/v001-llvm-abandoned). It had no parser and no
virtual machine — it lexed a file and executed embedded C++ blocks by linking
clang into the process, for a 107 MB binary. It is kept because the current
design is a reaction to it, and it is why a compiler here would emit C source.

## License

MIT.
