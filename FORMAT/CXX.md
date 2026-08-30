# satellite — writing the C++

**This file is permanent.** It is what somebody needs in order to write C++ in this
tree and have it come out looking like the C++ already here: the house style, the
comment culture, how the build is edited, how a test is built, and the X-macro
registry mechanism that milestone 2 ports from the first satellite.

It is **not** a design document. [DESIGN.md](../DESIGN.md) says what the language is,
[PLAN.md](../PLAN.md) says how it gets built, [WORD_NUMBERS.md](../WORD_NUMBERS.md) is
the authority over every number. This file says how the code is written.

**§9 listed the ten things that were not decided anywhere and blocked M2.** All ten
were settled when M2 was written on 2026-08-28, and §9 now records each answer and
the file it lives in — which is the part to read before M3 inherits them.

---

## 1. The three rules everything else serves

**Try to build for 300 lines.** PLAN §3, softened by the author on 2026-08-28 —
it is a target to write toward and not a ceiling that fails a build. What it is for
is the shape a file comes out in: *"splitting a file after the fact preserves its
shape; writing to a target changes the shape."* Aim at it from a file's first
commit. Going over is not wrong; getting long because nobody was aiming is, and so
is splitting at a seam chosen to satisfy an arithmetic rather than a subject.
`words.def` is exempt outright, because it is data and splitting a numbering whose
meaning is registration order could silently change what a program means.

An umbrella header is a legitimate answer to it. A dispatch table is a
legitimate answer and a chain of `if` arms is not.

**One place per fact.** The `Makefile` is an index over `make_support/`;
`install.sh` is an index over `install_support/`; `README.md` holds no fact of its
own. When you find yourself writing something down twice, one of the two is wrong and
nobody will know which.

**Prose may explain a number. It may never be the only place the number lives.**
Inherited verbatim from the first satellite's `format.def`. The test for whether a
fact belongs in compilable data rather than in a comment is *whether anything breaks
if it is wrong.* If yes, a `static_assert` has to be able to see it.

---

## 2. The file skeleton

Every header:

```cpp
#pragma once

// <One line saying what this file is.>
//
// <Why it is this way. The load-bearing paragraph, often with an ALL-CAPS
// lead-in naming the finding. See §3.>

#include <string>

namespace satellite {

// ...

} // namespace satellite
```

Every `.cpp`:

```cpp
// <One line.> See programs/opening.hpp.
//
// <Why this half is separate from the header.>

#include "programs/opening.hpp"      // own header first, on its own

#include "system_facts/version.hpp"  // then this tree, blank line between

#include <cstdio>                    // then the standard library
#include <string>
#include <vector>

#include <unistd.h>                  // then POSIX, last

namespace satellite {
```

Hard points:

- **`#pragma once`, never an include guard.** Every header in the tree uses it.
- **Includes are spelled from the top of `src/`** — `"system_facts/version.hpp"`, never
  `"../version.hpp"`. That is what the `-I$(SRC)` on the compile line is for, and
  `060-compile.mk` explains at length why it is a literal on the recipe rather than
  part of `CXXFLAGS`. The path a header is included by is a property of the header,
  not of whoever reached for it.
- **Blank line between include groups**, in the order above.
- **`namespace satellite { ... } // namespace satellite`** — the closing comment is
  always there. Nested namespaces use the C++17 form: `namespace satellite::format {`.
- **File-private helpers go in an anonymous `namespace { ... } // namespace`**, as
  `main.cpp` does for `readable`, `not_yet` and `usage_error`.

## 3. Naming and formatting

| | |
|---|---|
| functions, variables, files | `snake_case` — `version_line`, `usage_text`, `opening.cpp` |
| types | `PascalCase` — `Path`, `Kind`, `Word`, `ExitStatus` |
| enum constants, macros | `SCREAMING_CASE` — `EXIT_FINE`, `SAT_WORD` |
| constants | `kPascalCase` — `kPaths`, `kVariadic`, `kPathCount` |
| private members | trailing underscore — `items_`, `nodes_` |
| namespace | `satellite`, nested as `satellite::<module>` |

