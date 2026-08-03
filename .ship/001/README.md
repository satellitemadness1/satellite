# SATELLITE — version 001

An interpreted language with a space theme. One reserved word: `satellite`.

```
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments[0])
    satellite.return(satellite)
}
```

---

## Read this first

**Version 001 has no parser and no virtual machine. A `.satl` file cannot be
executed.**

That sentence is the most important one in this document, so it is not buried
further down. If you came here to run satellite programs, you cannot yet. What
this build can honestly do with a `.satl` file is two things:

1. **Lex it** — turn it into tokens, report errors, and tell you about it.
2. **Execute the `satellite.cxx { ... }` blocks inside it** — inline C++ that is
   compiled to a shared library and called. That path is complete end to end.

Everything else in the syntax above lexes cleanly and then stops there.
`satellite.console.display` produces tokens and no output. `satellite.capsule`
defines nothing. `satellite.include` loads nothing. There is no runtime for them
to reach.

The tool says the same thing when you run it with no arguments, and `satellite
run` on a file with no C++ blocks says it again. Nothing in this build pretends
to be more than it is.

## The version scheme

Versions are three digits, zero-padded.

| version | meaning |
|---|---|
| **001** | **Preliminary. In progress. This is what you have.** Individual pieces are built and tested; the language does not run. |
| 002 | Relatively feature-complete: parser, compiler, and VM, so that satellite source actually executes. |

001 is a snapshot of foundations, published because the foundations are real and
tested — not because the language is usable. Treat it as something to read and
poke at, not something to build on.

---

## What works

335 checks pass across five test binaries: 138 for the container, 42 for the
lexer, 27 for the machine, 44 for the C++ bridge, 84 for the interpreter seam.

### `satellite_container` — the universal value type

A 16-byte tagged union (`static_assert`ed at that size), so four fit in a cache
line and passing one costs two register moves. Seven types:

| type | storage |
|---|---|
| `Nil` `Bool` `Int` `Real` | inline, never allocates |
| `Big` | heap limbs, reached only on `int64_t` overflow |
| `Str` | refcounted; 16 characters live inline before the heap |
| `List` | refcounted `std::vector<Container>` |

Heap payloads are refcounted (`std::atomic<uint32_t>`) and destroyed through the
type tag — there is no virtual dispatch anywhere in the value model.

Dispatch is **hybrid**, and that word was earned by measurement rather than
assumed: a pure two-tag dispatch table benchmarked 1.1–1.3x *slower* than a
plain if-chain at seven types, because an indirect call is itself an
unpredictable branch and it blocks inlining. So the hot pairs (`Int+Int`,
`Real+Real`) are inline branches and the table catches the long tail. The
numbers are in the repository `README.md`; the benchmark is `bench/`.

Unsupported operand pairs return `nil` rather than crashing. Real error
reporting arrives with the interpreter.

### `satellite_string` — 32 bits per character

A satellite character is **not** a Unicode codepoint. It is an index into
satellite's own character map (the full table is [below](#the-satellite-charmap)).

One character is exactly one code, so `at(i)` is O(1) with no scanning and
`remove(0)` removes the first *character* rather than part of one. Text outside
the map converts to code 0 (void) and renders as `?`.

The cost is 4 bytes per character. The reason to pay it: an 8-bit character
closes the door at 256 possibilities, and satellite does not yet know what a
satellite character will eventually need to be. The operations are `memcpy` and
`memcmp` either way, just over wider elements.

Implemented on top of it: `find`, `find_last`, `count`, `starts_with`,
`ends_with`, `compare`, `erase`, `insert`, `replace_all`, `remove_all`, `upper`,
`lower`, `trim`, `reverse`, `repeat`, slicing, split, and join.

Sorting uses a **collation key** rather than the raw code. Raw code order would
put lowercase before uppercase before digits (`"zebra" < "Apple"`), which is not
what anyone expects from a sorted list of names, so the key maps back to ASCII
order while storage keeps the satellite layout.

