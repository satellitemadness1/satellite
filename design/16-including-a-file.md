*satellite design docs, §16 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§15](15-bootstrapping.md), On: [§17](17-the-abstract-machine.md).*

---

## 16. Including a file

### The name of the thing is `spaceship`

**Decided 2026-08-18.** A unit of includable satellite code is a **spaceship**. This was the
one genuinely open question in this document (§13), and it is now closed: the word is settled
and the rest of this section uses it.

It earns the name the way `capsule` and `spacesuit` earned theirs — by being the thing the
metaphor already implies. A capsule holds one computation. A spacesuit is worn by one occupant
and is the thing with an inside and an outside, which is the whole of `satellite.protected`
and `satellite.public`. A **spaceship is the vessel that carries both**, which is exactly what
a file of capsules and spacesuits is. The metaphor was already three-quarters built; this is
the piece it was missing.

It also fixes what was wrong with `payload`, the proposal it replaces: a payload is *cargo*,
and an included file is not cargo. The file **contains** capsules and spacesuits — it is the
vessel, not the thing carried — so `payload` had the relation backwards, and would have left
the language with a word for the contents and still no word for the container.

**`spaceship` does not rename `module`.** Every occurrence of "module" in this section is §5's
and §7's settled sense of a namespace of language-owned functions, which is what
`satellite.time`, `satellite.file` and `satellite.window` are. A spaceship is a *file*; a
module is a *namespace*. `satellite.window` is both — a module implemented as a native
spaceship — and that is a sentence the two words can now express, which is the test `module`
alone failed.

**The word does not enter the grammar.** `satellite.include(x)` is unchanged and gains no new
spelling; `spaceship` is the word for error messages, documentation and internal type names,
not a keyword. So it costs the parser nothing and §1's reservation rule is untouched — a user
may still name a variable `spaceship`, exactly as they may name one `time`.

One thing it collides with, named here so nobody rediscovers it: C++ calls `operator<=>` the
spaceship operator. That is a collision in the *implementation's* vocabulary only. §3.5 fixes
satellite's two-character operators at exactly `== <= >= !=`, satellite has no `<=>` and is
not getting one, and the C++ in this repo does not define `operator<=>` anywhere — verified.

Three candidates were rejected on the way here, each for a reason that is about satellite
rather than taste:

| candidate | rejected because |
|---|---|
| `module` | it is Python's word, and borrowing it imports every expectation Python attaches to it — packages, `__init__`, dotted import paths — none of which this section's flat namespace provides. It is also **already in use in this document for something else**: §5's dispatch table and §7 call a namespace of language-owned functions a module, which is what `satellite.time` and `satellite.file` are. One word for a namespace and for a file is one word too few. |
| `include` | it is already the **verb**. `satellite.include(x)` says what is done; the noun cannot be the same word, or `include an include` is the best the documentation can do. |
| `library` | `satellite.library` is the global variable registry (§6) **and** one of the keys in §5's segment-1 dispatch table. The word is spoken for twice over, and reusing it would make `satellite.library` mean two unrelated things at the same position in a path. |

A fourth, `payload`, was the standing proposal until 2026-08-18 and lost to `spaceship` for
the reason given above: it names the cargo, and what needed a name was the vessel.

### Status: **built**, 2026-08-19

`satellite.include` had its syntax from §2 and none of its meaning for the whole of the
project until now: the parser accepted any expression and the evaluator skipped the node.
Both prerequisites cleared first — file I/O in §8.3.1, because including a spaceship means
reading a file, and the **name**, settled above on 2026-08-18. It then landed in two pieces:

| piece | when | what it is |
|---|---|---|
| spans carry a file id, and a `SourceMap` | 2026-08-18 | so an error names the spaceship it is in |
| the load phase | 2026-08-19 | `loader.cpp`, `loader.hpp`, `loader_test.cpp` |

The spans went first because they are the only piece with no dependency on the rest — worth
having with one file, and what every later error message rests on. See "Spans have to name a
file" below.

**`loader.cpp` is 314 lines, 184 of them code**, which is the measurement this section's
design was making a claim about. Merging before resolve() rather than teaching resolve()
about files is what bought that: the loader concatenates declarations and knows nothing about
what a capsule means. `resolve()` and the evaluator needed **no changes at all** for the merge
itself — their only edits were to name both files in an "already defined" error, which is a
message improvement rather than a mechanism.

Everything below this line is now description, not design, with one exception: the native
`.so` half of a language-owned spaceship is still M7. `satellite.include(satellite.window)`
today resolves to `window.satl` in the installed library directory and reports that it cannot
find one.

§1 decides all three spellings without any new rule:

| form | means |
|---|---|
| `satellite.include(satellite)` | the runtime. Ceremony, and it stays ceremony (§2). |
| `satellite.include(satellite.window)` | a **language-owned** spaceship, possibly backed by a native `.so` |
| `satellite.include(my_parser)` | a **user-owned** spaceship |

A bare name is user-owned, so it names the user's spaceship; a satellite-rooted path is
language-owned, so it names ours. Nothing had to be invented.

**Not `satellite.gtk`.** GTK is somebody else's library, and a name that leaks it into the
language surface is wrong the day the implementation changes — every program in existence
would say a word that is no longer true. §8.3 already named the type
`satellite.variable.window`, so the module is `satellite.window` and GTK stays behind the
shim where it is an implementation detail.

### Where it happens: a load phase between parse and resolve

The includer's tree is parsed, its `Include` items are walked, each named spaceship is loaded
and parsed recursively, and every declaration is **merged into one Program**. Only then does
resolve() run, over the merged whole.

This is the decision that makes everything else cheap. resolve() already handles forward
references and mutual recursion across a whole program (§6's collect-then-walk), so a capsule
in one file calling a capsule in another needs no new machinery at all — it is the same
problem resolve() was already built for. The evaluator changes not at all.

- **Search order**: the directory of the *including* spaceship, then `$SATELLITE_PATH`, then
  the installed library directory — which is `library_path()`'s three tiers (§9's "Finding the
  installed library") with the including spaceship's own directory in front. Includer-first is
  what lets a project's own spaceships find each other with no configuration, which is the
  case that has to be frictionless.
- **Included once**, keyed by canonicalised path. Without it, `a` including both `b` and `c`
  where both include `d` is a duplicate-capsule error rather than a working program.
- **Cycles need no separate check.** Include-once makes `a -> b -> a` terminate on its own:
  by the time `b` asks for `a`, `a` is already loading and is skipped. This is what C's
  include guards do, and it is a feature rather than an error — two spaceships that genuinely
  need each other's declarations are what resolve()'s forward references are for.
- **Flat namespace**, and a name defined twice across spaceships is an error naming *both*.
  §1 makes a user's capsule bare, and qualifying a name by the spaceship it came from would
  need a second naming rule for no benefit until the library is large. Revisit when it is.
- **Top-level statements in an included spaceship run**, in include order, before the
  includer's own. That is the spaceship's body, and one that sets up globals needs it.

Four things the build settled that the list above had not:

- **A language-owned name skips the user's directory.** The search order above reads as one
  list, and it cannot be: if `window.satl` sitting next to a program could satisfy
  `satellite.include(satellite.window)`, a user file would be answering to a language-owned
  path and §1's rule that a satellite-rooted name is *ours* would hold only until someone
  picked an unlucky filename. So the includer's directory is searched for a bare name and
  skipped for a satellite-rooted one.
- **The entry point is marked seen before it is parsed**, not after. Otherwise a spaceship
  that includes *itself* loads twice — once as the entry point, once through its own include
  — and reports every capsule in it as already defined. The self-cycle is the smallest cycle
  there is and it has to terminate like any other.
- **Errors from an included spaceship come out before the includer's**, matching the order
  the items merge in. A reader fixes the dependency before the thing that depends on it.
- **The `Include` node stays in the merged Program** rather than being stripped once
  followed. The evaluator skips it, as it always did, so the merged tree stays a faithful
  record of what each spaceship actually said.

**The REPL is line-at-a-time, so an include does not outlive its line.** Typing
`satellite.include(helper)` at the prompt loads `helper.satl` and resolves it, and the next
line is a fresh program that has never heard of it. This is not an include limitation: a
capsule *defined* at the prompt does not survive to the next line either. Making the REPL
accumulate a program across lines is its own design question and is not part of §16.

### Spans have to name a file, and it costs nothing — **done**

`format_error` used to take one source string, because a program was one file. The moment a
program spans files, a runtime error inside an included capsule prints line N against the
**wrong file's text** — a language whose errors lie about where they are is unusable, and for
a self-hosting compiler it is fatal.

So `Span` gained a file id and the interpreter gained a `SourceMap` holding one text per
loaded spaceship. The id was free, and the arithmetic held exactly:

```
Span before:               3 x size_t                  = 24 bytes
Span with a file id:  2 x size_t + 2 x uint32          = 24 bytes
```

**Verified**: `ast_test` prints `Value=40 Expr=96 Stmt=200 Span=24` and a `static_assert` in
`ast.hpp` fails the build if `Span` ever leaves 24 bytes on 64-bit. A line number does not
need 64 bits and neither does a file id, so `Expr` stayed 96 and `Stmt` stayed 200.

The `SourceMap` is internal C++ bookkeeping — a vector of texts indexed by that id — and is
not a language feature. It is emphatically **not** `satellite.container.map` (§8.6). The
distinction is not about which one exists: a `SourceMap` is C++ bookkeeping internal to the
loader, and would not become satellite's map even now that satellite has one.

Three things this landing settled that were not obvious from the plan:

- **`format_error` takes the `SourceMap`, not a text.** All three of them — parser, resolver,
  evaluator — used to be handed a source string by their caller, which is precisely the call
  that would pick the wrong one. Passing the map and letting each look up `span.file` makes
  the mistake unspellable rather than merely discouraged.
- **An unnamed source is a real case, not a fallback.** The REPL evaluates a line that came
  from no file, so `SourceMap` allows an empty path and errors then read `line 3` exactly as
  before. `span_location()` is the one place that chooses, so all three renderers agree.
- **The evaluator and resolver needed no changes at all**, which is the claim above about
  resolve() doing the work already, arriving one stage early. Neither ever constructs a
  `Span` — both only propagate the ones the parser built — so the file id flows through both
  passes untouched. The only `Span` literals in the interpreter are the two inside `span_of`
  and `span_join`.

The payoff does not wait for the loader: `satl --run` knows the path it opened, so an error
in a single-file program already reads `orbit.satl:3` instead of `line 3`.

### The surface: a bare name, and a quoted path — §19.1

This section settled include *semantics* and left the *surface* at one form: a bare
name, or a satellite-rooted one. §19.1 adds the quoted path —
`satellite.include("object/forge_object.satl")` — because the bare form looks in the
includer's own directory and nowhere below it, and a project laid out as a tree
therefore had no way to name most of its own files. None of the semantics above
changed: include-once is still keyed by canonical path, which is what makes
`../helper.satl` from one spaceship and `object/helper.satl` from another **one**
spaceship. §1 is completed rather than weakened, and the argument is that a string
literal was never a name, so it can carry a location without any name having to mean
a place.

### Native modules, and what they buy

A language-owned module may be backed by a shared object that the runtime `dlopen`s and which
registers `satellite.<name>.*` functions. `satellite.window` is the first, and the payoff is
larger than the window:

- GTK loads **only** when a program includes it. `satl --run hello.satl` keeps its 2.5ms
  startup (§9) and never pays the 23.4ms.
- `satl-term` — 188 lines of C++ — can then be written *in satellite*, because a
  terminal host is just a program that opens a window. It appears in neither column of §15's
  boundary table, which is the tell: it is C++ that no "cannot be written in satellite"
  argument defends, and it is exactly the kind of deletion the bootstrap plan exists for.

The mistake to avoid is binding GTK's API surface. §8.3 already settled the shape: a window is
a `shared_ptr<WindowHandle>` reached through accessor methods, so satellite sees a handful of
functions and the shim behind them is small. Exposing GTK wholesale through `dlsym` would be
thousands of lines of binding for a language whose whole naming rule is that it owns its own
surface.

### What is not decided

1. **Whether a path is a fourth form.** `classify()` takes three shapes, and a project laid out
   in directories needs a fourth: `satellite.include("declarations/objects/countries/china.satl")`.
   The first real user of this — a reference database of ~18 files across `object/`,
   `declarations/objects/`, and `declarations/types/` — cannot say that with a bare name,
   because this section's namespace is flat and the search order's first tier is the includer's
   own directory, which reaches siblings and nothing else. `$SATELLITE_PATH` does not rescue it
   either: §9 reads it as one directory, not a list. A string literal is the obvious carrier. It
   is already lexed, and it cannot collide with a `Name`, so all three existing forms keep their
   meanings unchanged.

   What it appears to cost is the sentence at the top of `loader.cpp`: that a name with an
   extension in it is a path, "which would make the include a statement about where a file sits
   rather than what it is called". That reads as an argument against paths and is really an
   argument that a **bare name** must not be one. Both can hold at once — a bare name says what
   a spaceship is called and is searched for; a quoted path says where it sits and is resolved
   against `directory_of()` the includer alone, skipping `$SATELLITE_PATH` and the installed
   library entirely, because a path that is searched is a path that does not mean what it says.
   Include-once needs no change: `canonical()` already collapses `./` and `..`, so
   `declarations/x.satl` from the root and `../x.satl` from a sibling are one spaceship.

2. **Whether the flat namespace survives it.** A path form is a way of *not* deciding. The
   directories organise the files while every capsule still lands in one namespace at merge
   time, which is what makes the change cheap. The day two spaceships want the same capsule
   name, the path will have told the loader where to look and nothing else, and this section is
   back where it started.