- **Four spaces**, never tabs. Roughly **80 columns**; comments wrap there.
- **Function opening brace on its own line.** Control-flow braces on the same line.
- **Single-statement `if` bodies drop the braces** when they fit on one line.
- **Pointer and reference bind to the name**: `const char *program`,
  `const std::string &path`.
- **Module directories are spelled out** — `system_facts`, `satellite_words`,
  `abstract_syntax_tree` and not `ast`, `lexical_analyzer` and not `lex`. File names
  inside a module may stay short. `030-directories.mk` states this rule.
- **C++20**, `-Wall -Wextra`, `-O3` by default. `OPT` is the knob; `CXXFLAGS` is not.

## 4. The comment culture — the thing that makes this code look like this code

This is the most distinctive property of the tree and the easiest to get wrong.
**Comments here state a finding and where it came from, not an intent.** A comment
that says *what* the code does is noise; a comment that says *what went wrong once*
is the reason the file is shaped this way.

Three patterns, all real:

**An ALL-CAPS lead-in names the finding, then the paragraph proves it.**

```cpp
// R_OK and not F_OK, because "it is there" and "I may open it" are different
// answers and only the second one is useful to somebody about to be told their
// file cannot be run. access() rather than a stat of the mode bits, because
// access() asks the kernel the question with THIS process's real ids instead of
// reconstructing the answer from permissions and getting it wrong on an ACL.
```

**A number arrives with its date and its machine.**

```cpp
// MEASURED HERE, 2026-08-26, rather than quoted. The first satellite's source
// says gtk4 and vte pull "119 shared objects"; on this machine its satl-term
// resolves 79 and maps 78 (`ldd | wc -l`, and LD_DEBUG=libs counted by
// "calling init:"). 119 is not this machine's number and is not repeated.
```

**A past failure is named concretely, so nobody re-introduces it.**

```cpp
// ONE FUNCTION, read by --version and by the opening information both, so the
// two cannot disagree about what is running. The first satellite had them as
// separate literals and they drifted: the prompt still said 0.1 long after the
// language said 002, and nothing in the build had any way to notice.
```

Corollaries worth stating because they are easy to violate:

- **Say what is deliberately absent and why.** `060-compile.mk` documents a pattern
  rule that nothing currently reaches, because its absence would be the confusing
  failure later.
- **Measurements go beside the decision they justify**, in the file that makes the
  decision — never in a commit message, where nobody looks for them again.
- **Never write a comment that restates the code.** `// increment i` is not the
  register this tree writes in.

## 5. Adding a module to the build

The build is ten fragments under `make_support/`, included by name in numeric order
from the `Makefile`. Order is load-bearing in four places and each fragment says so
at its top — 065-tests.mk is read before 070-clean.mk so `clean` can name `$(TESTBINS)`.

**M2 used that line on 2026-08-28 and it is now live**: `WORDS = $(SRC)/satellite_words`
sits with the other module variables at the top of `030-directories.mk`, which is
where `TESTS` and `TESTNAMES` were added beside it. The prediction held — adding the
module was the two edits below, plus the test fragment, which is a target and not a
module.

**M4 added TWO modules on 2026-08-30 and the count came out at exactly the two
edits again** — `TREE = $(SRC)/abstract_syntax_tree` and `PARSER = $(SRC)/parser`
in `030-directories.mk`, then eight sources and four headers in `040-sources.mk`.
Nothing else. Both microarchitecture variants and both flag stamps came along
through the pattern rules with no rule written for them, which is what those rules
were put there in advance for. *The example this section has used since it was
written — `abstract_syntax_tree` and not `ast` — stopped being hypothetical that
day.*

So adding a module under `src/` is exactly three edits:

1. **`030-directories.mk`** — uncomment `WORDS = $(SRC)/satellite_words`.
2. **`040-sources.mk`** — add each `.cpp` to `SATL_SRCS`, and each `.hpp` to `HDRS`.
   Both lists are **spelled out one file per line, never wildcarded**: *"a wildcard
   here would quietly compile a file somebody is part way through writing, and quietly
   stop compiling one that got renamed."*
3. Nothing else. `SATL_OBJS` derives from `SATL_SRCS`, `SATL_HASWELL_OBJS` derives in
   `045-microarchitecture.mk`, and **both microarchitecture variants are already
   covered by pattern rules** in `060-compile.mk`:

```make
$(SRC)/%.o:         $(SRC)/%.cpp   →  $(CXX) $(CXXFLAGS) -I$(SRC) -c -o $@ $<
$(SRC)/%.haswell.o: $(SRC)/%.cpp   →  ... $(MARCH_HASWELL) ...
```

Explicit rules exist only for the two objects that bake in `VERSION_DEFS`
(`main.o`, `opening.o`) — an explicit rule outranks a pattern. **The haswell
pattern rule was unreached until M2 and is reached now**: `dump.cpp` was the third
source added to `SATL_SRCS`, and `dump.haswell.o` is built by that rule and no
other, which is exactly what it was written in advance for.

Two things not to break:

- **`.cxxflags-stamp` and `.cxxflags-stamp-haswell`.** `CXXFLAGS` is not a
  prerequisite of anything, so without these `make OPT=-O3` over an `-O2` tree
  recompiles nothing and yields a mixed binary. Two stamps, not one, because one file
  can only describe one variant.
- **`070-clean.mk` names what it removes.** A new build artefact that is not a
  `$(SRC)/*/*.o` needs adding there. Never `rm -rf` a directory.

## 6. Tests

**Ported at M2 on 2026-08-28.** Until then this section opened *"there is no test
infrastructure in this tree -- not a target, not a directory, not a harness"*, and
that was literally true. There is now `make test`, `make_support/065-tests.mk`, and
**five suites — `tests/words_test/` (M2), `tests/lexer_test/` (M3),
`tests/parser_test/` (M4), `tests/satc_test/` (M4.5) and `tests/reporter_test/`
(M5)**. Everything below is what was ported, and why.

`old_versions/first_satellite/make_support/120-tests.mk` plus `TESTNAMES` in its
`070-directories.mk` is the original. The shape:

- **`TESTNAMES` is the single place a test is declared to exist.** Everything else —
  the per-test source list, the binary list, the phony aliases — is derived from it
  with `$(foreach ...)`.
- **Every `.cpp` in a test's folder is part of that test**, by wildcard, so splitting a
  test needs no Makefile edit and a new piece cannot be forgotten.
- Headers are a separate wildcard because they are a *dependency*, not an input.
- The harness is **three functions and a counter**, no framework:

```cpp
int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}
```

`main()` calls each section, prints the figures, and returns non-zero if
`failures`.

**The lesson that fragment records is worth more than the code.** The test binary list
*was* spelled out by hand, and two tests were added to the run list and to neither
build list — so `make test` **ran binaries nobody had built** and reported PASS from
stale objects. *"That is the worst failure a suite has: not a red line, a green one
that is out of date."* Derive the list; never hand-copy it.

**The model to copy for M2 is `format_test`**, because it is the one test whose
subject is a registry. Most of it runs at *compile* time — the `static_assert`s in the
header fire on a duplicate id or a hole — and what is left for run time is the part a
`static_assert` cannot reach: that the worked examples in the documents still match
the tables.

That is exactly M2's test: **`satellite.console.display` must walk to `1 5 1` and
`satellite.random.normal` to `1 7 2`**, because those numbers are claims WORD_NUMBERS.md
makes in prose and the test is how we know the transcription did not drift.

**And it went further, because two rows are not a transcription check.**
`tests/words_test/authority.cpp` **opens WORD_NUMBERS.md at run time**, parses §2.2
and walks all 222 of its paths, then §2.3's nine spellings. That is unusual and it
is the point: WORD_NUMBERS.md is not documentation *about* the numbering, it **is**
the numbering, so checking the code against it is §1's third rule pointed at the
code. Hard-coding the 222 rows into C++ would have made a second place they live.

**The failure it exists for cannot be caught any other way.** `words.def` numbers by
POSITION, so a row left out does not leave a hole — it silently renumbers every
sibling after it, and both files stay internally consistent. **Verified by mutation
on 2026-08-28:** deleting `satellite.console.typed()` `1 5 5` from `words.def`
**compiles clean**, every `static_assert` passing, and the test then reports the
missing row plus the four siblings it shifted. A green build and a wrong language.