### BigInt — promotion and demotion

`Int` promotes to `Big` **only when an operation actually overflows**
(`__builtin_add_overflow` and friends), and demotes back to `Int` as soon as the
value fits again. Ordinary arithmetic never touches the allocator.

You can watch it happen through a `satellite.cxx` block, since that is the only
way to reach the value model from a `.satl` file today:

```
satellite.cxx
{
    Container n = Container::integer(1);
    for (int k = 1; k <= 25; ++k) n = mul(n, Container::integer(k));
    satellite.return(n.to_string() + std::string("   [") + type_name(n.type) + "]")
}
```

```console
$ ./build/satellite run factorial.satl
--- satellite.cxx block 1 (line 2) ---
  => 15511210043330985984000000   [big]   [string]

1 block(s), 1 compiled, 0 from cache, 0 failed
```

25! is 15511210043330985984000000, which is well past `int64_t`, so the value
arrives as `[big]`. The trailing `[string]` is the CLI reporting the type of
what the block returned — the block formatted its own result.

### The lexer

Two rules give the language its shape, and both are tested.

**`satellite` is the only reserved word — and only when it is not preceded by a
dot.** After a dot it is an ordinary identifier, which is exactly what makes the
escape hatch work:

```
satellite.library.my_capsule.satellite
^keyword                     ^just a name
```

Every other word belongs to the user: `capsule`, `class`, `return`, `if`,
`while` all lex as plain identifiers.

**A newline ends a statement only when the previous token could finish one.**
Go's rule. A line ending in `+` or `(` continues onto the next. Newlines inside
`( )` and `[ ]` never terminate, so argument lists span lines freely; braces
deliberately do *not* suppress termination, because statements inside a capsule
body do end.

Both rules are visible in one three-line file — see the `satellite lex` example
below.

The lexer also resolves the text-versus-arithmetic ambiguity once and for all.
Codes 98–110 are `+ - * / % ^ = == != < > <= >=` as *arithmetic*, deliberately
distinct from the ASCII characters that look like them at codes 63–92. A hyphen
inside a string is code 73; subtraction is code 99. Inside `"..."` the scanner
is in `scan_string` and everything becomes text codes; outside, `-` arrives as
`Tok::Minus` and maps to code 99. After lexing the ambiguity is gone
permanently, which is what will let tokenized code be stored as an ordinary
satellite string and read back exactly.

### `satellite.machine` — synthetic keyboard input

`/dev/uinput` creates a virtual keyboard at the kernel evdev layer, *below* X11
and Wayland, so the same code works on both and on a bare console. XTEST — what
`xdotool` uses — is X11-only and would not work on a Wayland session.

```cpp
kb.press(key::A);                                  // +A -A
kb.press_combo(key::CTRL, key::C);                 // +CTRL +C -C -CTRL
kb.press_combo(key::CTRL, key::SHIFT, key::C);     // +CTRL +SHIFT +C -C -SHIFT -CTRL
kb.press_combo({CTRL, SHIFT, ALT, META, A});       // any number of keys
kb.type_text("Hi");                                // +SHIFT +H -H -SHIFT +I -I
```

Held keys release in **reverse** order. That matters: releasing Ctrl before C in
a Ctrl+C would look to the application like a bare C after the shortcut ended.

`type_char` takes a **satellite charmap code**, not a keycode, and works out the
keystroke including shift — `A` becomes Shift+A, `!` becomes Shift+1. Linux
`KEY_*` scancodes are unrelated to both ASCII and the charmap, so that mapping
lives in one table in `src/machine.cpp`.

Two honest caveats:

- **There is no satellite-level syntax that reaches this.** It is a C++ API in
  this build, because there is no interpreter to call it from.
- Tests run in **recording mode** — the keyboard records what it *would* send
  and touches no hardware, which is what makes the key logic testable without
  root. On a machine without `/dev/uinput` permission the real-device path is
  exercised only by its permission-denied branch, and the test output says so:

  ```
  satellite.machine tests
    note: real device unavailable, as expected -- cannot open /dev/uinput: Permission denied

  27 checks, 0 failures
  ```

