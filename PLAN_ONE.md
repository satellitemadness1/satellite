# PLAN_ONE

**This file is disposable.** It is the first plan for the second satellite, written
before any of the second satellite exists, and it will be thrown away entirely and
replaced once enough of it has been built to know what it got wrong. Nothing here is
a promise. Nothing here is load-bearing. When PLAN_TWO exists, delete this file —
do not merge them, do not keep it "for reference."

What it is for: writing down the decisions made on 2026-08-25, after reading the
first satellite end to end, so that the reasons survive the week.

The first satellite lives at `old_versions/first_satellite/` and is not going
anywhere. It works, it is fast, and it is the source we are pulling from.

---

## 0. The two questions this plan answers

1. **What replaces the naive tree-walk?** Not bytecode. An arena-allocated AST,
   compiled once into a tree of closures, with inline caches on the call nodes.
2. **Is `satellite` still the integer 1?** Yes. See §2 — every node numbers its own
   children from 1, a path is that sequence of numbers, and the first satellite
   built a numbering like it and then never used it in the hot path.

---

## 1. What we found in the first satellite

The finding that shapes everything else:

**The first satellite already built the number table, and the evaluator never read
it.**

`src/bytecode_format/format.def` holds 108 words and 30 paths with frozen ids —
`SAT_WORD(1, SATELLITE, "satellite")`, `SAT_WORD(6, CONSOLE, "console")`,
`SAT_WORD(7, DISPLAY, "display")`, `SAT_PATH(P_DISPLAY, 1, 6, 7, 0, 1)` — with
static_asserts enforcing density, ascension and uniqueness. It is careful work.

Meanwhile the actual dispatch for `satellite.console.display(x)`, the most-called
thing in the language, is:

1. `expr_call.cpp` flattens the `Member` chain into a `std::vector<std::string>`
2. `modules.cpp:33` heap-allocates a joined `std::string` via `join_path()` —
   **on every module call, including the arms that never read it**