Two rules came out of writing it, and both are the stale-binary lesson again: **a
test whose subject is a document must fail loudly when it cannot read that
document**, never skip; and **the counts are checked before the rows** — 222 rows,
219 numbers, 3 aliases, 35 `(0)` markers — because a parser bug that silently read
half the table would otherwise report PASS over the half it read.

### 6.1 A test that links objects — what M3 added

*(2026-08-29.)* `words_test` **links nothing**, and §6 above reads as if that were
the shape of a test here. It is the shape of a test whose subject is `constexpr`
data. `lexer_test` is the other kind, and adding one is two lines:

```make
LEXER_TEST_SRCS = $(LEXER)/lexer.cpp $(STRING)/satellite_string.cpp

$(TESTS)/lexer_test/lexer_test: $(lexer_test_SRCS) $(lexer_test_HDRS) \
                                $(LEXER_TEST_SRCS) $(WORDS)/words.def $(HDRS) \
                                example/hello_world.satl .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/lexer_test -o $@ \
	    $(lexer_test_SRCS) $(LEXER_TEST_SRCS)
```

Three things in that rule are the point:

- **Name the module sources, never `$(SATL_OBJS)`.** Linking the interpreter's
  objects would drag `main.o` and its window handover into a test binary, and a
  test that begins by deciding whether to open a GUI is a test that hangs on a
  build machine.
- **A file the test READS is a prerequisite**, exactly as `WORD_NUMBERS.md` is for
  `words_test`. `lexer_test` lexes `example/hello_world.satl`, so editing the
  acceptance program re-runs the test that reads it.
- **`.cxxflags-stamp` is not optional**, for the reason 065-tests.mk records at
  length: without it `make OPT=-O0 test` re-runs the binary the previous build
  left.

And the rule from `words_test` carries over unchanged and is worth repeating
because it is the one most easily broken: **a test whose subject is a file must
fail loudly when it cannot read that file, never skip.** `lexer_test`'s span
section calls `check(false, ...)` and returns rather than passing over a missing
`example/`.

### 6.2 A file that must NOT parse is also an input — what M4 added

*(2026-08-30.)* `parser_test` names **all six** programs in `example/` as
prerequisites, and four of them are the milestone's done-when while **two are
checked as files that do not parse**, with the reason pinned to a line number.
That is not a smaller claim than the four — it is the same claim from the other
side, and it needs the same prerequisite line:

```make
$(TESTS)/parser_test/parser_test: ... $(wildcard example/*.satl) .cxxflags-stamp
```

**A file that starts parsing is a finding and not a pass.** Without the
prerequisite, correcting `example/class_test.satl` would leave the test that
asserts it is broken reporting `ok` from a binary built before the fix — the
stale-binary lesson again, arriving through a file nobody thought of as an input
because the test wants it to fail.

**And the round-trip is the form of the check, not a check among them.** Nothing
in this language runs until M10, so a tree cannot be verified by running it; it can
be verified by printing it back and printing that back again. `unparse.hpp` states
what that does and does not promise, and the short version is that **it is a
fixpoint and not equality with the input** — comments are discarded, blank lines
were never tokens, and a redundant bracket does not survive. A test written
against equality would have to be weakened every time the printer got better.

### 6.3 A test that renders what nothing produces — what M5 added

*(2026-08-30.)* Every rule above is about testing a pass against real input.
`reporter_test` needs one more, and it is the rule that keeps a shape honest
while half of it has no producer: **build the input by hand and render it.**

DESIGN §9's diagnostic has four fields and one of them, `vector<FrameRef>`, has
no producer until M9's evaluator. Three more arms of the renderer are reachable
only by mistake — a note with no span, a span past the end of its text, a hole
with no argument. All four are drawn by `render()`, which §9 says must be the
only place anything is drawn, so **a branch nothing exercises is a branch that
does not work** and will be discovered by whoever first raises a real one.

What makes it possible is a module boundary rather than a testing trick: the
reporter takes a `Span` — three integers — and the text those integers index, and
knows nothing about a token or a tree. A test can therefore construct any
diagnostic the type can hold. That is worth designing for on purpose.