### `satellite.cxx { ... }` — inline C++

This is the one path in 001 that runs end to end.

The lexer never lexes the block as satellite — it counts braces (correctly
skipping braces inside string literals, character literals, comments and raw
string literals) and hands the raw text through. The bridge then:

1. Splits the block into what must live at **file scope** and what goes in a
   function body. `#include`, `using namespace`, `template`, `struct`, `class`,
   `enum`, `union`, `namespace`, `typedef`, and free function definitions are
   hoisted — C++ has no nested functions.
2. Wraps the remaining statements in an `extern "C"` entry point.
3. Compiles it to a `.so` with `g++ -O2 -std=c++20 -shared -fPIC`.
4. Caches it under `~/.satellite/cache`, keyed by an FNV-1a hash of the
   *generated* translation unit. A failed compile is never cached.
5. `dlopen`s it, checks an ABI symbol and **refuses to load on mismatch** rather
   than corrupting memory silently, then calls it.

Two design points worth knowing:

- **`satellite.return(EXPR)` becomes `to_container(EXPR)`, and C++ overload
  resolution picks the conversion.** Satellite never has to understand C++
  types; a type with no overload is an ordinary compile error naming that type.
  There is a template overload for `std::vector<T>`, so any vector of a
  convertible element type comes back as a satellite list.
- **Exceptions are caught at the boundary.** An exception unwinding out of a
  loaded module into the host's frames is undefined behaviour, so the generated
  entry point wraps everything in `try`/`catch`. A `std::exception` is returned
  as a satellite string prefixed `cxx error: ` — the run continues and the exit
  status stays 0.

The body is **ordinary C++**, semicolons and all. Only `satellite.return` is
special. Modules are deliberately never `dlclose`d: static destructors could run
while satellite values still point into them, so a loaded module lives for the
process.

### The interpreter seam — and what it deliberately does not do

`include/satellite/interpret.hpp` is the seam the real interpreter will grow
into. It declares five stages — `Lex`, `Validate`, `Parse`, `Compile`,
`Execute` — and `InterpretResult::reached` reports how far a file actually got.

**Today it reaches `Validate` and stops.** It lexes, checks the one structural
requirement that can be checked from a token stream alone (that a main file
declares `satellite.include(satellite)`), runs any `satellite.cxx` blocks, and
then says plainly that it can go no further. `Parse`, `Compile` and `Execute`
are declared and unimplemented.

The point of declaring them now is that front ends can be written once. The
seam is covered by 84 checks in `test_interpret`, and `docs/INTERPRETER.md`
plans the remaining stages one landing at a time. It carries a rule worth
repeating here: until the `Execute` stage lands, nothing in the binary, the help
text or the documentation may say that satellite code runs.

Note that `satellite.include` is checked for and **not acted on** — the seam
verifies that a main file has it, and includes nothing. Be precise about where
that check lives: it is `interpret_source()` in `src/interpret.cpp`, exercised by
`test_interpret` and by `satellite-gui`'s `interpret` command. The `satellite`
CLI does **not** run it, because `check`, `lex` and `run` drive the lexer and the
C++ bridge directly rather than going through the seam. A file with a
`satellite.main` and no `satellite.include(satellite)` is reported `ok` by
`satellite check`.

---

## What does NOT work

Stated plainly, because the list above is easy to mistake for more than it is.

- **No parser.** No compiler. No VM. `.satl` files do not execute.
- `satellite.include(...)` lexes. Nothing resolves the name or loads a file. A
  main file is *checked* for it only through the interpreter seam — the
  `satellite` CLI does not perform that check, so `satellite check` says `ok` on
  a file that omits the include.
- `satellite.capsule`, `satellite.spacesuit`, `satellite.variable.*`,
  `satellite.console.display`, `satellite.return` — outside a `satellite.cxx`
  block these produce tokens and nothing else.