3. six arms are tried in a **load-bearing order** (the file says so: "the arms are
   not disjoint"), each doing `full == "satellite…"` or `path[0] == "satellite" &&
   path[1] == "…"` string comparisons
4. `module_console` is the **last** arm tried

So the fastest identity the language has was compiled into a header, asserted over,
documented at length — and then the hot path compared strings.

That is the whole thesis of the second satellite: **finish the job the numbering
started.**

Frames tell the other half of the story. `resolve()` turns local names into integer
slot indices statically, once, before anything runs — and it is the fastest part of
the interpreter. Same idea, applied, and it worked. `env.hpp` records the
measurement that justified it: without frames, eight threads running a capsule with
no recursion and no shared state produced **1585 wrong results out of 1600**.

Names got numbers and got fast. Paths got numbers and stayed strings.

---

## 2. The numbering

**Every node in the namespace numbers its own children, starting at 1.** A path is
the sequence of those numbers, read left to right.

```
satellite . console . display
    1     .    1    .    1
```

`satellite` is 1 because it is the root. `console` is 1 because it is the first
child of `satellite`. `display` is 1 because it is the first child of `console`.

```
satellite.console.display   1 1 1
satellite.console.input     1 1 2      input is console's second child
satellite.variable          1 2        variable is satellite's second child
satellite.random            1 5        random is satellite's fifth child
satellite.random.fast       1 5 1      fast is random's first child
satellite.random.normal     1 5 2      normal is random's second child
satellite.random.ultra      1 5 3
```

Note what `satellite.console.input` and `satellite.random.normal` have in common:
both end in 2, and the 2's are unrelated. One means "console's second child" and
the other means "random's second child". That is the scheme working, not a
collision.

**So there are a lot of 1's and a lot of 2's, and that is correct.** Every node has
a child 1 and most have a child 2. A number on its own means nothing; a number
means something *at a position, under a parent*. `1 1 2` is
`satellite.console.input` and `1 2 2` is something else entirely, and neither of
those 2's has anything to do with the other.

### 2.1 The lists are per-parent, not per-level

At the second word this distinction does not show, because every second word is a
child of `satellite` — one parent, so "the second word id list" and "satellite's
child list" are the same list.

At the third word it shows. `display` and `input` are children of `console`;
`string` and `number` are children of `variable`. Those are **two separate lists**,
each numbered from 1, not one flat list of every word that ever appears third.

Per-parent is the right one, for three reasons:

- **Validation is free.** `1 2 1` means "take satellite's child 2, then take *that
  node's* child 1." If the node has no child 1, the path does not exist, and you
  learned that from the array bound. A flat per-level list would let `1 2 7` name a
  legal level-3 word that is not legal under `variable`, so legality would need a
  second check.
- **It has no depth limit**, which is the requirement. A flat level-6 list mixing
  every unrelated namespace at that depth is not a list anyone can reason about.
- **Adding a word touches one node.** Registering a new child of `console` cannot
  renumber anything under `variable`.

### 2.2 Registration order is the numbering, so it is frozen

Because a child index *is* its position in its parent's list, **the order words are
registered in is the numbering**, and it is frozen exactly the way the first
satellite froze its word ids:

> Never renumber. Never reuse. Always append.

`console` is child 1 of `satellite` because it is registered first, and it stays
child 1 forever. A new child of `satellite` goes on the end. Removing one leaves a
hole rather than shifting its neighbours down.

This is stricter than the first satellite's flat list, and it has to be: there, a
word id was global, so moving a word between namespaces did not change its number.
Here the number *is* the position, so the position is the frozen thing.

### 2.3 Spelling is a separate table

A node stores its own spelling. Printing `1 2 1` back as
`satellite.variable.string` is a walk from the root, one node per segment, and the
whole path is always available wherever a number is — so nothing needs a global
word-id-to-text table to produce a name.

What we do keep is a **string interner** for the spellings themselves, so that
`list` under `container` and `list` under `directory` are two nodes sharing one
piece of text. That is deduplication, not identity: two nodes, one string, two
different child indices, and rightly so.

The **reservation rule** reads this same table — "is this bare word a reserved
spelling anywhere in the trie" is a lookup in the interner, not a walk.

### 2.4 PathId — one `uint32_t` for a whole path

The numbers above are how a path is *structured*. This is what a path *is* at
runtime.

`satellite.console.display` is **one thing**. It should not travel as `1 1 1` in
three registers, and it should not travel as three `long long`s — three 64-bit ints
is 24 bytes where a `uint32_t` is 4 and covers four billion paths. The trie walk
happens once, at parse time; the terminal node it lands on has an interned id; that
single integer is what the closure holds. Dispatch becomes:

```cpp
handlers[path_id](frame, args)
```

One array index. One indirect call. No allocation, no string, no ordered arms.

So the numbers are the *source of truth and the way a path is written down*, and the
PathId is the *handle*. The first satellite's `SAT_PATH(ident, s1, s2, s3, s4,
arity)` was reaching for exactly this and stopped at four segments, which is a word
limit we do not want.

### 2.5 What the numbering buys beyond speed

- **Better errors, which is an explicit goal.** A failed walk knows *which segment*
  failed and *which node* it failed under, so `satellite.consle.display` answers
  "no `consle` under `satellite` — did you mean `console`?" by running edit distance
  over that one node's children. The first satellite's answer today is
  `no such module function: satellite.consle.display`.
- **`satellite.help` becomes a walk of the trie**, so help cannot drift from what
  exists — the trie *is* what exists.
- **The reservation rule becomes one interner lookup.**

### 2.6 The rule we inherit, unchanged

From the top of `format.def`, and it stays in force:

> **PROSE MAY EXPLAIN A NUMBER. IT MAY NEVER BE THE ONLY PLACE THE NUMBER LIVES.**

The first satellite adopted this rule and still shipped three commits where the
registry had zero consumers, which is how two sections assigned kind 4 to different
things and neither noticed. **The registry gets a consumer in the same milestone it
gets written** (M2), and `satl --words` exists partly so that it always has one.

Under per-parent numbering this matters more, not less: a child index is only
meaningful relative to a registration order, so that order must be compilable data
and never prose.

## 3. Architecture

### 3.1 The options considered

Bytecode was ruled out by the brief. What remains:

| | approach | verdict |
|---|---|---|
| 1 | naive tree-walk (`variant` visit, `shared_ptr` children) | what v1 is — the baseline we are beating |
| 2 | **closure compilation** — walk once, emit a tree of callables with every static decision baked in | **adopt** |
| 3 | **flattened arena AST** — nodes contiguous, children by `uint32_t` index | **adopt** |
| 4 | **self-specializing nodes / inline caches** (the Truffle model) | **adopt, on call sites** |
| 5 | explicit control stack (CEK) — pausable, resumable, no C++ stack overflow | **defer** — see §3.5 |
| 6 | graph reduction | no — for lazy functional languages |
| — | copy-and-patch (pre-compiled machine-code stencils) | out of scope — it is a compiler |

### 3.2 Why closure compilation is not bytecode

It needs saying, because they look adjacent:

- no linear instruction stream
- no opcode decode loop
- no serializable format
- no compile step the user ever runs or waits for

The closure build is the same pass as resolve, measured in microseconds, and it
happens between "parsed" and "running" the same way resolve already does. From
outside, `satl file.satl` is exactly as interpreted as it was.

What it buys: every decision that *can* be made before execution *is*. Which frame
slot. Which PathId. Which handler. How many arguments, already checked. At runtime
a node is one indirect call with no tag test and no re-resolution. 2–5× over a
naive walk is the usual figure.

### 3.3 Why the arena

The first satellite's tree is `shared_ptr<const Expr>` with `sizeof(Expr) == 96`,
guarded by a static_assert. That means:

- an **atomic refcount touch per node visit**
- a **cache miss per child**, since children are wherever the allocator put them
- **atomic contention across threads** walking one shared program

And the refcounting buys nothing: the tree is immutable, lives as long as the
program, and nothing ever frees a node early.

An arena of PODs indexed by `uint32_t` fixes all four. Multi-threaded walking
becomes **atomic-free**, not merely safe — which matters because
`satellite.variable.thread` is on the roadmap.

It also kills a documented data race. `Name::slot` is `mutable int` on a
`shared_ptr<const Expr>`, and the comment holding the race off reads: *"resolve()
must finish, on one thread, before any evaluation begins."* With an arena and a
separate resolved side-table indexed by node id, that race is **structurally
impossible** rather than documented.

### 3.4 Inline caches

A `Call` node caches the resolved PathId and handler pointer on first execution,
behind a guard. This is what permanently retires the six-arm chain: the second
execution of a call site does no lookup at all.

### 3.5 Why the explicit control stack is deferred, not dropped

A CEK machine would make execution **pausable and resumable**, which is what you
want for green threads, generators, a stepping debugger, Ctrl-C at an arbitrary
point, and driving the interpreter from a GTK idle callback with no second thread.

It also costs 2–3× against direct recursion and is genuinely hard to write. So: not
now. But **bound the recursion depth from M7 onward** so deep recursion produces a
clean `capsule call too deep` error instead of a segfault. The first satellite has
`system_facts/stack_facts.cpp` already; the machinery is half there.

### 3.6 Order of adoption

Arena first — it is a data-layout decision and everything else rides on it. Then
closure compilation. Then inline caches. Doing them in the other order means doing
the arena twice.

---

## 4. GTK, windows, and threads

### 4.1 The factual answer

**GTK is not thread-safe, and there is no threadable alternative** — not SDL, not
Qt, not FLTK, not raw Wayland. This is a platform constraint, not a GTK failing:
Win32 gives windows thread affinity, macOS AppKit is main-thread-only, and
Wayland/X11 connections are not safe to share.

GTK3 had `gdk_threads_enter()` / `gdk_threads_leave()` (a global lock), deprecated
in 3.6. **GTK4 removed it entirely.** All GTK/GDK calls must happen on the thread
that owns the GMainContext where GTK was initialised.

The supported model is **one UI thread plus a message queue**: workers do work and
marshal to the UI thread with `g_main_context_invoke()` / `g_idle_add()`, or run
background work under `GTask`.

### 4.2 Which is good news, because we already have that shape

`console_output/console.hpp` is *exactly* this pattern: producer threads push into
a locked queue, one printer thread consumes, and a line stays atomic because the
unit queued is a whole string. A window is the same pattern with the polarity
flipped. Satellite already has the idiom; it has not pointed it at a window yet.

And **"if we can do it for the user, we do it for them"** decides the surface: the
user writes `satellite.window.new(...)` from any thread and satellite does the
marshalling silently. The user never learns the word "main thread."

### 4.3 GTK4 is not slow — startup is

The first satellite measured it exactly:

```
satl --run hello.satl, linked against gtk4       25.9 ms
the same interpreter, not linked against gtk4     2.5 ms
a bare int main(){return 0;}                      2.2 ms
```

23.4 of those 25.9 ms are the dynamic linker, and the interpreter's own share of
hello world is 0.3 ms. Rendering was never the problem; GTK4 draws through a GPU
backend and is fine.

The first satellite's source attributes the load to "119 shared objects". **That is
not this machine's number** -- measured 2026-08-26, its `satl-term` resolves 79 and
maps 78, against `satl`'s 6. The shape holds and the count does not, so the count is
not repeated anywhere in the second satellite. Re-measure before quoting.

### 4.4 The split does not solve the window problem — dlopen does

The first satellite fixed this by making `satl` and `satl-term` two binaries. That
works for a terminal host, which never interpreted anything anyway.

**It does not work for `satellite.window.new()`**, which runs inside a user program,
which runs in `satl`. You cannot split that out.

The answer is **`dlopen` a `libsatellite_window.so` the first time a window path
executes.** Startup stays at 2.5 ms for every program that never opens a window,
and the 23 ms is paid only by programs that do. This is a genuine improvement over
the first satellite rather than a port of it.

### 4.5 What stays

- **GTK4 + VTE.** No toolkit change. The alternatives all impose the same one-UI-
  thread rule and cost us a native widget set.
- **`satl-term` stays**, and keeps its name.
- **`satl` keeps its name.**
- **The 2.5 ms startup number is a guardrail, not a memory.** It gets measured in
  M1, before there is a language to slow it down, and it gets measured again in
  every milestone after.

---

## 5. What we keep

Ported, adapted, or taken as-is from `old_versions/first_satellite/`:

- **The syntax and the generating rule.** It is coherent and it is the identity.
  `design/01-the-generating-rule.md` through `design/21-*` are the prose to re-read,
  not to rewrite from scratch.
- **`resolve()` and frames** — `src/environment/env.hpp`. Names to integer frame
  slots, statically, before anything runs. Best engineering in the first satellite,
  and measured: 1585 wrong results out of 1600 without it.
- **Frames as isolation, library as the atomic global.** The `SLOT_GLOBAL` /
  `SLOT_CAPSULE` / `SLOT_METHOD` / `SLOT_SUIT` / `SLOT_FIELD` sentinel scheme.
- **Exact-decimal `Number` on base-10⁹ `BigInt`, with `double` constructors
  deleted.** Refusing binary floats at the C++ type level, so `Value v = 3.14` is a
  compile error rather than a silent truncation to 3, is a genuinely good call and
  it is the foundation `satellite.variable.float` needs.
- **The Console with its own printer thread**, and the `drain()` barrier before
  reading input.
- **The SIGINT handling** — installed without `SA_RESTART`, and using `eof()` to
  tell a real closed stdin from an EINTR-interrupted read. Hard-won; do not
  rediscover it.
- **The search power** — `src/evaluator/search.hpp`, `search.cpp`,
  `search_walk.cpp`. The comparator/walker split is clean, deliberately free of the
  Evaluator class, and *callable from anything*. That is the point: the ten-level
  ladder (`SEARCH_EXACT` … `SEARCH_SUBSEQUENCE`) is a general power that can be
  applied to any value the language has, present or future. Port it close to
  unchanged.
- **The X-macro registry mechanism**, ids frozen, append-only, static_asserts in
  the header so every consumer inherits them.
- **`format.def`'s rule** (§2.5), verbatim.
- **Spans on every node.**
- **PCG, and the fast / normal / ultra tiers.**
- **The two-binary split and the startup measurement discipline.**
- **The comment culture.** Comments that state a number and where it came from, not
  an intent. Rare and valuable. Keep writing them.

---

## 6. What we throw away

- **String-keyed dispatch.** All of it: `join_path()`, the ordered arms, the
  `full == "…"` ladders, `"satellite.random." + path[2]` built per call. This is the
  change everything else hangs off.
- **`shared_ptr<const Expr>` for the tree.** See §3.3.
- **`mutable int slot` on `Name`.** See §3.3.
- **`EvalError{std::string, Span}`.** 177 hand-written message strings, no codes, no
  source excerpt, no caret, no call stack, no suggestions. The new shape is
  `{ErrorCode, Span, vector<Note>, vector<FrameRef>}` with rendering in exactly one
  place. `ResolveError` already grew a `note` / `note_span` pair — that is the model
  asking to be generalised.
- **"Record the error and return `nullptr`."** Keep the decision *not* to throw —
  it is measured, 8.5 ns as an enum against 1537 ns thrown, 181× — but 177 sites
  where a missed null check is a segfault is the wrong shape. Either a sticky
  machine flag checked at statement boundaries, or a return type that cannot be
  silently dropped.
- **`help_for()` and `module_of()` switching on raw `std::variant` indices.** The
  code documents this as a landmine — "APPEND ONLY", "silently renumber every
  alternative after it". Make adding an alternative a compile error.
- **The `bytecode_format` module *as named*.** The ids are right; the framing is
  wrong. It is not a bytecode format, it is the language's word-and-path registry,
  and in the second satellite it belongs in the hot path, not in a serialiser. New
  home: `src/satellite_words/`.
- **The fixed four-segment `SAT_PATH`.** No word limit — the trie has no depth
  limit.
- **The `100ms` special case** in `eval_call`, which matches on the argument
  *expression* before `eval_args` and hardcodes
  `join_path(path) != "satellite.console.display"`. With a real registry, `display`
  declares that it accepts a pace argument.
- **Housekeeping.** No `.o` files committed beside sources. No `satl.tar.xz` /
  `satellite_rhel.tar.xz` blobs in the tree. No `pcg_test` / `print_test` /
  `speed_test` / `view_test` scratch directories at the root. Build output lives
  out of source.

---

## 6a. The line rule

**No C++ file exceeds 300 lines.**

This is tighter than the first satellite's 325 and it is a hard default rather than
a suggestion. The first satellite arrived at its ceiling late, by splitting a
2208-line `eval.cpp` and two 700-line functions after they had already been written
— and the splits are visible in the result, with headers named `eval_internal.hpp`
existing to hold what an anonymous namespace used to and comments explaining that
"the bodies below are UNCHANGED; they moved."

Splitting a file after the fact preserves its shape. Writing to a ceiling changes
the shape. So the ceiling applies from the first commit of every file, not from the
commit where someone notices.

Consequences to plan for rather than discover:

- **An umbrella header is a legitimate answer**, and the first satellite's
  `value.hpp` and `ast.hpp` are the model: one door that includes its parts in an
  order that compiles, so every consumer's include line stays the same.
- **A dispatch table is a legitimate answer** and a chain of `if` arms is not. This
  is the same change §1 already demands for other reasons — `handlers[path_id]`
  does not grow with the number of paths, so the file holding it does not either.
- **`words.def` is the one file that may exceed 300 lines.** It is data, not code,
  and splitting a numbering whose meaning is registration order is the one split
  that could silently change what the program means. The first satellite made the
  same exception for `format.def` and gave the same reason.

Markdown is not C++; this file is not bound by the rule.

---

## 7. Milestones

Each milestone is a thing that **works and can be demonstrated**. No milestone is
"the parser is half done."

### M1 — `satl` exists and says how to use it  ← START HERE

The binary, the build, and the opening information. **No language at all.**

- `Makefile` (an index, with the build in `make_support/` — the first satellite's
  final restructure was right, start there rather than arriving there)
- `src/system_facts/version.hpp`, carried over: VERSION and REVISION as two numbers
  that move at different rates, arriving as `-D` on the two recipes that need them
  and **deliberately not in `CXXFLAGS`**, so the build stamp stays quiet
- `satl` with **no arguments prints the opening information**, which says at minimum:
  what this is, the version, and **`satl filename.satl`** as the way to run a file
- `satl --version` / `-V`, `satl --help` / `-h`
- `satl <file>` recognises the argument and reports honestly that the reader lands
  in a later milestone — it does not pretend
- exit codes: 0 for `--version` and `--help`, non-zero for a usage error
- **the startup budget is measured here**, with `satl` doing nothing, and the
  numbers go in `make_support/040-sources.mk` beside the decision they justify —
  not in a commit message, where nobody looks for them again

Done when: `satl` prints its opening information, `satl --version` prints a version,
and startup is measured and recorded.

**Landed 2026-08-26.** `Makefile` + seven fragments, `src/system_facts/version.hpp`,
`src/programs/opening.{hpp,cpp}`, `src/programs/main.cpp`. Largest C++ file is 137
lines. Best of five runs of 200 invocations: bare `int main(){return 0;}` 1.74 ms,
`satl` 1.75 ms, `satl --version` 1.75 ms — so satl's own share of starting up is
about 0.01 ms, and `ldd satl` lists 6 shared objects. Two things changed against
the plan as written: `--help` exits 0 rather than 2 (a request correctly made is
not an error), and the first satellite's "119 shared objects" turned out to be 78
on this machine and is not repeated anywhere.

### M2 — the namespace trie and the path interner

- `src/satellite_words/words.def` — the X-macro list, written as a **tree**: each
  entry names its parent, and its position among that parent's children *is* its
  number. `satellite` is the root and is 1.
- the trie, the spelling interner, and `PathId`
- `words.hpp` as the consumer, with the static_asserts **in the header** so every
  future consumer inherits them. What they check, given per-parent numbering:
  every parent's children are dense from 1 with no holes and no duplicates, every
  named parent exists, and no node is its own ancestor.
- `satl --words` dumps the tree with each node's number — the registry has a
  consumer from its first commit, which is the one thing the first satellite did
  not do
- a test proving `satellite.console.display` walks to `1 1 1`,
  `satellite.random.normal` to `1 5 2`, and that both intern to stable `PathId`s

Nothing executes. This is the spine.

### M3 — the lexer

Tokens, spans, the reservation rule. Known words carry their `WordId` out of the
lexer; user-owned bare words carry their text.

### M4 — the arena AST and the parser

`uint32_t` node indices into a contiguous arena. No `shared_ptr` anywhere in the
tree. `satl --unparse file.satl` round-trips, which is how we know the parser is
right before anything can run.

### M5 — the error reporter

**Built before the evaluator, deliberately.** Codes, spans, a source excerpt with a
caret, notes with their own spans, and "did you mean" over the trie level that
failed. Every milestone after this one reports properly from its first commit.

Retrofitting this is exactly how the first satellite ended up with 177 bespoke
strings.

### M6 — resolve

Names to integer frame slots. Capsules, frames, the `SLOT_*` sentinels. Ported from
`env.hpp` with the resolved data in a side-table indexed by arena node id, not
`mutable` on the node.

### M7 — the value model and closure compilation

`Value` (40 bytes, the static_assert comes too), `Number`, `Str`. The arena AST
compiles to a closure tree. Module calls dispatch through `handlers[path_id]`.
Recursion depth is bounded here.

### M8 — hello world

```
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")
    satellite.return(satellite)
}
```

`satl hello.satl` runs it. Console with its printer thread. `satellite.main`,
`satellite.return`. **Startup measured again against M1's number.**

### M9 — scalars and control flow

`if` / `else` / `while` / `for`. `satellite.variable.bool`, `.number`, `.string`
and their methods.

### M10 — containers and the search power

`satellite.container.list`, `satellite.container.map`, and the search power ported
close to unchanged — comparator, walker, the ten-level ladder, and
`satellite.system.threshold`.

### M11 — the REPL and `satl-term`

The prompt, Ctrl-C, the exit words, and the GTK4 + VTE window binary. Two binaries,
same as before, same reason.

### M12 — threads

`satellite.variable.thread`. The arena makes the walk atomic-free; the Console
already keeps output lines atomic.

### M13 — windows

`libsatellite_window.so`, `dlopen`ed on first use. `satellite.window.new("title",
800x600)`. Marshalling to the UI thread is satellite's job, never the user's.

### Later, in no fixed order

`satellite.variable.file`, `.time`, `.date`; `satellite.random.*`;
`satellite.variable.float`; `satellite.variable.variant`; spacesuits;
`satellite.include` of other files; Satellite Orbit and the wire format.

---

## 8. Open questions

- **`satellite.variable.float` — "infinitely long in both directions."** The
  base-10⁹ `BigInt` gives us the integer half. The fractional half needs a decision
  about what "infinite" means when a program asks for a digit: lazy, or bounded by a
  precision dial like `satellite.system.threshold` is for search.
- **Time.** `high_resolution_clock` is the instinct, but `satellite.variable.time`,
  `.date` and `satellite.time.now()` need to agree on one clock and one epoch first.
- **Does the REPL share the closure compiler?** A REPL line is a program of one
  statement; compiling it to closures per line may cost more than it saves.
  Measure before deciding.
- **Error codes: numbered or named?** Numbered is testable and translatable. Named
  is readable. Probably both, the way words have an id and a spelling.
