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
arbitrary-precision decimal numbers; strings, lists, maps, slicing; `if` /
`else` / `while` / `for`; file I/O; **multi-file programs** — a **spaceship** is
satellite's word for a file of includable code, and `satellite.include(helper)`
loads `helper.satl` and merges its declarations into the program that included
it; a global variable registry with lock-free reads; random numbers at arbitrary
precision; a REPL; and a GTK terminal in a separate binary. Output from
`satl --run` streams as the program produces it, through a printer thread that
frees each line once it is written, so a long-running program is no longer
indistinguishable from a hung one.

Includes are loaded once per file (so a diamond is not a duplicate-definition
error), cycles terminate rather than being rejected, and an error inside an
included spaceship is reported against *that* spaceship's filename and line.

**Not built:** there is no `break` or `continue`. There is no `&&` or `||`:
`.and()` and `.or()` exist as methods, but they are ordinary calls, so both
sides evaluate and neither short-circuits. There is no string-to-number
conversion. `satellite.returns(T)` is parsed but never enforced at run time.
There is no garbage collector, so a cycle between objects leaks. The GUI window
is designed and unbuilt, which is also why `satellite.include(satellite.window)`
— the language-owned form of an include — has nothing to find yet. The REPL
evaluates one line at a time, so an include typed at the prompt does not outlive
its line; neither does a capsule defined there.

Sixteen test binaries cover the above and all pass, including a ThreadSanitizer
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

**Both tables predate the console change, and every row that prints is now
optimistic.** Output moved from one accumulating `std::string` to a queue with
its own printer thread, so that a program's output appears while it runs rather
than at exit. Measured on 300,000 lines: 47% slower, and an 8.4× cut in peak
memory. Three of the five rows above print, so their numbers moved. They have
not been re-measured, because this machine no longer has the clang the tables
were built with and a figure from a different compiler does not belong in the
same column. [§9 of the design docs](design/09-runtime-architecture.md)
records what *was* measured, including the two guesses at the cause that turned
out to be wrong.

## Build, test, run

Needs a C++20 compiler and libstdc++. Do not build with libc++: it has no
`atomic<shared_ptr<T>>` specialization, and the registry's lock-free read path
depends on it twice.

`satl`, the interpreter, needs nothing else. `satl-term`, the window, needs
gtk4 and vte — `libgtk-4-dev` and `libvte-2.91-gtk4-dev` on Debian and Ubuntu,
`gtk4-devel` and `vte291-gtk4-devel` (the second from CRB) on RHEL rebuilds.
`make deps` installs them with whichever package manager is here. Without them
`make` builds the interpreter, says which package is missing, and does not fail.

```sh
make                 # builds satl and satl-term, then installs them
make deps            # installs what satl-term needs, if it is missing
make test            # sixteen test binaries, each PASS/FAIL on exit
make TSAN=0 test     # the other fifteen, for platforms without libtsan

satl --run FILE [args]     # run a program
satl                       # REPL on stdin/stdout
satl --where               # which library directory resolved, and why
```

`make` run **from this directory** installs as well as builds, so the `satl` on
your PATH is the one you just compiled and the desktop icons are the ones in
`dist/icons`. It installs to `/usr/local` if you are root and `~/.local` if you
are not — a directory you already own — and it never runs `sudo`. Run make from
anywhere else, name a `prefix=` yourself, or pass `SATELLITE_AUTOINSTALL=0`, and
it only builds. If the prefix is not writable it says so and installs nothing.

Two copies of satellite can therefore exist at once, and `satl --where` is how
you tell which one answered. GTK picks the icon the other way round from the
binary — the *last* directory in its search path wins, not the first — so a
copy left in `/usr/local/share/icons` goes on being drawn even after `make`
refreshes yours. `make` says so when it finds one.

Installing somewhere else:

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

A program can span files. Each one is a **spaceship**, and an include names it
without its extension — a bare name is yours, by the same rule that makes a
bare identifier yours:

```satellite
// greeting.satl
satellite.capsule greet(satellite.variable.string who)
{
    satellite.console.display("hello, " + who + "!")
    satellite.return(satellite)
}
```

```satellite
// main.satl
satellite.include(greeting)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> argz)
{
    greet("world")
    satellite.return(satellite)
}
```

```sh
./satl --run main.satl
hello, world!
```

`greeting.satl` is found beside the spaceship that included it, then in the
installed library directory. Every spaceship is loaded once no matter how many
times it is reached, so two files that both include a third is not an error —
and neither is a cycle: `a` including `b` including `a` terminates, and the two
can call each other's capsules, because names resolve across the whole merged
program rather than file by file.

Random numbers come in three tiers, and the tiers differ in one thing only:
how long the call spends throwing draws away before it answers.

```satellite
satellite.variable.number n = satellite.random.ultra(40)
satellite.variable.number m = satellite.random.ultra.range(1, 100)
```

`ultra(40)` is uniform over `[0, 10^40)` — so about one draw in ten prints 39
digits or fewer, because a leading zero is not printed and that is what uniform
means. `.range` is inclusive at both ends. `fast` spends 50–100 ms, `normal`
250–300 ms and `ultra` 2000–3000 ms; the draw itself is exact at any width,
because a `satellite.variable.number` is.

The tiers are a **statistical** character and not a security property. The
generator underneath is PCG, which makes no cryptographic claim and whose state
is recoverable from its output, so nothing here is described as secure — see
[§18](design/18-satellite-random.md), which records what each step of the mechanism was
measured to be worth, including the two changes that would make it stronger and
have not been made.

Output is printed by a thread of its own, and a program can ask it to slow down
without slowing itself down:

```satellite
satellite.console.display(100ms)
satellite.console.display("one line every tenth of a second")
```

`100ms` — or `100 ms`, the same literal — sets the pause the printer takes
between two displayed lines. The pause is taken by the printer thread, so the
program keeps running while its output is metered out; `0ms` puts it back to
full speed. A duration is legal in that one position and nowhere else, because
there is no duration *type* — `satellite.variable.number x = 100ms` is an error
that says so.

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

[`DESIGN.md`](DESIGN.md) is the index; the design itself is nineteen numbered
sections under [`design/`](design/), one section per file, numbered so that
`§14` is `design/14-*.md`. It is authoritative and long: it fixes the syntax,
records what was measured rather than assumed, and names what is still open.
Cite it by section number and never by line — `§8.3.1` is the whole address. [`plans/todo.txt`](plans/todo.txt) indexes it by section and says what is next,
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