- `satellite.library.x.y` paths lex as identifiers. Nothing resolves them.
- No threads and no scheduler. The threading benchmarks in the repository
  `README.md` are measurements, not an implementation.
- No `Type::Handle` and no `satellite.statement.parallel_for`.
- `satellite.machine` has no satellite-level syntax, only the C++ API.
- The `satellite` binary has no REPL and no `-e`/`--eval`.
- There **is** an optional GTK4 console (`satellite-gui`), but it is a front end
  over the interpreter seam and is bound by exactly the same limit as the CLI:
  it cannot execute satellite code either. (The CLI does not yet go through the
  seam — `src/main.cpp` drives the lexer and the C++ bridge directly — but both
  stop at the same place, because there is nothing further to reach.)
- `^` has a charmap code (103) reserved for it as arithmetic, but the lexer has
  no token for it, so `a = 2 ^ 3` is a **lex error**:
  `error: unexpected character '^'`, exit status 1. The disagreement is known
  and recorded in `docs/INTERPRETER.md` as a gap to close.

---

## What is in this folder

```
.ship/001/
├── README.md                     this file
├── common_headers.txt            the 92 standard-library headers a satellite.cxx
│                                 block can #include on this machine, measured
└── examples/
    ├── hello_main.satl           the shape of a main file — the one to copy
    ├── cxx_hello.satl            the smallest satellite.cxx block that does something
    ├── cxx_math.satl             C++ control flow — a for loop summing 1..100
    ├── cxx_vector.satl           std::vector<int> crossing back as a satellite list
    ├── cxx_template.satl         file-scope hoisting: templates defined and used
    ├── cxx_error.satl            a C++ exception caught at the boundary
    └── satellite_full_test.satl  every piece of syntax 001 can lex, in one file
```

`common_headers.txt` is a measured list, not a guess: each of 102 candidate
headers was compiled and `dlopen`ed through the exact pipeline `src/cxx.cpp`
uses, and the 10 that failed that gate are named with the reason they failed. It
is machine-specific by construction — it describes what *this* toolchain
compiles.

`hello_main.satl` and the five `cxx_*` files each carry a header comment
explaining what they demonstrate and the output to expect, and each is a
complete `.satl` file with a `satellite.main` capsule around the block — so they
show the language's shape even though only the block executes. Every one of
those "expected output" comments was checked against a real `satellite run` on
this machine, not written from memory.

`satellite_full_test.satl` is the lexer conformance input: 355 lines touching
every construct version 001 can tokenise. It lexes to 1150 tokens with zero
errors, and its three `satellite.cxx` blocks run. Everything else in it — the
capsules, the spacesuit, the arithmetic, the library paths — is a valid token
stream and nothing more.

```console
$ ./build/satellite check .ship/001/examples/satellite_full_test.satl
.ship/001/examples/satellite_full_test.satl: ok
  1150 tokens, 117 statements
  122 satellite keywords, 365 identifiers
  22 strings, 50 numbers, 3 cxx blocks
```

**The `satellite` binary is not in this folder.** Build it from the repository
root; see below.

---

## Building

```sh
cmake -S . -B build && cmake --build build -j8
```

That produces `build/satellite` (the CLI), `build/libsatellite_core.so`, five
test binaries and a benchmark:

```sh
./build/test_container      # 138 checks
./build/test_lexer          #  42 checks
./build/test_machine        #  27 checks
./build/test_cxx            #  44 checks
./build/test_interpret      #  84 checks
./build/bench_dispatch
```

`build/satellite-gui` — a GTK4 console with a transcript pane and a command
entry — is built **only if `pkg-config` finds GTK 4**, and skipped with a
message otherwise, so a machine without it still gets the library, the CLI and
the tests. It is a front end over the interpreter seam
(`include/satellite/interpret.hpp`), not a second implementation, so it is
subject to the same limits as the CLI: it cannot execute satellite code.