**And it is not a substitute for the other kind.** The same suite links the lexer
and the parser and asserts one code per row of `errors.def` those two own,
through `parse()`, because a registry adds a defect bespoke strings did not have:
a site raising the *wrong row*, which renders perfectly and describes a different
problem. `MILESTONES/M5.md` §5 is the argument.

## 7. The X-macro registry — the mechanism M2 ported

`old_versions/first_satellite/src/bytecode_format/` is 1133 lines across five files
and it is what was ported and finished on 2026-08-28. Read it beside
`src/satellite_words/`, which is what it became.

**There is no generator and no build step.** Every consumer defines the macros,
includes the `.def`, and undefines them. The `.def` opens by defining each macro to
nothing if it is not already defined, so a consumer may expand one list and ignore the
rest:

```cpp
#ifndef SAT_WORD
#define SAT_WORD(id, ident, text)
#endif
```

Each list is then expanded several ways from one source:

```cpp
// an enum, so a name is spelled once
enum class Word : uint64_t {
#define SAT_WORD(id, ident, text) ident = id,
#include "bytecode_format/format.def"
};

// a switch, which is the cheapest duplicate detector C++ has:
// two rows sharing an id is `error: duplicate case value`, at compile time,
// with both identifiers named. An enum alone accepts the duplicate silently.
constexpr const char *name(Word w)
{
    switch (w) {
#define SAT_WORD(id, ident, text) case Word::ident: return text;
#include "bytecode_format/format.def"
    }
    return "<unknown word>";
}

// a constexpr array, which is what the static_asserts run over
constexpr uint64_t kWordIds[] = {
#define SAT_WORD(id, ident, text) id,
#include "bytecode_format/format.def"
};
```

**The identifiers are deliberately not the bare words.** `and`, `or`, `true` and
`false` are C++ keywords or alternative tokens; pasting them into an enum name is a
trap. The quoted string is what the language calls the thing and the identifier only
has to be unique — hence `TRUE_`, `AND_`.

**The umbrella pattern.** `format.hpp` outgrew 300 lines and became three parts
included in an order that compiles, with the include path and its meaning unchanged.
`format.def` is **not** split — it is the permanent exception, and each part
re-expands the lists it needs.

**M5 IS THE SECOND USER AND IT ENCODES ITS NUMBERS THE OTHER WAY ROUND.**
*(2026-08-30.)* `src/error_reporter/errors.def` is the same mechanism —
`SAT_CODE(number, ident, severity, text)`, expanded five ways, no generator —
with one deliberate difference: **the number is a column here and a position in
`words.def`.** A word's number is what a program *means*, so DESIGN §4.3 freezes
registration order and a duplicate becomes unrepresentable; an error code is what
a person *looks up*, so it must survive a deletion, leave gaps for a reserved
block, and be assignable out of order. Opposite requirements, opposite encodings.
What replaces "unrepresentable" is a `static_assert` that the numbers **ascend**,
plus the switch trick above, which catches a duplicate as `error: duplicate case
value` naming both rows.

**It also shows what a `.def` can check that a column cannot.** A sentence's
arity is the highest `{n}` in its text, counted at compile time rather than
declared beside it — so there is no column to go stale — and
`errors::make<Code::X>` is a template on the code, which makes the arity a
`static_assert` **at the call site**. `holes_are_dense()` is the one that earns
its place: a sentence rewritten from `"{1} under {2}"` to `"{2}"` keeps arity 2
and silently ignores the first argument, and no reader of either half alone can
see it.

**What not to carry over.** The `bytecode_format` name is wrong — it is a word-and-path
registry, and its new home is `src/satellite_words/` (PLAN §7). The four-segment
`SAT_PATH(ident, s1, s2, s3, s4, arity)` limit goes: `arguments.machine.threads` is six
numbers deep and the old macro could not have expressed it. And the `arity` column
stops being needed, because under WORD_NUMBERS §1.3 the number already carries the
call shape.

## 8. The `static_assert` style

The asserts live **in the header, not in the test**, so every future consumer inherits
them. That is the single most important decision in the first satellite's registry and
it is the one to copy.

Each is a named `constexpr` predicate plus an assert with a message that says what is
wrong and what to do:

```cpp
static_assert(detail::all_distinct(kWordIds),
              "format.def: two words share an id — ids are frozen, never reused");
static_assert(detail::ascending(kWordIds),
              "format.def: SAT_WORD rows must ascend — append, never insert");
```

**Predicates are named rather than folded into the assert**, and the comment above
`all_within` says why in the sharpest possible way: the assert it replaced read
`sizeof(kKindIds) / sizeof(uint64_t) <= 16` under the message *"a kind is four bits"*.
That counts **rows**, not values. Seven rows numbered `{0,1,2,3,4,5,99}` pass it and
encode a kind that does not fit. *"A bound stated as arithmetic on a sizeof is exactly
what hid the bug."*

**What M2's asserts must check**, given per-parent numbering (PLAN §8):

- every parent's children are dense from 1, no holes, no duplicates
- every named parent exists
- no node is its own ancestor
- an alias resolves to a number that exists

These are the same four checks the numbering was validated against by hand on
2026-08-27, when WORD_NUMBERS.md §2.2 went to 215 entries and all four came back zero.

**Written on 2026-08-28, three of the four turned out not to need an assert**, and
that is the encoding paying rather than a corner cut: a number is a POSITION, so
holes and duplicates cannot be expressed; an alias names an IDENTIFIER, so a bad one
is an unknown enumerator. Only *no orphans, no node is its own ancestor* was left to
assert. **Say which is which in the header** — `words_invariants.hpp` does, because a
reader who counts four here and one there will conclude three were forgotten.

**And one thing the header must say out loud**, or it promises more than it keeps: the
asserts cover the **frozen** half only. User capsules and spacesuits are numbered at
parse time (WORD_NUMBERS §3) and no compiler can see them.

---

## 9. What blocked M2, and what each answer turned out to be

**All ten were settled on 2026-08-28, when M2 was written.** This section used to
open *"the following is not documented anywhere and each would otherwise be settled
by whoever types first"* — that was its whole purpose and it worked, so what
replaces it is the answers rather than the questions. **Read this before M3**, which
inherits every one of them.

**1. How is `words.def` written as a tree?** `SAT_NODE(parent, ident, text, kind)`,
and **the number is not a column** — it is the row's POSITION among its parent's
`SAT_NUMBERED` siblings, computed at compile time in one forward pass. A parent must
therefore be declared before its children, and that is `parents_come_first()` rather
than a convention. The `// 1 5 1` ending each row is a comment nothing reads.

**2. Call shapes at two depths.** Not a conflict, and WORD_NUMBERS §1.3's definition
of `(0)` on 2026-08-28 had already settled it before the file was written: **a word
reached bare holds its shapes as children** (`include()` is `1 1 0`, a child of
`include`); **a word only ever called does not exist apart from them**, so its shapes
are siblings (`input()` is `1 5 2`, beside `display`). One rule, two appearances.
`match_shape()` in `words_walk.hpp` looks in both places, in that order, and nothing
else in the module has to know which kind a word is.

**3. What is a `PathId`?** `uint32_t`, an index into the node table. `0` is no path;
`1`–`254` are the language's frozen words in `words.def` order; a user's names are
allocated from `255` upward at parse time. `is_language_word(id)` is the boundary,
and it is the predicate anything about to write a `PathId` down must ask first.

**4. `(0)` forms.** A `SAT_BARE` row whose text is exactly `"()"`, taking position 0
and **not advancing its parent's counter** — which is what keeps a parent's numbered
children dense from 1 whether it has a bare shape or not. The kind and the text both
say it, and an assert requires them to agree.

**5. Aliases.** `SAT_ALIAS(ident, text)`: a second spelling of a node, written
relative to that node's **parent**, free to carry a dot. All nine live in
`words.def`. Two things fell out: an alias takes no number, so **properties 2 and 4
of M2 hold by construction and by the compiler**; and the alias match must end at
`.`, `(` or the end of the path, because `argument` is a declared spelling and is a
prefix of `arguments`.

