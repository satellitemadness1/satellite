# satellite — writing the C++

**This file is permanent.** It is what somebody needs in order to write C++ in this
tree and have it come out looking like the C++ already here: the house style, the
comment culture, how the build is edited, how a test is built, and the X-macro
registry mechanism that milestone 2 ports from the first satellite.

It is **not** a design document. [DESIGN.md](../DESIGN.md) says what the language is,
[PLAN.md](../PLAN.md) says how it gets built, [WORD_NUMBERS.md](../WORD_NUMBERS.md) is
the authority over every number. This file says how the code is written.

**§9 is the part to read first if you are about to write M2**, because it lists what
is *not* decided anywhere and would otherwise be decided by accident at the keyboard.

---

## 1. The three rules everything else serves

**No C++ file exceeds 300 lines.** PLAN §3. From the first commit of every file, not
from the commit where somebody notices — *"splitting a file after the fact preserves
its shape; writing to a ceiling changes the shape."* `words.def` is the one exemption,
because it is data and splitting a numbering whose meaning is registration order could
silently change what a program means.

An umbrella header is a legitimate answer to the ceiling. A dispatch table is a
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

The build is eight fragments under `make_support/`, included by name in numeric order
from the `Makefile`. Order is load-bearing in three places and each fragment says so
at its top.

**M2's directory line is already written and commented out**, at the bottom of
`030-directories.mk`:

```make
# Declared here and empty until M2, so that adding the trie is one line in this
# file and one in 040-sources.mk rather than a hunt through the build:
#   WORDS  = $(SRC)/satellite_words
```

So adding `src/satellite_words/` is exactly three edits:

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
(`main.o`, `opening.o`) — an explicit rule outranks a pattern, which is why the
haswell pattern rule is currently unreached and deliberately present.

Two things not to break:

- **`.cxxflags-stamp` and `.cxxflags-stamp-haswell`.** `CXXFLAGS` is not a
  prerequisite of anything, so without these `make OPT=-O3` over an `-O2` tree
  recompiles nothing and yields a mixed binary. Two stamps, not one, because one file
  can only describe one variant.
- **`070-clean.mk` names what it removes.** A new build artefact that is not a
  `$(SRC)/*/*.o` needs adding there. Never `rm -rf` a directory.

## 6. Tests

**There is no test infrastructure in this tree.** Not a target, not a directory, not a
harness. `make` builds three binaries and that is all.

**The first satellite had a good one and it should be ported**, not reinvented.
`old_versions/first_satellite/make_support/120-tests.mk` plus `TESTNAMES` in its
`070-directories.mk`. The shape:

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

## 7. The X-macro registry — the mechanism M2 ports

`old_versions/first_satellite/src/bytecode_format/` is 1133 lines across five files
and it is the thing being ported and finished. Read it before writing
`src/satellite_words/`.

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

**And one thing the header must say out loud**, or it promises more than it keeps: the
asserts cover the **frozen** half only. User capsules and spacesuits are numbered at
parse time (WORD_NUMBERS §3) and no compiler can see them.

---

## 9. What is not decided anywhere, and blocks writing M2

Everything above is documented somewhere. The following is not — in any of the six
permanent documents or in the existing code — and each would otherwise be settled by
whoever types first.

**1. How is `words.def` written as a tree?** PLAN §8 says *"each entry names its
parent, and its position among that parent's children **is** its number."* The first
satellite's rows carry an explicit `id` column; here the number is **implicit in
order**, which is a different macro and a different set of asserts. Nobody has written
the signature. Something like `SAT_NODE(parent_ident, ident, text)` — but the parent
must be declared before the child, and whether that is enforced or merely assumed is
part of the same decision.

**2. Call shapes sit at two depths, and the `.def` can only encode one.**
WORD_NUMBERS §4 is explicit that this is unsettled: `include()` is a *child* of
`include` at `1 1 0`, while `input()` is a *sibling* of `display` at `1 5 2`. §4
proposes a reading under which both are correct and asks for it to be confirmed
**before `words.def` encodes it**. This is a hard blocker on writing the file.