**Caveat worth knowing before you move anything:** the build bakes two absolute
paths into `libsatellite_core.so` — `SATELLITE_INCLUDE_DIR` (the source tree's
`include/`) and `SATELLITE_CORE_LIB` (the build tree's
`libsatellite_core.so`). They are what a `satellite.cxx` block compiles and
links against. Relocating the source or build directory after building will
break `satellite.cxx`; rebuild in place instead.

---

## Using it — the four commands

All output below is real, copied from actual runs on this machine. Paths are
relative to the repository root.

### `satellite version`

```console
$ ./build/satellite version
001
```

### `satellite` (no arguments)

Prints usage and exits **1**. `satellite help` prints the same text and exits 0.

```console
$ ./build/satellite
satellite 001 -- preliminary build

  satellite version
  satellite check <file.satl>   lex the file and report on it
  satellite lex   <file.satl>   dump the token stream
  satellite run   <file.satl>   execute the satellite.cxx blocks

What works in 001: the value type (satellite_container), strings with
the satellite charmap, big integers, the lexer, synthetic keyboard
input, and inline C++ via satellite.cxx { }.

What does NOT work yet: there is no parser and no virtual machine, so
ordinary satellite code cannot be executed.  `run` executes only the
satellite.cxx blocks in a file.
```

### `satellite check <file.satl>`

Lexes the file, reports every error, and summarises what it found. Exit status
is **0** when the file lexes cleanly, **1** when it does not, **2** when the
file cannot be read.

```console
$ ./build/satellite check .ship/001/examples/cxx_math.satl
.ship/001/examples/cxx_math.satl: ok
  42 tokens, 5 statements
  8 satellite keywords, 9 identifiers
  0 strings, 0 numbers, 1 cxx blocks
```

Errors are collected rather than thrown, so one bad string literal does not hide
the rest of the file. Given a file whose last line is
`satellite.variable.string bad = "unterminated`:

```console
$ ./build/satellite check broken.satl
broken.satl:3:33: error: unterminated string
broken.satl: FAILED
  17 tokens, 2 statements
  3 satellite keywords, 4 identifiers
  1 strings, 0 numbers, 0 cxx blocks
$ echo $?
1
```

### `satellite lex <file.satl>`

Dumps the token stream as `line:col  kind  value`. Both lexer rules are visible
in a three-line file:

```
satellite.variable.int total = 1 +
    2
satellite.library.my_capsule.satellite
```

```console
$ ./build/satellite lex scratch.satl
   1:1    satellite
   1:11   .
   1:11   ident       variable
   1:20   .
   1:20   ident       int
   1:24   ident       total
   1:31   =
   1:32   int         1
   1:35   +
   2:5    int         2
   2:1    term
   3:1    satellite
   3:11   .
   3:11   ident       library
   3:19   .
   3:19   ident       my_capsule
   3:30   .
   3:30   ident       satellite
   3:1    term
```

Read the two rules straight off that output. Line 1 ends in `+`, so there is no
`term` after it and the `2` on line 2 belongs to the same statement. And on line
3, the leading `satellite` is the keyword while the trailing one — after a dot —
is an `ident` like any other name.

A `satellite.cxx` block appears as a single token with its size, never as
satellite tokens:

```console
$ ./build/satellite lex .ship/001/examples/cxx_math.satl
...
  22:2    {
  24:5    cxx         <172 bytes of C++>
  33:1    term
  35:5    satellite
...
```

### `satellite run <file.satl>`

Executes every `satellite.cxx` block in the file, in order. **It does not
execute satellite code, because there is nothing to execute it with.**

```console
$ ./build/satellite run .ship/001/examples/cxx_hello.satl
--- satellite.cxx block 1 (line 25) ---
  => hello from C++   [string]

1 block(s), 1 compiled, 0 from cache, 0 failed
```

Run it again and the cache does its job — the summary changes and the run is
effectively instantaneous:

```console
$ ./build/satellite run .ship/001/examples/cxx_hello.satl
--- satellite.cxx block 1 (line 25) ---
  => hello from C++   [string]

1 block(s), 0 compiled, 1 from cache, 0 failed
```

A `std::vector<int>` comes back as a satellite list:

```console
$ ./build/satellite run .ship/001/examples/cxx_vector.satl
--- satellite.cxx block 1 (line 22) ---
  => [1, 4, 9, 16, 25, 36, 49, 64]   [list]

1 block(s), 1 compiled, 0 from cache, 0 failed
```

Templates defined at the top of a block are hoisted to file scope, in source
order, and can see each other:

```console
$ ./build/satellite run .ship/001/examples/cxx_template.satl
--- satellite.cxx block 1 (line 32) ---
  => ints -> 15, reals -> 4   [string]

1 block(s), 1 compiled, 0 from cache, 0 failed
```

A thrown `std::out_of_range` is caught at the boundary and returned as an
ordinary value. The run does not abort and the exit status is still 0:

```console
$ ./build/satellite run .ship/001/examples/cxx_error.satl
--- satellite.cxx block 1 (line 30) ---
  => cxx error: index 7 is past the end (size 3)   [string]

1 block(s), 1 compiled, 0 from cache, 0 failed
```

And on a file that lexes cleanly but has no C++ in it, `run` tells you the truth
rather than doing nothing quietly:

```console
$ ./build/satellite run no_blocks.satl
no_blocks.satl lexes cleanly, but contains no satellite.cxx blocks.
Version 001 has no parser or VM yet, so there is nothing else to execute.
```

A block that fails to compile is reported with the compiler's own diagnostic,
its `.so` is never cached, and `run` exits **1**.

---

## Requirements

**Platform.** Linux, x86-64. Developed and measured on AlmaLinux 10.2, kernel
6.12. `satellite.machine` is Linux-specific by construction (`/dev/uinput`), and
the C++ bridge needs POSIX (`popen`, `dlopen`). No other platform has been
tried.

**To build:** CMake ≥ 3.20 and a C++20 compiler. This build was made with GCC
17.0.0 (experimental) and CMake 3.31.8.

**To use `satellite.cxx`: `g++` must be on `PATH` at run time.** The bridge
shells out to literally `g++ -O2 -std=c++20 -shared -fPIC`. Be aware of the
limit here: the compiler name is a field on `CxxConfig`, so `clang++` can be
selected by constructing a `Bridge` with a different config from C++ — but
**version 001 exposes no CLI flag and no environment variable for it**, so from
the `satellite` command it is `g++` or nothing.

**A writable home directory**, for the compiled-block cache at
`~/.satellite/cache`. If `HOME` is unset the bridge falls back to
`$TMPDIR/satellite-cache`, then `/tmp/satellite-cache`. The cache is safe to
delete; blocks simply recompile.

**To use `satellite.machine`: permission on `/dev/uinput`**, which is root-only
by default. Grant it with a udev rule:

```sh
echo 'KERNEL=="uinput", GROUP="input", MODE="0660"' | sudo tee /etc/udev/rules.d/99-uinput.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG input $USER    # then log out and back in
```

Without it, `Keyboard::open()` returns false with a permission-denied message
and nothing crashes. The tests do not need it — they run in recording mode.

---

## The satellite charmap

Central to how satellite strings work, so here it is in full. A satellite
character is a 32-bit index into this table, not a Unicode codepoint.

