# M7 — what reading in found, before any of it was built

**Written 2026-08-31, at the start of M7 and before a line of resolve exists.**
Nothing here is a decision. It is what an afternoon of reading the tree turned up
about the milestone that is next, kept so that the reading does not have to happen
twice. **Delete it when M7 lands** — `MILESTONES/M7.md` is where any of this that
turns out to be true belongs.

The permanent documents are [DESIGN.md](../DESIGN.md) §7 (what resolve *is*),
[PLAN.md](../PLAN.md) §8's M7 paragraph and §2.2 (how it gets built),
[LAYOUT.md](../LAYOUT.md) and [WORD_NUMBERS.md](../WORD_NUMBERS.md) §1.5 and §3.

---

## 0. Where the tree stands

`make` is clean, `make test` passes **six** suites under clang, and `make startup`
exists as of today. **The working tree is uncommitted** — eight modified files and
three new ones, all of it the `make startup` harness and the two carried items
below. That is the state `M6_STATE.md` was written to catch and it applies again.

## 1. M7 has no `Done when:` in PLAN §8, and seven other milestones do not either

[PLAN.md](../PLAN.md) §8's M7 entry is the milestone line plus the three-things-it-
owns pass of 2026-08-28, and it stops there. Every landed milestone was reviewed
clause by clause against its done-when — `MILESTONES/M6.md` §2 is the table — and
**M7 has nothing to write that table against.**

The eight without one: **M7, M9, M10, M11, M16, M17, M23, M24.** M1–M5 lost theirs
to being rewritten in the past tense when they landed, which is a different thing.

**A draft, offered and not accepted, kept here so it is not composed twice:**

> **Done when** `satl --resolve <file>` prints every capsule with its frame —
> parameter slots first, then locals, each with its name and declared type; a name
> that resolves to nothing is refused with an S05xx code, a span and a caret; a
> redeclaration in one scope takes a **fresh** slot and the dump shows both;
> `sort("down")` folds to `1 4 2 5` at resolve and the `.satc` still reads
> `"down"`; a parameter named any of §7.7's six spellings becomes the special
> variable and a seventh (`argv`) is **refused** rather than silently plain; and
> resolve **skips a path the `.satc` already numbered**, measured — a warm hit
> beats `--unparse` on hello_world.
>
> Spacesuits are M26: pass 2 exists in the order and resolves nothing, and says so.

## 2. There is a draft, and it is `prototype/M6/`

**Old numbering — `M6 -> M7`.** PLAN §8's opening carries the whole table and
`SESSION.md` §0 is the short version. So the resolve draft is under the directory
named `M6`, which is frozen because a draft is dated by construction
(`MILESTONES/README.md` says why).

    resolver.cpp 147   resolver_names.cpp 275   resolver_scopes.cpp 188
    resolver_suits.cpp 248   resolver_walk.cpp 258   resolver.hpp 116
    resolve_types.hpp  — SLOT_* sentinels, ResolveNodeInfo, ResolveTable

1,526 lines, and it has the shape: the four passes, the side table, the option
fold, the six spellings, a `DepthGuard`.

**It is a design source and not code to copy**, and LAYOUT.md's "drafts, and never
sources" is the rule. The reason is concrete: it is written against a **typed-node**
AST — `CapsuleDecl`, `NameExpr`, `AstArena`, a `Type` struct, `Access`, `Span` on
the node — and this tree's [ast.hpp](../src/abstract_syntax_tree/ast.hpp) is a
24-byte POD in an arena with a `static_assert` pinning it. Nothing transfers
literally; the decisions do.

**And it over-reaches M7's scope.** `resolver_suits.cpp` builds inheritance links,
cycle breaking, flattened field layout, method tables and access checking —
**spacesuits are M26.** DESIGN §7.3's second pass has to be a *named hole* in M7:
present in the order, resolving nothing, saying so.

## 3. The parser already owns half of what M7's line reads like