**3. What is a `PathId`, exactly?** DESIGN §4.5 says one `uint32_t` for a whole path,
and that it is *"the interned id of the terminal node"* — so it is an index into a node
table, not the numbers packed into bit-fields. What is not said: the node table's C++
type, the maximum node count, and what happens on overflow. Four billion is the
headline figure and the real ceiling is whatever the table is.

**4. `(0)` forms.** A node that answers bare *and* has children — `satellite.container`,
`arguments.memory`, `arguments.machine` — is marked `(0)` in WORD_NUMBERS §2.2. How
that is expressed in the `.def` is undecided.

**5. Aliases.** Six spellings of `arguments`, `hexadecimal` for `hex`, `.range` for
its `(min,max)` sibling. One node, many spellings — the inverse of the interner's
many-nodes-one-string. The spelling table has to hold both directions (DESIGN §4.4)
and nothing says how the `.def` declares an alias.

**6. The digest.** SATC.md §2 requires one over `words.def` so a `.satc` can name the
numbering it was written against, and SATC.md §6 openly asks *what it is over* — if
the trie is built from more than that one file, the digest has to cover all of it or
two numberings can share a hash. Algorithm, input set and storage are all unchosen.

**7. How does a live child counter coexist with a `constexpr` frozen table?** PLAN
§8.1 requires both: the language's children frozen and asserted, the user's appended at
parse time. A `constexpr` array cannot grow, so the runtime trie is a separate mutable
structure seeded from the frozen one — but that is an inference, not a written
decision, and §8.1 explicitly asks whether M2 owns a real allocator or a stub.

**8. Error handling before M5 exists.** M5 builds the error reporter and M2 comes
first, so a failed trie walk at M2 has nowhere to report to. DESIGN §9.1 rules out
throwing (measured: 8.5 ns as an enum against 1537 ns thrown) and rules out
*"record the error and return nullptr"*. What M2 returns instead is unwritten.

**9. The C++ names.** `satellite::words::Node`? `Trie`? `Interner`? `PathId` is the one
name any document uses. Everything else is unnamed, and a name chosen at the keyboard
is a name every later file inherits.

**10. Whether M2's interning is threaded at all.** PLAN §4.5.1 says the crossover — how
many satellite-rooted source lines before 24 threads beat 1 — *"is not yet measured"*,
and §9's rule is measure on this machine rather than quote. Threading the fixed table
at startup is already argued to be a guaranteed loss; threading the user's source is
open, and the measurement blocks the decision.

**Also unscheduled rather than undecided:** [SCRATCH.md/MILESTONE.md](../SCRATCH.md/MILESTONE.md)
lists everything with no milestone at all, including the test infrastructure §6 says
does not exist.

---

## 10. The checklist, before a file is committed

- [ ] under 300 lines
- [ ] `#pragma once`, includes spelled from `src/`, grouped and ordered per §2
- [ ] `namespace satellite`, closed with its `// namespace satellite` comment
- [ ] a file-top comment saying what it is and **why it is this way**
- [ ] every number in it carries where it came from and when
- [ ] anything that would break if wrong is a `static_assert`, not a comment
- [ ] added to `040-sources.mk` — `SATL_SRCS` and `HDRS`, spelled out
- [ ] `make` builds all three binaries clean under `-Wall -Wextra`
- [ ] the new thing has a consumer **in the same milestone it is written** — the
      first satellite shipped three commits where the registry had none, which is
      how four defects accumulated behind a guarantee nothing checked

*Companions: [DESIGN.md](../DESIGN.md) — what the language is.
[PLAN.md](../PLAN.md) — §3 the line rule, §4 the build, §8 the milestones.
[WORD_NUMBERS.md](../WORD_NUMBERS.md) — the authority over every number.
[SATC.md](../SATC.md) — the cached form, and the digest §9 says is unchosen.
[LAYOUT.md](../LAYOUT.md) — every file in the tree.*