**6. The digest.** `constexpr` FNV-1a over **the numbering** — each node's parent,
number, kind and text, then every alias — and not over the file's bytes.
`words_digest.hpp` carries the argument and SATC.md §6 now records it: every input to
the numbering is in one file, so §6's worry does not arise, and hashing bytes would
invalidate every cached `.satc` on the machine when a comment was rewritten.

**7. Live counter beside a `constexpr` table.** The frozen table stays `constexpr`
and is never copied. `words::Words` holds one array of counters seeded from it and a
vector that stays empty until a name is defined — so a program that never defines one
pays nothing. **M2 owns a real allocator, not a stub** (PLAN §8.1 asked; the answer
is recorded in `words_runtime.hpp`), and **its caller arrives at M4.**

**8. Error handling before M5 exists.** `walk()` returns a `Walk`: the id, and when
there is no id, **which segment failed and which node it failed under**. Neither a
throw (DESIGN §9.1) nor a bare null — `under` is the load-bearing field, because
§4.6's *"did you mean `console`?"* is edit distance over one node's children and that
node is the one it names. M5 can be built against this without the signature moving.

**9. The C++ names.** `namespace satellite::words`, and: `NodeId`, `PathId`,
`SpellingId`, `Node`, `Alias`, `Walk`, `WalkError`, `Words`. No `Trie` and no
`Interner` — the trie is the node table and the interner is a `constexpr` array, and
neither earned a type.

**10. Whether M2's interning is threaded.** **It is not, and that is a decision.**
PLAN §4.5.1 already argued threading the fixed table at startup is a guaranteed loss;
the M2 startup measurement (PLAN §1) is why it is not close — `constexpr` tables land
in rodata and nothing runs before `main()`. **The crossover §4.5.1 wants measured is
about the walk over a USER'S SOURCE**, which does not exist until M3, so the
measurement was never M2's to take and M3 is where it can first be taken.

### What M2 found that this section could not have asked for

- **Only one of PLAN M2's four properties needed a `static_assert`.** Choosing
  position-as-number made "no holes" and "no duplicates" unrepresentable, and naming
  an identifier made "no alias points at a number that does not exist" a compiler
  error. `words_invariants.hpp` says which is which — because a reader who counts
  four in the plan and one in the header will otherwise conclude three were
  forgotten — and adds eight the encoding needs instead.
- **The transcription is the one thing no assert can reach**, and §6 above records
  the mutation test that proves it.
- **A predicate is easy to write about the wrong thing**, which is §8's whole
  warning, and it happened here on the first attempt: `argument_rows_have_no_spelling`
  reasoned about a row's *kind* when the honest property was about its first
  *character*, and it fired on all 38 bare rows. It is `spellings_match_texts()` now.

**Also unscheduled rather than undecided:** [SCRATCH.md/MILESTONE.md](../SCRATCH.md/MILESTONE.md)
lists everything with no milestone at all, including the test infrastructure §6 says
does not exist.

---

## 10. The checklist, before a file is committed

- [ ] aimed at 300 lines, and split by subject if it was split at all
- [ ] `#pragma once`, includes spelled from `src/`, grouped and ordered per §2
- [ ] `namespace satellite`, closed with its `// namespace satellite` comment
- [ ] a file-top comment saying what it is and **why it is this way**
- [ ] every number in it carries where it came from and when
- [ ] anything that would break if wrong is a `static_assert`, not a comment
- [ ] added to `040-sources.mk` — `SATL_SRCS` and `HDRS`, spelled out
- [ ] `make` builds all four binaries clean under `-Wall -Wextra`, and `make test` passes
- [ ] the new thing has a consumer **in the same milestone it is written** — the
      first satellite shipped three commits where the registry had none, which is
      how four defects accumulated behind a guarantee nothing checked

*Companions: [DESIGN.md](../DESIGN.md) — what the language is.
[PLAN.md](../PLAN.md) — §3 the line rule, §4 the build, §8 the milestones.
[WORD_NUMBERS.md](../WORD_NUMBERS.md) — the authority over every number.
[SATC.md](../SATC.md) — the cached form. **Built at M4.5**, and the digest §9 once called unchosen was settled at M2: over the numbering, not over `words.def`'s bytes.
[LAYOUT.md](../LAYOUT.md) — every file in the tree.*