| code | characters |
|---|---|
| 0 | void / error (renders as `?`) |
| 1–26 | `a` `b` `c` `d` `e` `f` `g` `h` `i` `j` `k` `l` `m` `n` `o` `p` `q` `r` `s` `t` `u` `v` `w` `x` `y` `z` |
| 27–52 | `A` `B` `C` `D` `E` `F` `G` `H` `I` `J` `K` `L` `M` `N` `O` `P` `Q` `R` `S` `T` `U` `V` `W` `X` `Y` `Z` |
| 53–62 | `0` `1` `2` `3` `4` `5` `6` `7` `8` `9` |
| 63–72 | `!` `@` `#` `$` `%` `^` `&` `*` `(` `)` |
| 73–76 | `-` `_` `=` `+` |
| 77–82 | `[` `{` `]` `}` `\` `\|` |
| 83–87 | `;` `:` `'` `"` `,` |
| 88–92 | `<` `.` `>` `/` `?` |
| 93–94 | `` ` `` `~` |
| 95–97 | space, newline, tab |
| 98–110 | **satellite math tokens** — see below |

The layout is deliberately contiguous by category, which makes every
classification a range check rather than a lookup, and case conversion plain
arithmetic. `'a'` is 1 and `'A'` is 27, so `to_upper` is `c + 26`. `'0'` is 53,
so the numeric value of a digit is `c - 53` — a subtraction, not a parse. ASCII
needs a table for these because its layout has gaps; this does not.

### The math tokens

Codes 98–110 are arithmetic, **deliberately not the same codes as the ASCII
characters that look like them**:

| code | token | code | token | code | token |
|---|---|---|---|---|---|
| 98 | `+` | 103 | `^` | 108 | `>` |
| 99 | `-` | 104 | `=` | 109 | `<=` |
| 100 | `*` | 105 | `==` | 110 | `>=` |
| 101 | `/` | 106 | `!=` | | |
| 102 | `%` | 107 | `<` | | |

They render identically to their text counterparts and can never be confused
with them:

```
source:   x = "a-b" - 2

packed:   x  ASSIGN  a  -  b  MINUS  2
                        ^        ^
                    code 73   code 99
                    (text)   (arithmetic)
```

### Two notes on the table

**Codes 95–97 (space, newline, tab) are an addition, not part of the original
design** — `"hello world"` cannot be represented without a space. They are
defined in exactly one place, `charmap::kTable` in `src/charmap.cpp`, and the
reverse lookup is built from it automatically, so moving them is a one-line
change.

**The table is not full.** `COUNT` is 111 and a character is 32 bits wide, which
leaves room to grow without a migration. That headroom is the whole reason for
paying 4 bytes per character.

---

## Where this is going

The next piece on the critical path is the **parser**, then a single-pass
compiler to bytecode that resolves `satellite.library.x.y` to slot indices at
compile time, then a register-based VM with computed goto. After that: ropes for
large-string concatenation, and a scheduler with the worker count as a runtime
knob.

- **`docs/INTERPRETER.md`** — that path broken into stages that match the
  `Stage` enum, so `InterpretResult::reached` is a literal progress bar for the
  document. Stage 0 (Lex) is built, Stage 1 (Validate) is landing, and
  everything from Stage 2 (Parse) onward does not exist. It also fixes the rule
  this whole folder is written under: until Stage 4 lands, nothing in the
  binary, the help text or the docs may say satellite code runs.

Two larger projects are mapped out in detail but not started, and both want the
VM to exist first:

- **`docs/PLAN.md`** — `satellite.statement.parallel_for` on oneTBB, and
  embedding Clang/LLVM so `satellite.cxx` needs no external compiler. Includes
  the hard part nobody gets to skip: `satellite.library` makes every variable
  reachable from anywhere, so a parallel body that writes one is a data race,
  and the compiler will have to classify every access and reject the unsafe
  ones.
- **`docs/EMBEDDING.md`** — a measured assessment of baking the C++ toolchain
  into the binary. Every number in it was measured rather than estimated, and
  the conclusion corrects the working assumption: the headers are 3.4 MB
  compressed and are not the problem; embedded Clang is 87.55 MB and cannot be
  built from any package available on the development machine, which makes
  "embed Clang" mean "build LLVM from source first."

The repository `README.md` carries the design decisions and their measurements —
the dispatch benchmark, the threading results and the three regimes they fall
into, and the string-subtraction semantics that are still open to change.

Some of those decisions are explicitly marked as cheap to change now and
expensive later. If any of them look wrong to you, 001 is the moment to say so.