`Parser::define_name()` — [parser_declarations.cpp:279](../src/parser/parser_declarations.cpp)
— already interns capsule, spacesuit and global names into `words::Words` and
already reports the reservation-rule collision as **S0241
`PARSE_NAME_IS_LANGUAGE_OWNED`**, with S0242 and S0243 beside it.

So **the number lands at parse time and the slot is M7's**, which is what PLAN's
M7 bullet actually says (*"this is where a capsule name becomes a slot"*) — no
correction needed there.

**One stale comment, which is M2-era and which M4 answered.**
[words_runtime.hpp](../src/satellite_words/words_runtime.hpp)'s `find()` says
*"DESIGN §2's reservation rule is decided at M7's resolve."* M4 decided it, in the
function above. Worth fixing when M7 touches that file.

## 4. The `.satc` skip is a done-when clause with a number behind it

`MILESTONES/M4.5.md` §5, written in three places on purpose: **"M7's resolve has to
learn to skip a path the `.satc` has already numbered"** before the cache pays for
itself. Until then a warm hit is *slower* than `--unparse`, because it reads a
larger file, runs the substitution back, and then lexes and parses anyway.

This is the one clause of M7 that can be *measured* rather than asserted, and
`make startup` now exists to measure it.

## 5. M7 needs a consumer, because every milestone since M2 has had one

PLAN M2's rule — the registry gains a consumer **in the milestone that writes it**,
because the first satellite shipped three commits where its registry had none and
four defects accumulated in that window. `satl --words`, `--unparse`, `--satc`,
`--check`, `--limits`. `satl --resolve <file>` is the obvious shape and nothing has
named it yet.

**And `main.cpp` is 427 lines**, 127 over what PLAN §3 asks a file to be built
toward, with `M6.md` §6.1 naming the seam and declining to take it: the arms that
DUMP a registry and the arms that read a FILE are two subjects. M7 adds a
fourteenth arm to that switch. The seam does not get better by waiting.

## 6. §7.5's recursion bound is M9's, and M7 has a different one

DESIGN §7.5 sits inside §7, which is M7's section, so it reads as M7's. It is not:
[facts.hpp](../src/system_facts/facts.hpp) says in its own words **"M9's ceiling
comes from here and DESIGN §7.5 is why"**, and it is right — the depth guard fires
while a *program* recurses, which is the evaluator's.

**What is M7's is a different stack**: the resolver walks the tree recursively, so
a deeply nested expression blows the *resolver's* C++ stack before anything runs.
The draft guards it with `MAX_RESOLVE_DEPTH = 2000` and a `DepthGuard`. Two bounds,
two milestones, one section — worth separating before it is built.

## 7. Reserved and empty, waiting

`errors.def` reserves **S05xx** for resolve and has populated none of it. The block
comment names DESIGN §7 and M7. `tests/reporter_test/parsing.cpp` is the shape to
copy: **one assertion per row of `errors.def` the pass owns, through the real entry
point**, because a site raising a neighbouring code renders perfectly and no assert
can see it (`SESSION.md` §5.20).

---

## What was done on 2026-08-31 instead of starting M7

Both items the previous session carried, and both are now in permanent documents —
listed here only so nobody looks for them twice.

- **`make startup` is built.** `MILESTONES/M6.md` §6.9 is closed. It found that
  this tree's "both sides static" instruction is a special case of *both sides
  linked the same way*: satl's own **share** is stable across link modes (0.176
  dynamic, 0.187 static, against M6's 0.168) while the absolute rows are 0.9 ms
  apart. PLAN §4.3, `make_support/040-sources.mk` and `startup.rows` carry it.
- **The watchdog's 130 is carried into M22.** Correct behaviour, verified — SIGINT
  130, SIGTERM 143, 4 when the ceiling fires. The gaps were documentary: M22's
  entry did not mention what M6.md §6.5 said it inherits, and still said
  `_exit(2)` where M6 §3.4 changed it to 4.
- **Two things found in passing**: LAYOUT.md had no row for `048-static.mk`, and
  FORMAT/CXX.md §5 was still teaching a load-bearing-order constraint that
  `065-tests.mk` recorded being disproved on 2026-08-28.
