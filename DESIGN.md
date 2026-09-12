# satellite — the language

**This file is permanent.** It says what satellite *is*: the naming rule everything
follows from, the syntax, the numbering, the scope model, the types, and what the
language deliberately refuses. [PLAN.md](PLAN.md) is the other half and says how it
gets built — architecture, milestones, the build and the install. When the two
overlap, this file describes the language and PLAN.md describes the work.

**Status, 2026-08-26.** The second satellite is at milestone 1: there is a binary
that says what it is and how a file will be run, and no interpreter behind it yet.
So most of this document describes a language that is **specified and not yet
running here**. Everything marked *(v1: verified)* was built and measured in the
first satellite, which is at `old_versions/first_satellite/` and still works; it is
the source this is pulled from, not a promise about this tree. Everything marked
*(new)* is a second-satellite decision with no first-satellite equivalent.

---

## How to cite this document

**By section number, never by file and never by line.** `§4.2` is the whole
address. The first satellite learned this the hard way: thirty-five notes citing
`DESIGN.md:3051` and the like pointed at nothing the moment the file was split, and
the prose they quoted survived only because it was findable by its words.

The companion rule, inherited verbatim from the first satellite's `format.def` and
still in force:

> **PROSE MAY EXPLAIN A NUMBER. IT MAY NEVER BE THE ONLY PLACE THE NUMBER LIVES.**

Every number in this document is either measured and attributed, or it is a
pointer to the file that actually holds it. The test for whether a fact belongs in
compilable data rather than here is whether anything breaks if it is wrong.

---

## 1. The generating rule

One invariant produces all satellite naming:

> **A dotted path rooted at `satellite` names something the language owns.
> A bare identifier names something the user owns.**

Every other naming rule follows from it:

- Types are language-owned, so they are prefixed: `satellite.variable.time`.
- Variable names are user-chosen, so they are bare: `my_time`.
- Method names are selectors interpreted relative to what precedes them, so they
  are bare: `my_time.some_function()`.
- `satellite.main` is prefixed because the *runtime* chooses and calls that name.
  A capsule the user writes is bare: `fact(3)`.

The looser phrasing — "everything the language provides has `satellite` in front of
it" — is not the real rule, because `some_function` and `now` are language-provided
and bare. The invariant above is the version that holds everywhere.

`satellite` is the **only reserved word in the language.** Nothing else needs
reserving: `variable`, `library`, `time`, `file`, `list` are special only *inside* a
satellite-rooted path, and never on their own — `time` is `satellite`'s child in
`satellite.time.now()` and `variable`'s child in `satellite.variable.time`, and in
neither case does that stop a user naming a variable `time`. Declaring anything
named `satellite` is a hard error, checked at the binding site.

### 1.1 What the rule is for

The path *is* the interface, and that is the point. The thing it exists to abolish:

```cpp
std::something<std::something> my_file_object("filename", std::ios::in, std::ios::app);
```

— template noise, a bitmask of flags nobody remembers the order of, and punctuation
ceremony. Against:

```satellite
satellite.file.open("filename", "read_append")
```

which reads without a reference open. So: **no flag soup, no bitmask ORs, no
options that are numbers.** If a call needs an option, the option is a *word* at the
call site. The path segments are the documentation.

This is the tie-breaker for every fork in the language: **do absolutely everything
for the user**, and pay for it in performance rather than in their attention. Speed
is the secondary goal and never wins an argument against clarity.

But *doing everything for the user* is not *doing things behind their back*. Do all
the plumbing — framing, byte order, buffering, partial reads, sharing, cycles — and
none of the policy. Never silently truncate, silently convert a type, or silently
reconnect. **A refusal in plain words beats a guess.**

---

## 2. The reservation rule

> **`satellite.variable.*` and `satellite.container.*` are type namespaces.
> A path in either namespace is never a value expression.**

This rule is not optional. Without it the design has a genuine ambiguity *(v1:
verified with a prototype lexer)*: the parameter

```
satellite.container.list<satellite.variable.string> arguments
```

produces a token stream **identical** to the chained comparison
`((satellite.container.list < satellite.variable.string) > arguments)`. That is
exactly the C++98 `a<b>c` problem, and no amount of lookahead resolves it, because
the distinguishing fact — "is this path a type or a value?" — is semantic, not
syntactic.

With the rule, type context and expression context are disjoint by construction:
`parse_type` is entered only where a type is grammatically required, so `<` inside
it is always a generic opener; `parse_expression` never calls `parse_type`, so `<`
there is always less-than.

**Three tokens settle *this* question** — `satellite . variable` against
`satellite . library` separates a type path from a value path, and no fourth token
is needed *(v1: `Parser::at_language_path` tests indices 0–2)*. The grammar as a
whole is **LL(5)**, and the extra two are spent in one place: every statement form
in §6 shares the prefix `satellite . statement .`, so choosing between `if`, `while`
and `for` needs a fifth token.

Corollary: **user-defined generics must not use bare names**, since a bare name can
be a value. They stay deferred (§12).

### 2.1 The consequence worth naming

Because a type is always syntactically distinguishable by its `satellite.` prefix,
satellite **cannot have C++'s most vexing parse.** The declaration-versus-expression
ambiguity that has plagued C and C++ for forty years arises precisely because a type
name and a variable name are both bare identifiers. Here they never are.

---

## 3. Hello world

```satellite
// this is a comment

satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")

    // this is a comment inside of main

    satellite.return(satellite)
}
```

**That is `example/hello_world.satl`, byte for byte, comments included.** PLAN M17
names the file rather than a paragraph as its done-when, so the two must not be
able to drift; the way to guarantee that is for this section to be a copy rather
than a description. The `//` comments are part of the program and are specified in
§5.6.

**The parameter is back.** *(Restored 2026-08-28, reversing the 2026-08-27
removal.)* This section spent a day showing `satellite.main()` with no parameter,
on the argument that declaring
`satellite.container.list<satellite.variable.string> arguments` made hello world
**unreachable at the milestone that owns it** — M17 could not construct a `list` of
`string` before M16 built the container and M11 the string. The removal was made in
good faith and it was wrong on the point it rested on.

**Its load-bearing citation misread the numbering.** The removal argued that
WORD_NUMBERS.md §2.2 writes `satellite.main` as **`1 3 (0)`** and that §1.3's `(0)`
means *zero arguments, nothing there*. WORD_NUMBERS §1.3 settled what the marker
means the day after, and it means something else: `(0)` is **a reading aid marking
a node that is reached both bare and as a parent**, and a row written `1 3 (0)`
says the node is `1 3` and its zero-argument call shape is `1 3 0`. That is a fact
about how `satellite.main` can be *reached*. It says nothing about how the capsule
is *declared*, and it never did. WORD_NUMBERS is the authority (its own opening
rule), so the reading that contradicted it was the bug.

**Hello world was also the only one of the four acceptance programs without the
parameter.** `advanced.satl`, `thread_test.satl` and `super_advanced.satl` all
declare it, and SATC.md §1.1's specimen listing compiles this program's signature
as `1.2 1.3(1.4.2<1.6.1> arguments)` — the parameterised form, which that section
never stopped showing. One program out of four disagreed with the other three and
with the cache format's own worked example.

**And WORD_NUMBERS.md §1.1 is the current program again, not a historical walk.**
That section is the worked example of §4.3's order-of-first-appearance rule, and
the parameter is what fixes two of the first six numbers: `container` `1 4` and
`variable` `1 6` are met **only** inside
`satellite.container.list<satellite.variable.string>`. With the bare form, `console`
walked onto `1 4` and neither `container` nor `variable` was met at all, so the
document that *is* the authority on the numbering had to be annotated as no longer
matching the program that produced it. Nothing renumbers either way — §4.3 froze
the order and the freeze is the point — but a frozen numbering whose worked example
still runs is worth more than one whose example has to be explained away.

**What it costs, stated plainly rather than assumed away.** The program never
reads `arguments`, so what it owes is an empty list bound to the slot and nothing
else: no `satellite.variable.string` value is ever constructed, so M11 is not a
dependency, and none of M16's twenty-five list methods is reached. What is left is
one empty `satellite.container.list` value, and **that list is M16's.** *(Decided
2026-08-28.)* An empty list is still a list, and the milestone that constructs one
has built the type — so **hello world is PLAN M17 and runs after M16**, while the
console it prints through stays at **M10**, before M11. PLAN §8's opening carries
that split. The milestone is still the place that has to say what it hands over, or
a program written between M17 and the milestone that builds §7.7 gets an error for
`arguments.username` that no document predicts.

Both shapes remain legal and §6's grammar already allows both: `capsule_decl` reads
`"(" [ param_list ] ")"`, and the brackets are not new. A program that wants what
the machine knows declares the parameter and gets §7.7's special variable; a
program that does not, does not pay for it. Hello world declares it because the
first program in a language should show the shape every other program will use.

- **capsule** = function.
- **`satellite` as a value** is the singleton runtime object, not a zero sentinel.
  That is why `satellite.include(satellite)` and `satellite.return(satellite)` use
  the same word to mean sensible things: include the runtime, return the runtime
  (that is, success).
- **`satellite.include(satellite)`** is the one include form that does nothing, and
  that is not a leftover — it means "include the runtime", which a running program
  already has. Every other form names a **spaceship** and loads it.
- **`satellite.return()`** is not required. Only `satellite.main` *must* have
  `satellite.return(satellite)`.
- **`arguments`** is one of §7.7's six accepted spellings, and §7.7 is where the
  special variable it becomes is specified.

**This program is the target of PLAN.md §8's M17**, which is where it runs end to
end — **and since 2026-09-06 it does.** *(It said "milestone 8" until 2026-09-02,
which is the number it had before the 2026-08-30 renumber — §8's opening carries
the whole old-to-new table. The split is the point: the console it prints through
landed at **M10** on 2026-09-02 and the program still did not run, because its
parameter is an empty `satellite.container.list` and that is **M16's**; until M16
landed, `satl example/hello_world.satl` answered with a caret under the word
`arguments` and said which milestone. M16 bound the empty list on 2026-09-05 and
M17 is where this section became a claim something checks.)*

**And the byte-for-byte claim at the top of this section is now enforced rather
than intended.** *(2026-09-06, M17.)* `tests/parser_test/roundtrip.cpp` reads
this document, cuts the fenced block out from under this heading, and asserts it
IS `example/hello_world.satl` — so an edit to either side that the other does
not get fails `make test`. The sentence "the way to guarantee that is for this
section to be a copy rather than a description" was true about the intent and
could not be true about the outcome: **a copy is an intention until something
compares them.** DESIGN.md is a prerequisite of that test in
`make_support/065-tests.mk`, the same arrangement `WORD_NUMBERS.md` has with
`words_test`. **The heading is load-bearing** — the test finds the block by
`## 3. Hello world` — and it says so rather than failing silently if the
heading is retitled.

---

## 4. The numbering *(new)*

This is the second satellite's central design and the one thing it does that the
first did not. The first satellite built a word-and-path registry with frozen ids,
asserted over it at length — and then compared strings in the hot path. §4 is that
job finished.

**This numbering is satellite's bytecode.** The language never emits one — PLAN §2
rules a bytecode VM out of the architecture and closure compilation replaces it —
but the job a bytecode does, *give every operation a small integer so that
dispatching on it is an array index rather than a comparison*, is the job §4 does.
The difference is where the integer comes from: a bytecode assigns it at compile
time to a linear instruction stream, and this assigns it to the **namespace**, once
and permanently, so nothing has to be compiled for the number to exist.

That is also why the number carries the **call shape** and not only the path
(§4.5). An opcode encodes an operation together with its operands;
`satellite.console.input()`, `input(prompt)` and `input(prompt, target)` are three
different things a program can ask for, so they are three numbers, exactly as they
would be three opcodes.

The first satellite very nearly had this and did not use it: `format.def` held 107
words and 29 paths with frozen ids and static_asserts over them, and the evaluator
still joined the path into a heap string and ran a chain of string compares.
**The table was the bytecode and nothing read it.**

**[WORD_NUMBERS.md](WORD_NUMBERS.md) is the authority and this section only
explains it.** Every actual number lives there; the examples below are quoted from
it, and if they ever disagree with it, they are the ones that are wrong. This is
the project's own rule applied to itself — prose may explain a number, but it may
never be the only place the number lives.

### 4.1 Every node numbers its own children, starting at 1

A path is the sequence of those numbers, read left to right.

```
satellite . console . display
    1     .    5    .    1
```

`satellite` is 1 because it is the root. `console` is 5 because it is the fifth
child of `satellite`. `display` is 1 because it is the first child of `console`.

```
satellite.console.display   1 5 1
satellite.console.input     1 5 2      input is console's second child
satellite.variable          1 6        variable is satellite's sixth child
satellite.random            1 7        random is satellite's seventh child
satellite.random.fast       1 7 1
satellite.random.normal     1 7 2
satellite.random.ultra      1 7 3
```

Note what `satellite.console.input` and `satellite.random.normal` have in common:
both end in 2, and **the 2's are unrelated.** One means "console's second child",
the other "random's second child". That is the scheme working, not a collision.

*(2026-09-04. The three tier rows above are written bare and §2.2 writes them
`fast()`, `normal()` and `ultra()` — the same numbers, and the parens are the
truth: `1 7 1`–`1 7 3` name the zero-argument call shape, which §11 refuses by
design. A bare tier spelling folds to the same number, and a path is not a
value.)*

**So there are a lot of 1's and a lot of 2's, and that is correct.** A number on its
own means nothing; a number means something *at a position, under a parent*.

#### The order is the order words first appear

Which word gets which number is decided by one rule, and it is not a ranking:
**walk a real program from the top and change the number only when you must.**
Hello world (§3) fixes the first six — `include`, `capsule`, `main`, `container`,
`console`, `variable` — and everything after is appended as it is first needed.

Nothing about `include` being 1 claims it matters more than `variable`. The
alternative was to group namespaces by what they do and number the groups, and it
was rejected for being an argument about taste that no two people settle the same
way. Reading order is a fact about the program; importance is an opinion about the
language.

**The last two of those six do not fall out of a strict walk, and WORD_NUMBERS.md
§1.1 now carries the correction in full.** *(Found and fixed 2026-08-28.)*
`satellite.variable.string` sits inside the parameter's type, so a reader meets
`variable` on §3's third line, before the fifth reaches `console` — read strictly,
that program gives `variable` 5 and `console` 6, and the table has them the other way
round. **The numbers do not move**; §4.3 froze them and the freeze is the whole point.
WORD_NUMBERS §2.1 says what actually happened — **1 to 15 were written by hand**, and
16 to 24 were assigned by this rule applied strictly — and §3's own last line proves
it, since `satellite.return` is met there and is **15**, not 7.

So the walk is why the order is this one and not another, and it is the rule that
decides every number from 16 on. **It is not a procedure that regenerates 1 through
15**, and saying so is cheaper than letting a reader find the gap and conclude the
numbering is arbitrary. It is frozen, which is a different thing.

### 4.2 The lists are per-parent, not per-level

At the second word the distinction does not show, because every second word is a
child of `satellite`. At the third word it does: `display` and `input` are children
of `console`; `string` and `number` are children of `variable`. Those are **two
separate lists**, each numbered from 1.

Per-parent is the right one, for three reasons:

- **Validation is free.** `1 6 1` means "take satellite's child 6, then take *that
  node's* child 1." If the node has no child 1, the path does not exist, and you
  learned that from the array bound. A flat per-level list would let `1 6 7` name a
  legal level-3 word that is not legal under `variable`, so legality would need a
  second check.
- **It has no depth limit**, which is the requirement. A flat level-6 list mixing
  every unrelated namespace at that depth is not a list anyone can reason about.
- **Adding a word touches one node.** Registering a new child of `console` cannot
  renumber anything under `variable`.

### 4.3 Registration order is the numbering, so it is frozen

Because a child index *is* its position in its parent's list, **the order words are
registered in is the numbering**:

> Never renumber. Never reuse. Always append.

`console` is child 5 of `satellite` because it was registered fifth, and it stays
child 5 forever. A new child of `satellite` goes on the end. Removing one leaves a
hole rather than shifting its neighbours down.

This is stricter than the first satellite's flat list, and it has to be: there, a
word id was global, so moving a word between namespaces did not change its number.
Here the number *is* the position, so the position is the frozen thing.

The numbering is written down in **[WORD_NUMBERS.md](WORD_NUMBERS.md)**, which is
the authority, and transcribed into `src/satellite_words/words.def`, which is the
copy a compiler can check. `words.def` is the one file exempt from PLAN.md §3's
line ceiling, because splitting a numbering whose meaning is registration order is
the one split that could silently change what a program means.

**This freeze is a promise about the language's own words, not about the user's.**
A capsule or a spacesuit the user writes also gets a number — the next one free
under the node that owns it, allocated when the name is first met rather than
frozen in advance (WORD_NUMBERS.md §3). It cannot be frozen across programs,
because it is not the same name in two of them. So every node keeps a live count
of its children: the language's are numbered first and never move, and the user's
are appended after them and last exactly as long as the program does. A user
name's number is therefore **not stable between runs**, and anything that writes
one down must record the name instead.

### 4.4 Spelling is a separate table

A node stores its own spelling. Printing `1 6 1` back as
`satellite.variable.string` is a walk from the root, one node per segment, and the
whole path is always available wherever a number is — so nothing needs a global
word-id-to-text table to produce a name.

What is kept is a **string interner** for the spellings, so that `list` under
`container` and `list` under `directory` are two nodes sharing one piece of text.
That is deduplication, not identity: two nodes, one string, two different child
indices, and rightly so.

**§1's reserved word is one interner comparison** — "is this bare word spelled
`satellite`" is answered at the binding site without a walk. Note this is §1's rule,
not §2's: §2 reserves whole *namespaces* inside a path, which is a question about
position, not about a bare spelling.

**And an interner id is what a token carries (§5.6).** *(2026-08-30.)* This is the
section that decides it, so it is worth saying here rather than only there: because
the table is deduplication, an id answers *"which of the language's spellings is
this bare word"* and nothing more. It does **not** name a node — two nodes can share
it — so it is not a §4.5 `PathId` and must never be stored as one. Reading that
sentence the other way cost a working lexer once; §5.6 has the account.

### 4.5 PathId — one `uint32_t` for a whole path

§4.1 is how a path is *structured*. This is what a path *is* at runtime.

`satellite.console.display` is **one thing**. It should not travel as `1 5 1` in
three registers, and it should not travel as three `long long`s — three 64-bit ints
is 24 bytes where a `uint32_t` is 4 and covers four billion paths. The trie walk
happens once, at parse time; the terminal node it lands on has an interned id; that
single integer is what the compiled closure holds. Dispatch becomes:

```cpp
handlers[path_id](frame, args)
```

One array index. One indirect call. No allocation, no string, no ordered arms.

**A `PathId` is produced by the walk and by nothing else** — which is why the lexer
cannot hand one out (§4.4, §5.6): it has one bare word and a path is what is being
identified. *(Said explicitly 2026-08-30, because the type does not say it:
`SpellingId` and `PathId` are the same `uint32_t`, so the compiler accepts either
wherever the other is meant.)*

So the numbers are the *source of truth and the way a path is written down*, and
the PathId is the *handle*. The first satellite's `SAT_PATH(ident, s1, s2, s3, s4,
arity)` reached for exactly this and stopped at four segments, which is a word limit
this language does not want.

### 4.6 What the numbering buys beyond speed

- **Better errors**, which is an explicit goal (§9). A failed walk knows *which
  segment* failed and *which node* it failed under, so `satellite.consle.display`
  answers "no `consle` under `satellite` — did you mean `console`?" by running edit
  distance over that one node's children. The first satellite's answer was
  `no such module function: satellite.consle.display`.
- **`satellite.help` becomes a walk of the trie**, so help cannot drift from what
  exists. **The trie is what is *numbered*, though, and that is not the same as what
  is built** *(corrected 2026-08-28)* — after M2 it holds every path in `words.def`,
  including the ones no milestone has reached, so a walk of it alone would advertise
  unwritten paths as working and break §1.1's *never behind their back*.
  **And `handlers[path_id]` alone is not the answer either, which is the second
  correction and it arrived with the build** *(corrected 2026-09-08, M18)* — of the
  six paths hello world is written in, **exactly one, `satellite.console.display`
  `1 5 1`, is a row in that table**. `include`, `capsule`, `main`, `return` and
  every type name are recognised by the parser and the resolver and are never
  dispatched, so a help that printed only handler rows would name one word of the
  language's own first program and stay silent about the other five. **The trie is
  what *exists*, the handler table is what *runs*, and help answers for what is
  *built*** — a node with a handler row, an **assigner** row (§4.5.3's dials), or a
  **front-end word**, plus anything with one of those underneath it. The front-end
  set is **written down as data** in `words.def`'s fourth list rather than inferred,
  because a set derived from which spellings a parser compares against moves the
  first time somebody refactors a comparison and the only symptom is a help text
  that has started lying. `src/satellite_help/built.hpp` is the predicate and
  MILESTONES/M18.md is the record; PLAN M18 carries the argument in full.
- **The reservation rule becomes one interner lookup.**

---

## 5. Lexical structure

The lexer walks `SatString`, so the code table in `satellite_string.hpp` *is* the
language's alphabet. Five rules are load-bearing, and each fixed a verified defect
in the first satellite's original plan — they are inherited as rules, not
rediscovered.

### 5.1 Underscore is an identifier character

*(v1: verified blocker.)* `_` is punctuation in the code table. Under the naive
rule `my_time` lexes as **three tokens**, and every example in this document
breaks. The slicing example `list_name[some_number_start:some_number_end]` degrades
from 6 tokens to 16. Underscore is a word-start and word-continue character, named
by a constant beside the table rather than by a bare number.

### 5.2 Whitespace lives in the raw area

*(v1: verified.)* Space, tab and newline have no code-table entry, so they arrive
as `raw base + byte`. A lexer that calls `isspace()` on a `SatChar`, or compares
against `' '`, skips nothing and emits one Error token per space.

### 5.3 Lex raw; expand escapes only inside a string body

*(v1: verified.)* An `encode()` that expands backslash escapes everywhere corrupts
source text before the lexer sees it. Lex with a raw encoder; apply the
escape-expanding one only to the *body* of a string literal.

### 5.4 String literals need real escapes

*(v1: verified.)* `\n \t \r \\ \"` all round-trip unchanged, by two different
routes: `\n`, `\t` and `\r` have no code-table entry and land in the raw area
(§5.2), while `\\` and `\"` are real code-table punctuation. Match escape names
longest-first, so a short name cannot shadow a longer one that starts with it.

And the table's silence about every *other* backslash turned out to matter: an
escape the table does not know passes through with the backslash still attached,
which is not an error and therefore invisible — `"DOESN\'T"` printed the backslash
501 times in one real program's output. Unknown escapes are a decision to make on
purpose (§13), not to leave to silence.

### 5.5 There is no `<<` and no `>>`, ever

satellite has no `<<` or `>>` operator, ever, and that is written policy rather
than an accident of the operator list. This is why nested generics need no special
handling: `list<list<string>>` lexes as two independent `>` tokens and
`parse_type`'s recursion consumes one per level. **The C++98 maximal-munch bug
cannot occur.** Bit shifts are `satellite.variable.number.shift_left(n)` `1 6 4 1`
and `.shift_right(n)` `1 6 4 11`, consistent with §1 — under
`satellite.variable.number`, where every other number operation lives, and not
under a top-level `satellite.number`, which does not exist. *(Corrected
2026-08-27; this was the only place in either document that implied one.)*

#### What a shift means, which this section did not say until 2026-08-31

**`a.shift_left(n)` is `a × 2ⁿ` and `a.shift_right(n)` is `a ÷ 2ⁿ`. Both are
exact, neither ever rounds, and a fractional receiver is not a special case.**

That sentence is the whole rule, and the paragraph above is why it had to be
written: this section introduced the spelling as an aside about *where a path
hangs* — "bit shifts, if ever needed" — and an aside is not a specification.
M8 went to build the two paths, found the first satellite has neither and no
document defining either, and the milestone stopped rather than guessing.
`MILESTONES/M8.md` §4.1 is that finding.

**It terminates in both directions, and that is the fact the decision rests on.**
2 divides 10, so `2⁻ⁿ` has an exact decimal expansion of exactly *n* places —
`1 ÷ 2¹⁰` is `0.0009765625` and not a rounded approximation of it. So
`shift_right` belongs in §8.6's first class with `shift_left`: **exact and
bounded, never rounds**, and neither reads `division_digits` nor waits on the
rounding rule §8.6 leaves open. The implementation does not divide at all —
`a ÷ 2ⁿ` is `a × 5ⁿ ÷ 10ⁿ`, and dividing by a power of ten is an adjustment to
the exponent.

**The fractional receiver needed no rule because a multiplication does not care.**
`7.5.shift_left(1)` is exactly `15` and `7.5.shift_right(1)` is exactly `3.75`.
The two alternatives were `×10ⁿ`, which is one line and makes the name a lie
about what a shift is, and refusing a receiver with a fraction, which invents a
refusal the arithmetic does not need.

**On an integer this is the C meaning and nothing is surprising** — `1 << 10` is
1024 either way — which is the property that makes the name honest. What it does
NOT inherit from C is C's undefined behaviour: there is no width to shift out of,
so no shift count is too large and no bit is ever lost. §7.5 is why that has to
be true, and it is the same sentence as everywhere else — the bound is memory.

The greedy two-character operators are exactly `== <= >= !=`, and those are the only
places `<` or `>` is not a single-character token. `>=` is the one that could in
principle collide with a generic close, so `parse_type` guards it *(v1: verified)* —
though in the grammar of §6 no collision is actually reachable, because a complete
type is only ever followed by IDENT, `)`, `,` or `>`.

### 5.6 The rest

- **`//` runs to the end of the line and never reaches the parser.** *(Specified
  2026-08-28; every program in `example/` used it and no section defined it.)* It is
  discarded in the lexer, not stored as a token, so it costs the grammar in §6
  nothing. There is no block comment — `/*` is two ordinary punctuation tokens, and
  a form that can be left unclosed is a form that can swallow a file. §3's hello
  world is the worked example, and SATC.md §1.1 uses the same two characters for the
  comment column of a `.satc`, which is read by people and **never trusted** on
  load.
- **Never fold a sign into a Number.** `-1` is always `Punct(-) Number(1)`. Folding
  would turn `a-1` into `Word(a) Number(-1)` and break subtraction. Unary minus is
  an expression rule.
- A `.` joins a Number only when the token started with a digit *and* a digit
  follows, so `3.14` is one Number but `main.x` is `Word . Word`.
- Tokens carry `start`/`end` offsets **and a `line`**. All three are load-bearing —
  offsets for error spans, `line` for the same-line postfix rule in §6.2.
- **The lexer never throws.** It emits an Error token carrying a position.
- **A known word carries its SPELLING out of the lexer — §4.4's interner id, not a
  `PathId`**; a user-owned bare word carries its text *(new — this is §4 reaching
  back into the lexer)*. The two are different questions and only the first one
  is answerable here: a lexer sees `display` and cannot know whether it belongs
  to `satellite.console.display` or to a variable, because the answer is a
  property of the path being built around it, and §6's parser is what builds one.
  §4.5's walk is what turns a run of spellings into a `PathId`.

  *(Corrected 2026-08-30. This bullet read "known words carry their node
  identity out of the lexer", and **that sentence cost a working lexer.** Read
  against §4.5 it says `PathId`, and a first implementation stored `intern()`'s
  answer in a field of that type — which **compiles**, because the two are the
  same `uint32_t`, and hands the parser a number meaning "the first node in the
  registry spelled this" to dispatch on. §4.4 is what settles it and said so all
  along: the interner is "deduplication, not identity", because `list` under
  `container` and `list` under `directory` are two nodes sharing one string. The
  word that has to appear in this bullet is therefore **spelling**, and the test
  that catches the confusion needs a word the language spells twice.)*

---

## 6. Grammar

```ebnf
program        := { top_level }
top_level      := include_decl | capsule_decl | spacesuit_decl | global_decl
global_decl    := "satellite" "." "library" "." IDENT [ "=" expression ]

include_decl   := "satellite" "." "include" "(" [ expression ] ")"

capsule_decl   := "satellite" "." "capsule" capsule_name
                  "(" [ param_list ] ")" [ returns_clause ] block
capsule_name   := IDENT                        // user capsule, bare
                | "satellite" "." IDENT        // reserved; currently only `main`
returns_clause := "satellite" "." "returns" "(" type ")"
param_list     := param { "," param }
param          := type IDENT                   // prefixed type, bare name

type           := "satellite" "." type_space "." IDENT [ "<" type { "," type } ">" ]
                | "satellite"                  // the singleton runtime type
                | IDENT                        // a spacesuit, named bare (§13)
type_space     := "variable" | "container"

spacesuit_decl := "satellite" "." "spacesuit" IDENT [ "(" [ IDENT ] ")" ] [ ":" ] suit_block
suit_block     := "{" { suit_section | member } "}"
suit_section   := "satellite" "." ( "protected" | "public" ) suit_block

block          := "{" { statement } "}"
statement      := var_decl | assign | expr_stmt | return_stmt | block
                | if_stmt | while_stmt | for_stmt
assign         := postfix "=" expression
expr_stmt      := expression
var_decl       := type IDENT [ "=" expression ]
return_stmt    := "satellite" "." "return" "(" [ expression ] ")"

stmt_kw        := "satellite" "." "statement" "."
if_stmt        := stmt_kw "if" "(" expression ")" block
                  [ stmt_kw "else" ( block | if_stmt ) ]
while_stmt     := stmt_kw "while" "(" expression ")" block
for_stmt       := stmt_kw "for" "(" [ var_decl | assign ] ";" [ expression ] ";"
                  [ assign ] ")" block

expression     := precedence climbing over the operators in §6.6
postfix        := primary { "." IDENT | "(" [ args ] ")" | "[" subscript "]" }
args           := expression { "," expression }
subscript      := expression | [ expression ] ":" [ expression ]
primary        := NUMBER | STRING | "satellite" | IDENT | "(" expression ")"
```

Statements are newline-terminated, which is why §6.2's same-line rule for a postfix
opener is load-bearing rather than a nicety.

***`suit_section` cited `block` until 2026-08-30, and as written the rule could not
parse the program it was written for.*** A `block` is `"{" { statement } "}"`, and a
capsule declaration is not a statement — while every section in
`example/class_test.satl` holds capsule declarations and nothing else. **A section
holds what a suit block holds**, so it cites `suit_block`, and one function reads
both. Found by PLAN M4 when the parser was built against this table; the same
correction M3 made to §5.6, and made the same way — the sentence stays visible
rather than being quietly replaced, because a specification that gets silently
corrected stops being a record of what it said.

*(A section inside a section is expressible under this rule and means nothing.
That is left rather than forbidden: a grammar that refuses it needs a second
production for no gain, and `satellite.public` inside `satellite.protected` is a
question for M26's resolve, where access actually decides something.)*

***`include_decl` read `"(" expression ")"` until 2026-09-06, and the numbering
had said otherwise since the numbering was written.*** WORD_NUMBERS.md §2.2 gives
`satellite.include()` the number **`1 1 0`**, and §1.3 uses that exact form to
teach what a trailing zero is — *"`0` means nothing in that position. It is a real
number in the sequence and not a piece of notation, which is why `include()` and
`include(satellite)` are two different sequences rather than one path called two
ways"* — under a rule that settles which document wins: **"a trailing `0` is
written only where a program can actually write the bare form."** A program could
not. `satellite.include()` was answered with **S0231, "expected an expression"**,
which is this grammar refusing a call shape the numbering had assigned.

**WORD_NUMBERS.md is the authority over the numbering by its own opening rule, so
the grammar is what moved** — the same resolution §3 reached on 2026-08-28 when
the parameter came back, and reached the same way. The brackets are now optional,
`include()` means *include nothing* and does exactly that, and PLAN M17 owns the
row. **Every other layer was already built for it**: `words.def` carried the row,
`shape_of()` matched it at arity 0, the resolver already read a missing argument
as zero arguments, the `.satc` writer already printed a zero-arity row without
brackets, and `satellite_cache/unnumber.cpp` names *this form* as the reason a `0`
segment may not be skipped when a cache is read back. **One production was the
whole of the disagreement**, and the sentence stays visible for `suit_section`'s
reason above.

### 6.1 Statement dispatch is on segment 1, not on shape

*(v1: verified.)* The signal that a type path ended and a name began is *two
adjacent Word tokens with nothing between them*. Whitespace is load-bearing without
being a token: `satellite.variable.timemy_time` glues into a single Word, while
spaces around the dots are free.

But a purely **structural** rule — "dotted path followed by a bare Word is a
declaration" — is not safe, and the collision comes from §1's own unifying rule:

```
satellite.control.return my_time    ->  Word . Word . Word Word
satellite.variable.time  my_time    ->  Word . Word . Word Word
```

Identical in shape. A structural rule silently declares a variable named `my_time`
of type `satellite.control.return`. Every statement form shaped
`satellite.<x>.<y> <operand>` collides this way, and no amount of lookahead fixes
it.

**Dispatch on segment 1 instead** — which under §4 is no longer a string compare
but a child index:

| `path[1]` | meaning |
|---|---|
| `variable`, `container` | type path — a following Word is a declaration |
| `library` | variable path |
| `statement` | statement form: `if`, `else`, `while`, `for` |
| `include`, `capsule`, `spacesuit`, `return`, `returns`, `protected`, `public` | declaration and statement forms with their own parse rules |
| anything else | module; needs `(` — but not absolutely, or module constants like `satellite.bool.true` become errors |

The last two rows are one rule, not two: a segment-1 word either has a parse rule
of its own or it does not, and the ones that do are exactly the forms above.

This also makes "no keyword enum" *honest* rather than aspirational: the keywords
become a parser dispatch table rather than an enum in the lexer. The lexer stays a
pure text-to-tokens function with no feedback edge from a symbol table — no C-style
lexer hack, and none of the reclassification machinery Java needs an entire spec
chapter for.

### 6.2 The postfix loop

*(v1: verified defect in the original sketch.)* `parse_primary` then
`while (peek == '.')` cannot parse `foo().bar()` — it fails on the first token after
the primary. The loop must cover all three postfix forms over one node variable,
each *replacing* the node with a wrapper:

```cpp
for (;;) {
    if (at_punct(".")) { node = member(node, ident());    continue; }
    if (at_punct("(")) { node = call(node, args());       continue; }
    if (at_punct("[")) { node = index(node, subscript()); continue; }
    return node;
}
```

This single loop is what makes chaining uniform. `Member` and `Call` are peers, so
`satellite.time.now().some_function()` and `my_list[0].f()[1:2]` both fall out with
no extra rules.

**The same-line rule.** A postfix `(` or `[` binds only when it opens on the *same
line* as the thing it applies to. Statements are newline-terminated (§6), so without
this the loop would reach across a line break and swallow the next statement: a line
ending in `x` followed by a line opening `(f(y))` would parse as `x(f(y))` rather
than as two statements. It covers both openers, not just `[`.

*(This paragraph ended "this is what `line` on every token is for (§5.6)" until
2026-08-30, when M4 built the loop and found that is not what enforces it. §5.6
leaves the **newline in the stream as a token**, so an opener on the next line is
separated from its receiver by a token the loop never crosses — the terminator
being a token is the mechanism, and `line` is the proof. Corrected rather than
deleted because the field is still load-bearing: §9's reporter is built on it, and
a rule that holds for a different reason than a specification claims is a rule
somebody will re-derive.)*

**A newline inside a bracket is not a terminator.** `f(` on one line and its
arguments on the next is one call, because nothing in this section can be reached
while a bracket is open. That is not a weakening of the rule above: the postfix
loop runs at bracket depth zero, which is the only place the rule is asked.
*(Decided at M4, 2026-08-30; §6 stated statements were newline-terminated and said
nothing about an argument list that spans lines.)*

The rule that distinguishes a call from a path is **one token**: after a dotted
path, if the next token is `(`, it is a call — the last segment is the method name,
everything before it is the receiver.

### 6.3 Keep the parser resolution-free

Do *not* teach the parser that `satellite.library.<fn>.<var>` is exactly four
segments — that encodes the standard library's shape into the grammar and forces a
new parse rule per namespace. Emit a flat Member/Call/Index chain and resolve it
afterwards, letting each object answer for its own members.

*(new)* §4 changes what "afterwards" means, not this rule. The trie walk that turns
a path into a PathId happens **once, after parsing and before running**, in the same
pass as resolve (§7). The parser still emits a flat chain.

### 6.4 Methods are sugar

```
my_list.append(x)   ===   satellite.container.list.append(my_list, x)
```

The right-hand side is **not** surface syntax — no program may write it. It names
the dispatch-table entry: the (type node, method name) key that receiver methods and
module functions share, with the receiver written out as the first argument.

*(v1: verified.)* A prototype over **one** table landed 16000/16000 concurrent
appends from 8 threads with no lost write. Receiver methods and module functions
share one table, so methods cost almost nothing once module functions exist.

Three qualifications:

1. The module path is **not derivable** from the type name — `satellite.container.list`
   has two segments, `satellite.time` has one. Use an explicit table, not string
   derivation. *(new: under §4 this is a `PathId` and not a string at all — a
   real node identity, which §5.6's spellings are not; the walk has happened by
   the time anything reaches this table.)*
2. Constructors live in the same table and must carry a "does the first parameter
   bind the receiver" tag, or `my_file.new()` degrades into a confusing arity error.
3. `nil` has no module, and there are **two** non-dispatchable states needing
   different messages: an undeclared variable, and a declared variable holding
   nothing.

A mutating method needs a receiver that names a storage slot. `foo().append(x)` is
rejected — there is nowhere to write back.

### 6.5 No satellite code runs inside a mutation

> **No satellite code runs inside the lock that a mutating method holds.**

*(v1: verified blocker otherwise.)* `std::mutex` is not recursive, so re-entering
the update on the same variable **deadlocks** — and receiver syntax makes this
reachable from ordinary user code: `my_list.append(my_list.size())`. Fully reduce
arguments to values *before* taking the lock.

**This invariant is what decides which types may be map keys** (§8.4). A map's hash
and equality run inside that lock, so they must be native and can never be satellite
code. That is not a preference about extensibility; it is the deadlock above, one
level down.

### 6.6 The operators, and how tightly each binds

*(Decided at M4, 2026-08-30. §6's expression rule read `... precedence climbing ...`
and stopped, so this table did not exist and the parser could not be written
without one.)*

| | binds | |
|---|---|---|
| 4 | tightest | `*` `/` `%` |
| 3 | | `+` `-` |
| 2 | | `<` `>` `<=` `>=` |
| 1 | loosest | `==` `!=` |

**All four levels are left-associative**, so `a - b - c` is `(a - b) - c` and
`a - (b - c)` is a different number that keeps its brackets.

**A FIFTH LEVEL IS DECIDED AND UNBUILT: `!!` JOINS TWO BIT RUNS AND BINDS
LOOSER THAN `+`.** *(The author, 2026-09-09. §8.5 carries the whole set of
decisions this belongs to; it is here as well because a table that a milestone
will change is one every reader of the parser consults first.)* When it lands
the table becomes

| | binds | |
|---|---|---|
| 5 | tightest | `*` `/` `%` |
| 4 | | `+` `-` |
| 3 | | `!!` |
| 2 | | `<` `>` `<=` `>=` |
| 1 | loosest | `==` `!=` |

so `xFF + x01 !! x02` is `(xFF + x01) !! x02` — each part worked out, then laid
end to end — and `a !! b == c !! d` is `(a !! b) == (c !! d)`, which is what
puts it above the comparisons rather than below them. **It is left-associative
like every other row**, and that is worth saying because the author's first
instruction was "right to left every time": scoped to all operators it would
have made `10 - 3 - 2` answer 9, so it was scoped to `!!`, where associativity
turns out not to change the answer at all — concatenation being associative —
only the level does.

**`+` JOINS TWO STRINGS AS WELL AS ADDING TWO NUMBERS.** *(The author,
2026-09-08, at M19.)* It is one operator over two types and not a second meaning
for the character: addition and joining are the same shape — take two of a
thing, answer one of that thing, change neither of them. **Nothing is
converted**, which is what keeps this on the right side of §1.1: `"n = " + 4` is
still refused (S0711), and a program that wants it writes `4.to_string()` out
loud. The only pair that joins is two strings, and every other mixed pair is the
refusal it already was.

*Until this, `append(x)` `1 6 1 14` was the only way to put two strings
together, and it MUTATES its receiver under §6.4's storage-slot rule — so a
string built out of pieces needed a variable to be built in, and
`display("could not open " + f.path())` was unwritable. M19's file diagnostics
are all that shape, and two example programs in the tree had been written with
`+` on strings and could not run. `append` is unchanged and is still how a
string a program is holding is added to in place.*

**Unary `-` and unary `!` bind tighter than all of them.** The minus is not a
choice: §5.6 refuses to fold a sign into a Number so that `a-1` stays a
subtraction, which makes unary minus an expression rule by construction.

**These are C's levels with every operator satellite does not have removed**, and
the reason is §1.1's tie-breaker rather than deference: this is the only table a
reader of this language already knows, and surprise is a cost paid by the user.
`<<` and `>>` are absent permanently (§5.5). **There is no bitwise and no logical
row**, which is §13's open question and not an omission — until it is answered,
`&` has no precedence, so it ends an expression and is reported rather than
guessed at.

**Assignment is not on this table because it is not an operator.** §6 makes
`assign` a *statement*, so `a = b = c` is not an expression and `if (a = b)` is
not writable — which is the C defect this language does not have to warn about.

*The eleven rows live in `src/abstract_syntax_tree/ast.cpp`, which is what this
document's opening rule requires of any number in it: the table above explains
them and is not the only place they are written. They are in the tree rather than
in the parser because they have **two** readers — the parser climbs them, and the
printer reads them to decide whether a bracket a program wrote has to come back,
since no node records that a parenthesis was there.*

---

## 7. Scope — frames and slots

**The most important decision in the language**, and the first satellite's best
engineering. It is ported, not reconsidered.

### 7.1 What it fixes

*(v1: verified blocker.)* With one global registry keyed by
`<capsule>.<variable>`, a capsule has **exactly one slot per local for the whole
program**, and no per-call storage.

- **Recursion collapses.** A recursive `fact` returns **1 for every input** — 1, 2,
  3, 5, 10 all yield 1, because the base case's write is the last one standing. The
  failure is evaluation-order-dependent: caching in a temporary before the recursive
  call makes the same source return 6 correctly, which is the worst possible
  property for a bug.
- **Concurrency is worse, and would not be seen coming.** Eight threads running a
  capsule with *no recursion and no shared state* produced **1585 wrong results out
  of 1600.** A write lock does not help: the registry provides *atomicity* (no torn
  value) but not *isolation* (private storage), and locals need isolation.

This cannot be reframed as deliberate "capsule-static" semantics, because
parameters are locals too — `arguments` would be a program-wide static.

### 7.2 The fix

`satellite.library` is reserved for **shared and global state**. Every capsule call
gets its own frame; locals resolve to **integer slot indices statically, before
execution**, in a separate resolve pass.

```cpp
struct Frame {
    std::vector<Value> slots;   // no mutex, no atomic: reachable from one thread
};
```

Three things make this the clear choice:

1. The global registry needs **zero logic changes**. It is not demoted; it is
   pointed at the data it was designed for. Permanent identity, cross-thread
   sharing, lock-free reads and atomic read-modify-write are exactly what globals
   need and exactly the opposite of what locals need.
2. **Frames are faster.** 200k iterations of `fact(15)`: **160 ms with frame slots
   against 1131 ms through the registry — 7.1×.** Correctness here is not a tax.
3. It does not violate "no bytecode VM". The tree stays the tree; a
   variable-reference node holds an integer instead of a string.

### 7.3 Resolve runs in four passes

Not in the parser, and the reason is not purity: **a capsule may call one defined
further down the file, and mutual recursion is unresolvable in single-pass recursive
descent.** So resolve runs every capsule name first, then every spacesuit name and
the links between them, then top-level statements, then each body — the order the
first satellite's `Resolver::run()` numbers in its own comments. Two properties fall
out: the parser's purity survives, and a resolver bug reports
`unknown variable in capsule: x` *before* anything executes rather than producing a
wrong value somewhere inside the walk.

*(new)* The resolved data lives in a **side table indexed by arena node id**, not in
a `mutable` field on the node. See PLAN.md §2.2 — this turns a documented data race
into a structural impossibility.

**PASS 3 IS "top-level statements" AND THIS GRAMMAR HAS NONE.** *(Found at M7,
2026-08-31.)* §6's `top_level` is include, capsule, spacesuit and global, and the
parser's S0204 says so in the words a user reads: *"a statement at the top of a
file is not an unfinished feature, it is a program with no capsule to run."* So
what that pass resolves is the two forms that can carry an expression outside a
body — a global's initialiser and an include's argument. **It keeps its place,
because the ORDER is what this section is about**: a global read by a capsule
body has to be numbered before pass 4.

### 7.4 A slot is never reused across scopes

A redeclaration in one scope **rebinds the name to a fresh slot** rather than being
an error. The fresh slot is not an implementation detail: a spacesuit is a reference
type, so reusing the old slot would leave every handle already taken to the first
instance pointing at the second — and a list built by that idiom would read back as
*n* copies of its last element **with no error anywhere.**

### 7.5 Recursion is not bounded, and a walker keeps its own stack

**REWRITTEN 2026-08-31 BY THE AUTHOR'S DECISION, AND THE OLD CONCLUSION IS
WITHDRAWN.** This section said *"recursion is bounded, and the bound is
derived"*, and the rule is now the opposite: **the language has no depth limit,
and no walker in it may use the C++ stack for depth the user's program
controls.** §7.5.1 was written the same day to say that the tree did not meet it.

**THE TREE MEETS IT AS OF 2026-09-01.** M8.5 rewrote the four static passes and
M9 built the evaluator onto the same shape, so the rule holds end to end: a
program 1,000,000 capsule frames deep answers at the 8 MiB a login shell hands
out, and a runaway one is refused **in words about recursion**, with a caret, a
call stack and exit 4. §7.5.1 is kept as the record of the gap and of what
measuring it cost.

**A WALK OVER USER-CONTROLLED DEPTH KEEPS ITS OWN STACK FROM ITS FIRST COMMIT,
AND THAT IS THE FORM THIS RULE TAKES FOR EVERYTHING STILL TO BE BUILT.** M9 found
it by building one: the closure COMPILER is a walk too, it did not exist when
M8.5 rewrote the others, and there is no exception here for a pass that runs
once. A program that parses at 100,000 deep and then dies being compiled would
have moved the crash rather than removed it. The same applies to M16's search
walk and to every reader of a tree after it.

The measurements below are kept **as evidence and not as a specification.** They
are the reason a fixed guess is the wrong shape, and they were paid for once.

*(v1: measured, and the first guess was wrong.)* A default depth of ~10000 sat past
both stack cliffs, so the guard could never fire and the segfault it existed to
prevent was exactly what a runaway recursion got. One activation costs 3 units and
~3169 bytes at -O2, so a ceiling derived from `RLIMIT_STACK` is 3000 on an ordinary
8 MB stack and ~24,000 on 64 MB.

**That derivation is better than a constant and it is still a limit.** It makes
`ulimit -s` decide how deep a program may go, and this section used to call that
*"outside the language on purpose"* — which is true about where the number comes
from and false about whose problem it is. A program that runs on one machine and
dies on another, for a reason neither the language nor the user chose, is §1.1's
*never do anything behind their back* with the shell's configuration standing in
for a decision.

**The answer is that a walker keeps its own stack on the heap**, so depth is
bounded by memory the way a list's length is — and running out of memory is an
event this language already has words and an exit status for (§9, and PLAN §4.5.2's
watchdog). PLAN §2.5 has called that the *explicit control stack* and deferred it
since the plan was written; the author un-deferred it on 2026-08-31.

**Two things fall out that were listed as reasons to WANT it rather than
consequences of having it.** PLAN §2.5's own first paragraph: execution becomes
**pausable and resumable**, which is what green threads, generators, a stepping
debugger, Ctrl-C at an arbitrary point (§10.2) and driving the interpreter from a
GTK idle callback with no second thread (§10.3) all need. Under the old rule those
were a future CEK machine's to buy. Under this one they arrive with the fix.

**A CEILING THE USER SETS IS NOT A LIMIT THE LANGUAGE HAS**, and the distinction
matters here because one exists. `satellite.library.system.max_depth` `1 14 2 2`
is a numbered path that predates this section's rewrite and could not be deleted
— WORD_NUMBERS §1.2 is *never renumber, never reuse* — so on 2026-09-01 the
author gave it the one reading that keeps its name honest: **a memory ceiling on
the control stack, in bytes, and unset means the machine.** It bounds nothing by
default, it is a number the user chose rather than one satl invented, and what it
buys is a sentence — a control stack that has its own ceiling knows it is a
control stack, so a runaway recursion can be told about recursion instead of
about memory. This is the same distinction §8.1's `division_digits` draws: a
bound on what a configuration file may ask for is not a bound on the language.
PLAN §8's M9 entry is where it gets built, **and M9 built it on 2026-09-01**:
`limits::max_depth_bytes()` is the reader, the check happens when the control
stack GROWS rather than on every push, and unset reads `MEMORY_MAX` — which is
the whole machine by default and is the smaller, correct number where somebody
has said how much memory satl may take. **It has no range**, because any number
of bytes is a number of bytes; `MILESTONES/M9.md` §4.5 is why the one PLAN
expected does not exist.

The numbers live beside the code that uses them, never only here.

#### 7.5.1 What the tree does today — CLOSED AT M8.5 AND M9, and kept as the record of the gap

**EVERY WALKER IN THE TREE NOW KEEPS ITS OWN STACK, AS OF 2026-09-01.** M8.5
rewrote the parser, the resolver, the printer and the `.satc` writer onto stacks
on the heap — seven walkers over four cycles — and 100,000 nested brackets now
answer from all four commands at the 8 MiB a login shell hands out, where 19,000
used to end `--unparse` with signal 11. `MILESTONES/M8.5.md` is the review and
`PLAN.md` §8's M8.5 entry is the milestone. **§7.5's rule is met by the static
passes and is not yet met by an evaluator, because there is not one**; M9 builds
it onto the same shape, which is what `PLAN.md` §2.6 puts this milestone ahead of
it for.

**AND M9 BUILT IT, ON 2026-09-01.** The evaluator compiles onto an explicit
control stack — four heap vectors carrying what a recursive one would have put in
C++ frames — so `eval(node)` never calls `eval(child)`. Measured at the same
8 MiB: a capsule 1,000,000 frames deep answers, where the same closure tree walked
with C++ recursion segfaults between 15,000 and 20,000, which is the band the
printers were dying in above. **What the rule costs is measured rather than
quoted**: 1.3× on a program shaped like a program and 3.5× on the tightest
arithmetic loop that can be written, against a recursive evaluator over the same
arena. `MILESTONES/M9.md` §6 has the method, and §1.1's tie-breaker is why it is
spent — pay in performance rather than in the user's attention.

*The rest of this section is the measurement that made the case, kept because a
specification that only states its rule cannot show why the rule is worth what it
costs. Everything below was true on 2026-08-31 and the depths are now history.*

*(Measured 2026-08-31, `ulimit -s 8192`. `SCRATCH.md/NO_LIMITS.md` is the full
table and the plan.)*

**Four walkers recurse on the C++ stack with no bound at all**, so each has a
depth at which it dies with signal 11 and says nothing. At the 8 MiB a login
shell hands out, one expression nested N deep killed `satl --unparse` at 19,000,
`satl --satc` at 20,000 and the parser at 32,000.

**satl now raises its own stack to a share of the machine at startup and those
depths all work** — 500,000 nested brackets pass. `machine_limits/limits.hpp` is
the policy — **32 KiB of stack for every MiB of memory, never under 128 MiB**,
which is 1.9 GiB on this 61.9 GiB machine and 32 GiB on a terabyte one — and
`satl --limits` prints both the number and where it came from. The 8 MiB was a
shell's soft DEFAULT with an unlimited hard limit behind it, not a kernel wall,
so a process may raise its own without root; measured 2026-08-31, it costs one
syscall and no memory, because a stack is lazily committed.

**That is a bigger number and not the absence of one, and the rule above is still
unmet.** A share of the machine is still a number the machine chose: 1.9 GiB
here, and every machine has one.

**BUT THE WATCHDOG ALREADY REFUSES IT IN WORDS, WHICH THIS SECTION GOT WRONG ON
ITS FIRST DAY.** It said exhausting the C++ stack is *"a segfault with no code,
no span and no sentence, because that stack is not something satl allocates and
therefore not something it can count."* **Touched stack pages are resident
memory**, so `process_memory_bytes()` counts them like any other page, and PLAN
§4.5.2's watchdog was already watching. Measured 2026-08-31: a 2,000,000-deep
parse reaches 1.36 GiB resident, and under `MEMORY_MAX=1GiB` satl stops with

    satl: stopping -- this run is using 1.0 GiB and MEMORY_MAX is 1.0 GiB (the file)

and exit status 4. One line, a ceiling the user set, and a status a script reads.

**So what is actually left is narrower than a whole rewrite, and it is two
things.** First, the DEFAULT case: with no config, `MEMORY_MAX` is the whole
machine — 61.9 GiB here — so 1.9 GiB of stack is exhausted long before the watchdog
has anything to say, and that run still segfaults. Second, the SENTENCE: the
watchdog names *memory*, which is true and is not what happened. A program that
recursed away is better told about recursion.

**And both have a cheap answer that is not a heap stack.** `system_facts/` already
reports `thread_stack_bytes(used, total)` — M6 built it and `satl --limits` prints
it. A watchdog that watched that ratio as well as the memory one would refuse a
runaway recursion in its own words, on the default configuration, using machinery
that already exists. §7.5's rule still asks for the heap stack; this is most of
what the rule was FOR, at a fraction of the work, and it belongs to whoever
touches the watchdog next.

**A crash is not a limit.** A limit refuses in words with a code, a span and a
caret (§9); this leaves no exit status a script can read and no sentence a person
can act on. It is the failure this section's v1 note already describes — *"the
segfault it existed to prevent was exactly what a runaway recursion got"* —
arriving in the passes rather than in the evaluator.

**And one real limit existed, added at M7 and withdrawn the next day.**
`name_resolver/resolve.hpp` bounded the resolver at a **fixed** 2000 written
levels and refused with **S0501**. It was the only place in the tree that said
anything at all at depth, and it was the wrong shape by this section's own
argument — a constant, where even the rule it replaces asks for a derivation.
**Both are gone**: the constant and its error row were deleted on 2026-08-31 and
the resolver's walk stopped recursing at M8.5, which is the other half of the
same sentence. `errors.def` has 61 rows where it had 63.

**`satellite_cache/paths.cpp`'s `flatten()` is the one walker that cannot be made
to crash**, and it is the model: it reads a postfix chain with a `for` loop up the
tree into a flat vector. Its own comment gives a different reason for that shape —
that the alternative is *"the same walk written twice"* — which is how a thing
done right for one reason turns out to be right for another.

### 7.6 Capsules are not in the registry

Capsule definitions live in their own table. A bare capsule name cannot even form a
legal registry key *(v1: verified)* — the key normaliser accepts a name only if it
holds exactly one dot with a non-empty segment on each side, which is
`function_name.variable_name` and never a bare word. Capsules are written once at parse time and never reassigned; they need none
of the registry's machinery.

### 7.7 `arguments` — the one library global the language provides

`satellite.main` takes a list, and that list is not only a list. It is the language
handing the program everything it knows about the machine it woke up on, and it
lives at **`satellite.library.main.arguments`** — a library global (§7.2), which is
exactly what §7.2 reserves `satellite.library` for.

```satellite
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments.username)

    satellite.return(satellite)
}
```

**Displaying it bare prints all of it.** `satellite.console.display(arguments)` is
how a program shows the user everything that went into the special variable, which
is the §1.1 tie-breaker applied to introspection: the information is already there,
so the language hands over all of it rather than making someone ask for it one
field at a time.

#### The name is any of seven spellings

`arg`, `args`, `argz`, `argument`, `arguments`, `argumentz` and `argv` all become
the special variable inside the program that declares one. The author writes
`arguments`; the others exist because people type what they type, and a language
whose tie-breaker is *do absolutely everything for the user* does not make somebody
lose an afternoon to a plural.

This is the **inverse of §4.4's interner** and needs saying plainly, because §4.4
describes the opposite arrangement. There, two nodes share one spelling — `list`
under `container` and `list` under `directory` are different nodes that happen to
be spelled alike. Here, **one node answers to seven spellings.** Deduplication is
many-nodes-one-string; aliasing is one-node-many-strings, and the table has to hold
both directions.

It is also the one place a **bare** identifier is language-owned, which §1's
generating rule otherwise forbids. The rule survives because the user still writes
the name: the language does not introduce `arguments`, it *recognises* the name the
user chose for `satellite.main`'s parameter when that name is one of the seven.

#### What it holds

Nested, not flat, and **the shape below is the author's, settled 2026-09-09** —
thirty-six facts in eight sections, each one a numbered word the registry can
print and `satellite.help` can answer for:

```
arguments.machine       cores  cpu  threads  architecture
                        byte_order  page_size  pointer_bits
arguments.memory        total
arguments.username      the login name
arguments.system        name  kernel  kernel_version  distribution
                        distribution_id  distribution_version  hostname
arguments.build         compiler  compiler_version  standard  flags
                        make  standard_library  c_library  built
arguments.interpreter   bare = the path to satl; version
                        library_path  library_path_source
arguments.process       id  parent
arguments.session       shell  terminal  language  home  directory
arguments.length        how many words were on the command line
```

**IT WAS `arguments.count` UNTIL THE OBJECT GAINED ITS OWN METHODS**, on the
day M20 built them, and the author split the two words rather than let one
shadow the other. `satellite.container.arguments.count()` `1 4 3 2` is how many
ENTRIES the object holds — the command line and every fact about the machine,
37 against 3 on this machine — and both are reached off `satellite.main`'s
parameter, so one spelling would have answered the other's question with
nothing saying so. **`length` is how long the command line is and `count` is
how many entries there are, everywhere**, which is §1's *a word means one thing*
applied to a collision rather than to a preference.

**AND THE OBJECT ANSWERS TEN QUESTIONS ABOUT ITSELF**, settled by the author on
2026-09-11 and numbered under `satellite.container.arguments` `1 4 3` — a TYPE,
so §1.5's fold through the receiver is what reaches them. `length()`, `first()`,
`last()` and `contains(x)` are the command line; `count()`, `keys()`, `has(k)`
and `get(k)` are every entry, by the dotted name the printer uses; `to_string()`
and `lines()` are the whole of it as text. It is M16's thirty-four container
methods one level down, over an object that is two containers at once.

**A GROUP WORD WRITTEN ON ITS OWN DISPLAYS ITS CHILDREN** — the author,
2026-09-11, at M20, asked with the alternatives laid out. `arguments.machine`
answers all seven of its facts as a map, `arguments.session` all five of its,
and so on. That is the §1.1 tie-breaker one level down from *"displaying it bare
prints all of it"*: a program can print a whole section without naming each word
in it.

**This section used to say otherwise and the two lines are corrected here.** It
wrote `arguments.memory` as *free memory* and `arguments.machine` as *what the
processor is*, which was v1's flat shape read through a nested one. Free memory
is `satellite.system.memory.free()`, which M20 built, and the processor is
`arguments.machine.cpu`, which this section already spelled as *"the same, asked
for directly"*.

**`interpreter` is the one exception and it is deliberate.** Written bare it is
the path to the running `satl` binary rather than a map of its children:
`machine`, `system`, `build`, `process` and `session` are groupings, and an
interpreter is a thing, whose natural value is where it is.

A node that is both a value and a parent is therefore still the general case
here rather than a special one, and it is the same shape `satellite.container`
already has — a bare form and a set of children, which is what the `(0)` in
WORD_NUMBERS.md marks.

**`cores` and `threads` are satellite numbers and so are five others** — page
size, pointer bits, total memory, and the two process ids. The rest are strings.
v1 could not do this: its arguments object *was* a `list<string>` and every
entry had to be one. This one is its own type — `satellite.container.arguments`
`1 4 3` — so a count is a count.

#### Three surfaces, one set of facts

`arguments.machine.threads` and `arguments.memory.total` are **the same numbers**
as `THREAD_COUNT` and `MEMORY_MAX` in the configuration, and the same numbers
again as codes 97 and 98 in `satellite_string`'s live code table (PLAN §6).
`arguments.machine.cores` has **two** surfaces and not three: it is the
configuration's `CORE_COUNT` and the arguments object, and there is no live code
for it. Three ways to ask where there are three, one place that knows —
`system_facts`.

**THAT SENTENCE COUNTED WRONG UNTIL 2026-09-09 AND THE ARITHMETIC IS WHY IT WAS
RESTATED RATHER THAN MADE TRUE.** It read *"the same numbers again as codes 97,
98 and 99"* over three names, which positionally makes 98 `cores`; the live code
table has 97 threads, 98 mem_total_mb, 99 mem_used_mb, and PLAN §6.1 agrees.
Minting a code 100 for `cores` would have added a fourth row to a table whose
rows are a permanent wire format, to satisfy a sentence's arithmetic. The
sentence was restated instead, which costs nothing and is true.

**AND `arguments.memory.total` AGREES WITH CODE 98 AT WHOLE MEGABYTES, WHICH IS
THE RESOLUTION BOTH OF THEM HAVE.** `satellite.system.memory.total("mb")`
answers the exact figure, fraction and all — 63430.21484375 on this machine —
and these two answer 63430. They are not three renderings of one number; they
are two surfaces on the coarse fact and one on the exact one, and a program that
asserts all three are equal has to say which of the two it is asking for. M20's
demonstration does exactly that.

They must not be allowed to disagree. Whatever the configuration finally says, it
is a *setting* and the machine is a *fact*, and a program that asks
`arguments.machine.threads` is asking what the machine has, not what the config was
told. If the two ever need to differ, they need two different names.

**And the configuration writes these three names, which it could not until
2026-08-31.** `CORE_COUNT=arguments.machine.cores` is the machine's own answer, in
the spelling above and in any of the seven; `CORE_COUNT=12` is twelve. It is the bare
form and not the rooted path, for the reason this section already gives — a program
writes the name it gave `satellite.main`'s parameter, never
`satellite.library.main.arguments`, and a configuration that demanded the rooted
spelling would be asking for one the language does not use. PLAN §4.5.4 has what
*not* having it cost: satl read all three facts on every run, because the file had
no way to say "the machine" and something had to fill the values in before it was
opened.

#### Open

- ~~**Which spellings, exactly, and what happens to the seventh?**~~ **ANSWERED
  AT M7, 2026-08-31 — AND THE ANSWER WAS REVERSED BY THE AUTHOR ON 2026-09-09.**
  The spellings come out of `words.def`'s alias rows rather than a list in any
  source file, and **there are seven of them: `argv` is one.**

  **What M7 decided, and why it was right at the time.** The seventh was
  *refused rather than silently plain*, but only where somebody plainly meant
  the object: §4.6's suggester, run over `satellite.library.main`'s children and
  aliases, had to come back with one of the six before the refusal was raised.
  `argv` was one edit from `arg` and got **S0531** telling it so; `input_lines`
  is nowhere near any of them and is an ordinary list, which is what this
  section already says the language does — *it does not introduce `arguments`,
  it recognises the name the user chose.*

  **What changed is not the rule but which side of it `argv` sits on.** The old
  text called this *"a rule and not a seventh spelling"* and named the one door
  a reversal could come through: *"adding `argv` to `words.def` would make it
  work and is an edit to the numbering, which WORD_NUMBERS.md is the authority
  over and which is the author's to make rather than a milestone's to take in
  passing."* On 2026-09-09 the author walked through that door, having written a
  program with `argv` in it and been refused by their own interpreter — which is
  this section's *"people type what they type"* with the language's author as
  the witness. `argv` is the spelling every C program on earth uses; a language
  whose tie-breaker is *do absolutely everything for the user* had no business
  making it the one near miss that fails.

  **S0531 keeps its number and its job**, and deleting it would have been the
  mistake this reversal invites. It still fires for `argu`, `arrgs`, `argvs` and
  every other near miss. One of its worked examples became a spelling; the code
  did not become wrong. And because the suggester reads the alias rows, the list
  it offers grew with no edit to the resolver at all — which is the same
  property that let this change be one line of `words.def`.

  **And the object is `satellite.main`'s parameter and no other capsule's.**
  *(Decided at M7.)* A capsule of the user's own with a parameter called `args`
  holds whatever its **caller** passed; giving it the machine's answers instead
  would be §1.1's *behind their back* with the wrong value in the variable. The
  M6 draft in `prototype/` recognises the spellings everywhere, which is
  where that was found.
- **What is `arguments[0]`?** It is a list as well as an object, and nothing yet
  says whether index 0 is the program name, the current directory, or the first
  thing the user typed. (§13.)
- **The first satellite's version was flat and had 33 entries** —
  `thread_count`, `architecture`, `distribution`, `cxx_compiler`, `process_id`,
  `page_size`, `byte_order` and the rest, in `system_facts/arguments_facts.cpp`.
  The nesting above is a redesign, so every one of those 33 needs placing under a
  parent or dropping, and that is a decision per entry rather than a port.

---

## 8. Types and the value model

| satellite type | representation | notes |
|---|---|---|
| `satellite.variable.bool` | `bool` | written `satellite.bool.true` / `.false` |
| `satellite.variable.number` | exact arbitrary-precision decimal, with a `positive` bool | §8.1 — the same sign mechanism §8.6 gives a float |
| `satellite.variable.string` | `SatString` | 16-bit code table |
| `satellite.variable.float` | a `bool` and **two `satellite_number`s** — `positive`, left of the point, right of it | §8.6. Left exact and unbounded; right bounded, and its length is the precision |
| `satellite.container.list<T>` | vector of values | children shared |
| `satellite.container.map<K,V>` | body behind a handle | insertion-ordered; keys restricted (§6.5) |
| `satellite.variable.binary` | a run of bits, behind a handle — one `satellite.variable.bool` per bit, packed | §8.5. **The width is part of the value**, so the run's length is not metadata beside it; built at M19.5 |
| `satellite.variable.time` | absolute instant, UTC — int64 nanoseconds on the Unix epoch, `system_clock`'s reading | §13, settled 2026-09-04; built at M13, and display-only until M29 numbers its methods |
| `satellite.variable.file` | handle | reference type |
| `satellite.variable.thread` | handle | reference type |
| `satellite.variable.capsule` | `(capsule number, argument values)` | a deferred call; §13, and `1 6 16` |
| `satellite.variable.variant` | the `Value` itself | §8.7 — no arm of its own, no handle, no bytes; holds any arm, the nothing state included |
| `satellite` | the runtime singleton | `satellite.return(satellite)` |

### 8.1 Numbers — one type, exact, arbitrary precision

"Technically infinite" means **exact arbitrary-precision decimal** — a bignum
significand times a power of ten. Not rationals (denominators grow without bound and
collide with the memory watchdog), not integer-only (`1/3` → 0 is unacceptable), and
**not `double`.**

*(v1: verified by doing the migration.)* `double` is gone from the value variant,
and the guard is at the C++ type level: a template constructor constrained to
integral types **plus deleted float overloads**, so that `Value v = 3.14` is a
compile error rather than a silent truncation to 3.

All three predicted traps fired as real compile errors:

1. After removing `double`, `Value v = 3.14` does **not** fail to compile on its own
   — it silently truncates, with zero warnings under `-Wall -Wextra -O2`.
2. The obvious guard `Number(double) = delete;` then breaks `Number(42)`, because
   deleted overloads still participate in overload resolution.
3. "Keep both `double` and `Number`" is worse than it looks: `Value v = 4096.0`
   silently selects `double` while `Value v = 42` is a hard compile error.

**This argument is about `satellite.variable.number` and does not reach
`satellite.variable.float`.** *(2026-08-27.)* Refusing `double` works here because
every operation on an exact decimal has an exact decimal answer. That stops being
true one type over: `pow` at a fractional exponent has no exact decimal value at any
length, and repeated multiplication of fractions grows digits without bound on a hot
path. §13 has the evidence and the open question. The guard below stays exactly as
it is — what must not happen is reading it as a promise the float can also keep.

The representation is **base-10⁹ limbs**, not base 2³². This backs a *decimal* type,
so decimal I/O and scaling by powers of ten are most of the work, and both are limb
shifts in base 10⁹ against full base conversions in base 2³². Division is long
division one **decimal** digit at a time: the inner loop runs at most nine times, and
getting nine comparisons right is a different proposition from getting base-10⁹
quotient estimation and its correction step right.

#### The sign is a `satellite.variable.bool`, and it is the same one a float uses

*(2026-08-27.)* A number's sign is an explicit `satellite.variable.bool` named
`positive`, **defaulting to `true`** — a number with no sign written is positive, and
something has to flip the flag for it to be otherwise. The magnitude never carries a
sign of its own.

**It is deliberately the same mechanism §8.6 gives a float**, not a parallel one. The
two types are the language's arithmetic and they will be compared, converted and
mixed constantly; one sign rule held in one place is the difference between that being
free and being a source of disagreements nobody can find. Negation, `abs`, the
sign of a product, and the ordering of negatives are then written once and true of
both. There is no negative zero in either.

**This is a change to what gets ported.** PLAN §6.1 brings `satellite_number` across
close to unchanged, and where its sign currently lives has to be checked against this
before the copy — `SCRATCH.md/PORTING.md` has the other four things to settle and this
is a fifth.

**Done at M8 on 2026-08-31, and it is the only part of that port that is not a
port.** The first satellite packed the sign into the significand, so a boxed
magnitude stored `1` or `-1` where a value should be. `Number` now carries a
`bool positive_` and an unsigned magnitude, and **there is no negative zero
because there is no path that can build one** — construction answers a zero
magnitude with a default-constructed value, whose flag is `true`. That is
stronger than §8.6's *"normalise it away on construction"* and it is worth
knowing that the float will not inherit it: §8.6's warning about a
zero-initialised struct reading as `-0` is true there and false here, because a
float's two halves can be zero independently and a number's magnitude cannot.
[MILESTONES/M8.md](MILESTONES/M8.md) is what it cost and what it paid for.

**The small case never allocates.** A null bignum pointer is the fast path, so a
loop counter, an index and every small literal live entirely in a `long long`, and
`i = i + 1` is an add and an overflow check with no allocation and no atomic.
Measured on the first satellite's spacesuit benchmark — a million constructions, a
million prints and a million string transfers — at **2.79 s against the `double`
version's 2.79 s.** The exactness is free at that workload.

### 8.2 A value is inline for the common case

*(new, and it is milestone 7 rather than a follow-up.)* The first satellite made
every intermediate result a `make_shared` — a malloc plus an **atomic** refcount, to
add two integers.

Small exact integers, `bool` and nil must never allocate. A number promotes to the
bignum only when an operation leaves the inline range, and demotes when it fits
again. §8.1's guarantee is untouched: the type is still one exact
arbitrary-precision decimal, and the promotion is a representation detail below the
specification line. **There is still no float, and this is not a machine-double fast
path** — it is one number type with two representations of the same exact value, not
two answers.

Order matters. Fixing dispatch while leaving this alone removes the large win and
keeps the small one, and then reads as evidence that the dispatch work was oversold.

**BUILT AT M9, AND THE FORTY BYTES IS AN ASSERT.** `sizeof(Number)` is 32 and a
variant's discriminator is 8, so a `Value` is 40 with nothing to spare — PLAN
§6.1 called that "the one number that could make this port not fit" and measured
it before a line of M8 was written. `satellite_value/value.hpp` carries the
`static_assert`, and the variant is **append-only**: every arm a later milestone
adds re-runs it.

**FOUR ARMS AT M9, WHICH IS THE ONES A PROGRAM CAN PRODUCE.** Nothing, a bool, a
number and a string. The table above has thirteen rows and an arm with no producer
is a case every later reader has to rule out, so the rest arrive with the
milestone that can build one.

**AND A FLOAT CANNOT BE ONE OF THEM, WHICH M15 DOES NOT GET TO DECIDE.** §8.6
makes a float a bool and **two** `satellite_number`s — 72 bytes laid flat, against
a budget the number alone fills. So a float arrives behind a handle the way a list
and a map do, and that is a consequence of §8.1's exactness rather than a choice.
It is said here because the place it would otherwise be found is a failing
`static_assert` with no explanation attached.

### 8.3 Strings

`SatString` over a 16-bit code table, which *is* the language's alphabet (§5). A
string interner is separate and serves §4.4.

**The sixteen methods' semantics were decided at M11** *(2026-09-03 — they came
to the numbering from QUAD with signatures and no meanings, so the milestone
that built them chose, and MILESTONES/M11.md §2 carries every decision with its
overrule point)*: **positions count from 0** and `substring(start, end)` is
end-exclusive; **where another language hands back a sentinel, satellite
refuses** — `find` on a missing needle, `at` past the end, `to_number` on a
word — because a refusal can loosen into an answer once §8's nil is askable
(M12) while a `-1` written into programs is forever, and `contains`, `size`
and `empty` exist so the question can be asked first; **`append` and `clear`
mutate their receiver in place** under §6.4's storage-slot rule, and a
mutating method's answer is its receiver's new value; `at(n)` answers a
one-character string, there being no character type; and a method sees the
STORED codes — a live code counts as one and resolves at display, per §5's
render/store boundary.

**ONE METHOD IS ON THE OTHER SIDE OF THAT LINE AND IT IS THE ONLY ONE.**
`satellite.variable.string.resolved` `1 6 1 17`, minted by the author at M20,
answers the string with its live codes resolved — the same question `display`
asks on the way to the screen, asked where a program can keep the answer. What
the rule above left behind was six values a program could PRINT and never READ:
`"\memtotal".to_number()` refuses on the placeholder text, and
`total.to_string() == "\memtotal"` is false while both print `63430`, so M20's
own done-when — *assert the three surfaces agree* — could not be written at
all. The sentence above still holds for every other method, and it is what
keeps `size` of `"\threads"` at 1; this row is the door through the boundary,
and it is one call wide because a program asks for it by name.

### 8.4 Maps

Insertion-ordered. Key types are restricted, and the restriction is not a
preference about extensibility — it is §6.5's deadlock one level down.

### 8.5 Binary and hexadecimal

`x00FF` and `b1010` are real types with literals, and **the width is part of the
value**: `x0009` is not `x9`. That is the whole reason they are not number literals
in another base. `hexadecimal` is the language's one alias, for `hex`.

**THEY HAD A LEXER AND NO MILESTONE, WHICH M9 FOUND AND IS RECORDED HERE RATHER
THAN LEFT TO AN AUDIT.** M3 lexes both, this section specifies both, and PLAN §8
gave `satellite.variable.binary` `1 6 5` and `.hex` `1 6 11` to the lexer's
milestone and to no evaluator's — M11 is scalars and names bool, number and
string; M16 is containers. Neither claims these two. `satl --compile` on a program
holding a hex literal said so in those words, which was the most useful form the
answer could take until somebody assigned it. **A binary literal no longer says
it and a hex literal now names M19.5**, which is the same refusal doing the same
job one step further along: a gap became a milestone, and half of that milestone
became a type.

**ASSIGNED 2026-09-08: THEY ARE M19.5's**, minted at M19 because that is the
milestone that gives bytes a destination. `satellite.variable.file.write(x)`
`1 6 2 11` is byte-exact and takes a string at M19; M19.5 makes it take these
two with no edit to the verb. **What this section specified until that
milestone was the LITERALS and the width rule and nothing else** — whether a
bit can be indexed, whether two binaries concatenate, whether either converts
to a `satellite.variable.number` — none of it was written down anywhere, so
M19.5 was a design milestone and not a port, and PLAN §8's entry says so.

#### What M19.5 decided, and what it deliberately left

*(Binary 2026-09-08, hex 2026-09-09, and the split was the author's — binary
first, hex directly after, so that the type's shape could be settled on one
radix before the second inherited it. **Both are now BUILT.** What the split
bought is visible in this section: every paragraph below it that says "and hex
inherits this" was written once and paid for once.)*

**The value is a run of `satellite.variable.bool`, one per bit**, and a hex
value is the same run with a wrapper — four bits to the digit, so nothing is
lost converting either way and neither spelling is the "real" one.

**The width is kept literally and nothing trims it.** `b0010` and `b10` are two
values; `b0010 == b10` is **false**; and display prints what was written,
leading zeros and all. This is the one clause of this section that a
representation can silently break — an integer with a length beside it passes
every other test — so it is the one the milestone's fixtures are built around.

**`==` is by bits AND width, and a bit run is never equal to any other type.**
The second half is not a decision M19.5 took: §8's model has no conversion
anywhere, so different arms compare false, and that is where `b1111 == xF`
will land when hex arrives.

**Six methods on each radix, and the two tables line up row for row.**
`1 6 5 n` and `1 6 11 n` ask the same question for every n, which cost nothing
to arrange and means a reader who learns one has learned both:

| | binary `1 6 5` | hex `1 6 11` |
|---|---|---|
| `to_number()` `n=1` | `b1010` → 10 | `x00FF` → 255 |
| `width()` `n=2` | 4 — bits | **16** — bits, not digits |
| `to_string()` `n=3` | `"b1010"` | `"x00FF"` |
| `as_number()` | `1 6 5 4` — `b1010` → 1010 | **no such row** |
| `digits()` | `1 6 5 5` — 4 | `1 6 11 4` — 4 |
| the other radix | `1 6 5 6` `.to_hex()` → `xA` | `1 6 11 5` `.to_binary()` → `b0000000011111111` |

**Which of the two conversions wore which verb was reversed once during the
milestone** — the author settled it on the ground that `to_` is already this
language's conversion verb (`string.to_number`, `number.to_string`) and that
the surprising answer must not sit behind the name a reader expects.
**Neither conversion keeps the width**, by two different routes, which is why
`width()` exists at all.

**`width()` COUNTS BITS ON BOTH AND `digits()` COUNTS DIGITS**, the author's
call on 2026-09-09, and the reason is one rule downstream rather than taste:
`write(x)`'s "the width must be a multiple of eight" then reads the same on
both types. Had hex's `width()` answered 4, that rule would silently have
become "a multiple of two" for one radix while meaning one thing — the kind of
sentence nobody writes down and everybody rediscovers. `digits()` is why
nothing is lost by it, and it is on binary too so that the question is askable
of a value whose radix the program does not know.

**HEX HAS NO `as_number()`, AND THE ROW EXISTED FOR PART OF ONE DAY BEFORE THE
AUTHOR DROPPED IT.** `as_number()` means "take these characters as a decimal
number", and binary can answer only by an accident of alphabet: `0` and `1` are
also decimal digits, so `b1010`'s characters are the legal decimal 1010. Hex's
are not — there is no decimal number spelled `00FF`. It was built on 2026-09-09
reading the BIT EXPANSION's characters instead, which made it total —
`x00FF.as_number()` answered 11111111 — and the author removed it the same day
on reading that answer: *"x00FF.as_number() = 11111111 looks wrong to me"*.

**The objection is right and it is not really about zeros.** 11111111 is the
decimal reading of characters **no program ever wrote**; `00FF` is what was
written. So one name stood over two different questions, which is worse than
one question having no name. `digits` and `to_binary` moved down to `1 6 11 4`
and `1 6 11 5`; both had been minted hours earlier and seen by nothing outside
this tree, which is the only condition WORD_NUMBERS §1.2's *never renumber*
permits a shift under. **Binary keeps its `as_number()` at `1 6 5 4`**, where
the row does mean what its name says.

**AND THE LEADING ZEROS ARE NOT WHAT WAS AT STAKE, WHICH IS WORTH WRITING DOWN
BECAUSE IT COST A CONVERSATION TO ESTABLISH.** No conversion to a
`satellite.variable.number` can keep a leading zero, on either radix, ever:
§8.1's number has no width, `007 == 7` is **true** in this language and always
was, and `b0011.as_number()` and `b11.as_number()` are both 11 — binary loses
them exactly as hex did. **The zeros are kept by the VALUE**, which stores the
width beside the bits, and they are readable as characters through
`to_string()` and `display`. That is what the width being part of the value
buys, and it is the whole reason these are types rather than numbers in another
base.

**THE TWO CONVERSIONS ARE NOT SYMMETRIC AND THE ASYMMETRY IS THE
MULTIPLE-OF-FOUR INVARIANT SEEN FROM EACH SIDE.** `hex.to_binary()` can never
fail, every digit being exactly four bits; `binary.to_hex()` refuses when the
width is not a multiple of four, because three bits are no hex digit at all and
padding would write a bit the program never wrote. That is `write(x)`'s
refusal one step in, and it is the same argument: the refusal is the only
answer that invents nothing.

**CASE IS NOT PART OF A HEX VALUE, AND IT IS THE ONE THING THE TWO RADICES DO
NOT SHARE.** The lexer takes `x00ff` and `x00FF` alike because the width is
what carries meaning and case does not — so the case is not stored, `x00ff ==
x00FF` is **true**, and `display` therefore CANNOT print back what was written
the way it can for a bit run, whose digits *are* its value. It prints upper,
which is how this section spells every example it has.

**`write(x)` `1 6 2 11` puts the BYTES and refuses a width that is not a
multiple of eight.** A file is made of bytes and half a byte cannot be written;
padding and left-aligning both invent bits the program never wrote, so the
refusal is the only answer that invents nothing. Writing the TEXT is spelled
`write(x.to_string())`, out loud.

**A method needs a number and an operator does not, which is the line the
milestone drew.** PLAN §8 asked for "two numbered paths and no third", and the
twelve rows above are under `1 6 5` and `1 6 11` rather than beside them. `+`,
`!!`, `[` and the shifts cost no path number at all, so a later milestone can
give these types any of them without touching the numbering — **and M19.5
built none of them.**

**THAT IS THE AUTHOR'S CALL OF 2026-09-09 AND THE REASON IS SCOPE, NOT
DOUBT.** Every question below was answered that day; none was built. The
author's words: *"we are doing just too much at one time, and we need to just
pick a few things to do at a time"* — and the observation that earned the
split, arriving as the operators were being specified: *"we forgot about order
of operations like completely"*. Arithmetic on two width-carrying types across
four radix combinations, with a new operator needing a precedence level, is a
milestone; deciding it in the margin of the one that built the values is how it
comes out inconsistent. **So the answers are recorded here as DECIDED AND
UNBUILT, which is what this section is for.**

- **`+` ADDS AND `!!` JOINS.** The author's, and reversed once en route: the
  argument for `+` concatenating is §6.6's note about strings ("take two of a
  thing, answer one of that thing, change neither"), and the author's own
  answer to it is that `+` is the addition sign and hex values are numbers.
  **The cost is named rather than denied** — `+` then joins two strings and
  adds two bit runs, so a reader who expects concatenation gets arithmetic. No
  compile-time signal can catch it, both readings being legal programs; the
  mitigations are the help text and one accidental tell, that joining two
  2-digit hexes is always 4 digits while adding them is 2 or 3.
- **THE ANSWER'S WIDTH GROWS TO FIT AND NEVER WRAPS.** `xFF + x01` is `x100`,
  three digits. Wrapping would drop a bit the program computed and refusing
  would make ordinary addition a thing that can fail; growing matches §8.1's
  unbounded number, which is the answer this language gives everywhere else it
  is asked where a limit goes.
- **MIXED RADICES ARE ALLOWED AND THE LEFT OPERAND'S TYPE WINS.** `xFF +
  b1010` is a hex and `b1010 + xFF` is a binary. **This is the language's
  first operation across two types** and §8's no-conversion model gains a
  stated exception rather than losing a rule — different arms still never
  compare *equal*, which is a separate clause. Because `+` is left-associative
  (§6.6), the consequence states in one line: **the leftmost operand's type
  wins the whole chain.**
- **`!!` BINDS LOOSER THAN `+`, AND BOTH FOLD LEFT TO RIGHT.** `xFF + x01 !!
  x02` is `(xFF + x01) !! x02`. The author asked for one to be picked rather
  than leaving the two at one level, and looser is the reading that makes `!!`
  assemble finished pieces — and that keeps `a !! b == c !! d` meaning
  `(a !! b) == (c !! d)`. It takes a new level 3 in §6.6's table, pushing the
  arithmetic up one. **Nothing becomes right-associative**; the author's first
  instruction was "right to left every time", and it was scoped to `!!` alone
  when the cost of the general form was measured — `10 - 3 - 2` would answer 9.
- **INDEXING COUNTS FROM THE RIGHT: `b0001[0]` IS `b1`.** The author's, and it
  **reverses what this section said until 2026-09-09**, which was leftmost-is-0
  on the ground that §8.3 gives a string 0-based positions from the left. The
  author chose the hardware convention instead — bit 0 is the least significant
  bit, the 1s place — and the cost is real and is recorded rather than argued
  away: **a subscript now counts one way on a string or a list and the other
  way on a bit run.** `[-1]` reaches the far end on both, as everywhere else.
  Nothing is built, so nothing is broken yet.
- **What indexing ANSWERS is a width-1 run of the same type** — `x00FF[0]` is
  `xF` and `b0001[0]` is `b1` — by §8.3's "there being no character type" one
  type over.
- **The shifts are still OPEN**, the one question of the set nobody has
  answered. §5.5's number shifts are not them: that section's "there is no
  width to shift out of" is a fact about `satellite.variable.number` and is
  the opposite of true here.

**AND THERE IS NO `"binary"` MODE WORD ON `satellite.file.open`, WHICH IS A
DIFFERENT QUESTION WEARING THE SAME WORD.** A `satellite.variable.string`
already holds arbitrary bytes — §5 maps one byte to one code and back, and the
raw area holds whatever the table has no character for — and POSIX has no
newline translation to switch off. A mode word that arranged nothing would be
worse than no mode word, because a program that passed it would believe
something had been arranged for it. The mode governs the DIRECTION a handle may
go; what gets written is decided by which verb is called and what value it is
handed.

### 8.6 Floats — two numbers, and the four operations

*(Decided 2026-08-27.)* §13 records the decision; this is the specification.

**A float is a `satellite.variable.bool` and two `satellite_number`s.**

```
    float = (positive, L, R)      value = (positive ? +1 : -1) x (L + R)
```

`L` is the integer part and `R` the fractional part, each an exact base-10⁹
arbitrary-precision magnitude (§8.1). `positive` is the sign of the whole value.
Three invariants:

1. **`L ≥ 0` and `R ≥ 0`** — the halves are magnitudes and never carry a sign.
2. **`R < 1`.**
3. **`positive` is true when `L` and `R` are both zero** — there is no negative zero.

**`positive`, and it defaults to `true`.** A number with no sign written is a positive
number, so the default is the common case and nothing has to say so at a call site.
The field is named for the true case rather than the false one deliberately: reading
`positive: true` is one thought and reading `negative: false` is two, and §1.1's
tie-breaker spends the language's cleverness on the reader rather than on the
implementation.

**The consequence to know before writing the C++**: a zero-initialised struct is
therefore **not** a valid value — it reads as negative zero, which invariant 3
forbids. That is a footgun and also a check: `memset` over one of these is detectable
rather than silent. Construction must set the flag, and §8.1's guard is the model for
making the compiler enforce it.

**Sign-magnitude, with the sign in one place, and that is a correction.** An earlier
draft of this section defined the value as `L + R` with each half carrying its own
sign and an invariant that the two must agree. That works arithmetically and it is
worse: it stores one logical fact twice, so the invariant is a thing a bug can
violate silently rather than a thing the representation makes unsayable. One sign,
held once, is what every real float format does and it is what this does.

```
     3.14  =  (true,  3, 0.14)
    -3.14  =  (false, 3, 0.14)
    -0.14  =  (false, 0, 0.14)
         0  =  (true,  0, 0)
```

**Invariant 3 is not fussiness.** `-0` would compare unequal to `0` while printing
the same, and QUAD's seven sort comparators are all `if (a != b) return a > b` over
floats (QUAD.md §3.3). A value that is neither equal nor orderable against its own
twin is how a deterministic program stops being one. Normalise it away on
construction, not at comparison time.

**Why two halves rather than one number.** §8.1's `Number` can already hold `3.14`
exactly — it is a significand times a power of ten. What it cannot do is carry
**different bounding policies above and below the point**, because its precision is
one count of significant digits across the whole value. The split exists so that
**`L` is never rounded and `R` always may be.** That is the entire reason for it, and
it is what makes a float safe for QUAD's `activation *= keep` on every node every
tick: the growth is downward, so the bound belongs downward.

**`R`'s digit count is the float's precision**, so precision travels with the value.
`satellite.library.system.float_digits` (`1 14 2 4`) is the **default** for a value
that does not state one, not a global dial.

#### normalize — the one shared step

Every operation ends here, and **it is one step, not two.**

- **carry**: while `R ≥ 1`, move `trunc(R)` into `L`.
- then set `positive` if `L` and `R` are both zero (invariant 3).

Sign-magnitude is what buys this. Because both halves are non-negative there is no
sign disagreement to repair, so the borrow step the earlier signed-halves draft
needed does not exist. Carry is exact and cannot round.

#### The four operations

**Addition and subtraction are EXACT and never round.** Two fractions of at most *n*
digits sum to at most *n* digits, plus a carry that `normalize` moves into `L` — and
`L` is unbounded, so nothing is ever lost. This is worth stating plainly because it is
the property `double` does not have and it is free here.

```
    negate                    flip `positive`, unless the value is zero
    subtraction               addition of the negation

    same sign                 normalize(sign, L₁+L₂, R₁+R₂)
    opposite signs            compare magnitudes; subtract the smaller from the
                              larger; the result takes the LARGER's sign
```

**The opposite-sign case is where the code is**, and that is the one cost of
sign-magnitude: a comparison and a conditional swap that the signed-halves draft got
for free. It buys invariant 1, which is worth more — a magnitude that can never be
negative is a state a bug cannot reach, and a sign that agrees with itself is a state
a bug can only be *tested* for.

**Multiplication is four products and then rounds.**

```
    (L₁+R₁)(L₂+R₂)  =  L₁L₂  +  L₁R₂ + R₁L₂  +  R₁R₂
                       ↑exact    ↑ carry into L      ↑ this is what grows
```

`L₁L₂` is an exact integer product and stays in `L`. The three remaining terms are
summed, `normalize` carries their integer part into `L`, and the fraction that
remains is **2n digits wide from an n-digit input** — which is precisely the growth
QUAD's per-tick decay produces, and precisely what the bound on `R` exists to stop.
Round `R` to the result's precision.

**Multiplication and division get their sign for free**, which is the other half of
the trade: the result is `positive` exactly when the operands' flags **agree**,
decided before any arithmetic runs and never revisited. Only the magnitudes are multiplied or
divided.

**Division always rounds, because its answer is usually not finite.** `1/3` has no
terminating decimal. Long division proceeds one decimal digit at a time — v1's
`Number` already does exactly this and already reads a digit budget from
`satellite.library.system.division_digits` (`1 14 2 1`) — producing the result's
precision plus guard digits, then rounding to precision. Division by zero is an error
in plain words (§9), never an infinity and never a silent zero.

**The result's precision is `max` of the operands', floored at `float_digits`.** So
precision never silently shrinks and never grows without bound.

#### Modulus, and why it is exact

`a % b` is `a - b × trunc(a/b)`, and **it never rounds** — which is worth stating
because `/` always does. The quotient is only ever needed as an *integer*, so the
inexact tail of the division is discarded before it can matter: the truncated
quotient is exact, one multiplication and one subtraction follow, and both are
operations on values that already fit.

```
    7.5 % 2.1  →  trunc(7.5 / 2.1) = 3   exact
                  3 × 2.1 = 6.3          exact
                  7.5 − 6.3 = 1.2        exact
```

**Truncated and not floored**, so the result takes the sign of `a` — which is C's
rule rather than Python's, and it is chosen because `trunc` is the free operation
here: under the invariants above, `trunc` is **`L` with `a`'s sign**. Dropping `R` is
the whole implementation.

`satellite.variable.number.modulus(a, b)` is `1 6 4 12`.

#### Power, which returns a float

Three cases, and they are not one operation:

| exponent | how | exact? |
|---|---|---|
| integer, ≥ 0 | repeated multiplication | **exact**, and grows — `R` doubles per squaring |
| integer, < 0 | reciprocal, then the above | rounds, because `1/x` does |
| fractional | irrational in general | **must** round |

`2 ^ 3` is exactly 8 and `pow(0.37, 4.65)` is irrational, and the same path serves
both — which is why **power returns a float and not a number.** A result type that
depended on the *value* of an argument would make `a ^ b` mean two different things
with nothing at the call site to say which.

`satellite.variable.number.power(a, b)` is already `1 6 4 10`.

#### What else — the three classes, which is the useful answer

Every operation on a float falls in one of three groups, and the group says exactly
where the rounding rule bites.

**1 — Exact and bounded. Never rounds, never grows.**
`+` `−` negate `abs` `trunc` `floor` `ceil` `%` `shift_left` `shift_right`
comparison `min` `max` `is_integer` (which is just `R == 0`). These are safe in a
loop forever. Addition and subtraction being here is the property `double` does
not have.

**The two shifts joined this class on 2026-08-31**, when §5.5 was given the
meaning it had been carrying only a spelling for: a shift is multiplication and
division by a power of two, and 2 divides 10, so both directions terminate. **And
this class is a list of which operations M15 does NOT block**, which is what M8
found by reading it: `%` is here, it was scheduled for M15 anyway on the grounds
that it could not be finished before the rounding rule was chosen, and this
section says in as many words that it never rounds. `%` and both shifts landed at
M8 instead. `MILESTONES/M8.md` §6 is the correction.

**2 — Exact but growing. Rounding is a policy, not a necessity.**
`×`, and power at a non-negative integer exponent. The exact answer exists and is
finite — an *n*-digit fraction times an *n*-digit fraction is exactly 2*n* digits —
so rounding here is a deliberate choice to stop growth rather than a mathematical
requirement. **This is the class QUAD lives in**: `activation *= keep` every tick on
every node, where the exact answer after a thousand ticks is thousands of digits wide
and entirely correct and entirely useless.

**3 — Not finite. Rounding is required for an answer to exist at all.**
`÷`, power at a fractional or negative exponent, `sqrt`, and the transcendental family
— `log`, `exp`, and trigonometry — none of which is numbered yet and all of which
belong together whenever they arrive. `1/3` and `pow(0.37, 4.65)` have no terminating
decimal, so there is nothing to round *from*; the rounding is what produces the value.

`satellite.variable.number.truncate(a)` is `1 6 4 13` and `.sqrt(a)` is `1 6 4 14`.
Sign-magnitude makes two of class 1 nearly free: **`abs` sets one bool** and
**`trunc` is just `L`**.

**Conversion, for completeness.** A number becomes a float as `(n, 0)`, which is
exact and always succeeds. A float becomes a number by `trunc`, `floor`, `ceil` or
`round`, and **which one is never chosen silently** — §1.1 forbids a silent
conversion, so the program names it.

#### Comparison

Compare `positive` first — a true sorts above a false, and invariant 3 is what makes
that safe, since there is no `-0` to be unequal to `0`. Within one sign compare `L`,
then `R`, reversing the result when both are negative.

Total and exact with no normalisation first, which matters because QUAD's seven sort
comparators are all `if (a != b) return a > b` over floats (QUAD.md §3.3).

#### What this costs to build

**No new arithmetic.** PLAN §6.1 has `satellite_number` at 10 files and 1509 lines,
internally closed, porting as-is. Every operation above is composition over two of
them plus `normalize`, and the only genuinely new code is the rounding step.

#### The rounding rule — chosen at M15

**ROUND HALF AWAY FROM ZERO.** *(2026-09-04, at M15, delegated — this section
read "Still open" from 2026-08-27 to that day.)* Wherever the language rounds
— a float's right half, `Number::divide`'s guard digit, `round()` — a tie
leaves zero. Because every rounding here runs on a MAGNITUDE, with the sign
decided before any arithmetic and never revisited, half away from zero and
half up are one motion; the rule is stated in the away-from-zero form because
that form needs no footnote about negative values.

It could not be avoided by any choice of representation: `rack.hpp:59`
computes `pow(urgency, exp)` with `exp` always fractional, and `x^y` at
fractional `y` is irrational, so **no pair of exact numbers represents it**
and `R` must be rounded to exist. QUAD's determinism invariant means a
program's behaviour depends on which rule is chosen — it demands A rule, not
any particular one, which is what made the choice delegable.

**The decision is a ratification, and that is the argument rather than an
apology.** MILESTONES/M15.md §2 carries it in M12 §2.1's shape — the tree had
already taken the decision three times and the milestone's job was to notice:
`Number::divide` bumps its guard digit at `5` (v1's behaviour, ported at M8,
asserted by `tests/number_test/arithmetic.cpp`); `Number::round()` documents
"half away from zero"; and sign-magnitude rounds magnitudes, so any rule
chosen here had to agree with itself across the sign. **Truncate** would
contradict `divide` at every call `satl --number 2 / 3` has ever answered;
**half-even** would contradict both sites, and its virtue — bias cancellation
over long accumulations — defends against a failure this type does not have,
because addition and subtraction are exact and the classic accumulation cases
never round at all. The number's division therefore AGREES with the float by
construction, and that test file is the tripwire that makes any future change
of rule a visible edit.

**It blocked three operations and not six, which is narrower than PLAN read
it.** *(2026-08-31.)* The classification above is the list: `power` at a
fractional or negative exponent and `sqrt` are class 3, and `truncate` on a
float is its left half and therefore waited on the float rather than on the
rule. `modulus`, `shift_left` and `shift_right` are class 1 and were never
waiting on anything — PLAN §8 had all six under one sentence, and M8 built
the three that class 1 already answered. All three waiters answer since M15,
each returning a float per this section's result-type argument, except
`truncate`, which stays a number because its answer is one by definition.

### 8.7 The variant, and what "nothing" is

*(New 2026-09-03, at M12. PLAN §8's M12 entry held the blocker — "is 'nothing'
a state every type has, or a value only a `variant` can hold?" — and the author
delegated the answer on the day it landed. MILESTONES/M12.md §2 carries the
full argument; this section is the language's statement of it, and M14 and M19
read it here.)*

**Nothing is a state every type has.** A declared variable of any type holds
nothing until a value is assigned to it — §6.4 qualification 3's second
non-dispatchable state, S0714's exact sentence — and nothing may be handed
around: assigned, passed, returned, put into a `variant`. A capsule that falls
off its end answers it. What nothing is NOT is dispatchable or convertible:
methods asked of it refuse by name at the line it happened (S0714), only a bool
is a condition, and equality against a different arm answers false without
error. The state is universal; the *vocabulary about it* belongs to one type.

**`satellite.variable.variant` is represented as the `Value` itself.** M9's
discriminated union is the representation — no arm of its own, no handle, no
allocation, and §8.2's forty-byte assert does not move. A slot declared
`variant` holds whatever `Value` it holds, and the declared type does the one
thing a declared type does anywhere in this language: it numbers the selectors
(§6.4, WORD_NUMBERS §1.5). What it numbers is the asking vocabulary, the only
method family for which the nothing state is an answer rather than a refusal:

- **`holding`** `1 6 14 1` — the word for what it holds: `"nothing"`,
  `"bool"`, `"number"`, `"string"` or `"satellite"`, one word per arm of
  §8.2's variant, growing as arms are appended. Never refuses.
- **`holds(x)`** `1 6 14 2` — the same question as yes or no. `x` off the
  word list is refused (S0713), never answered false: a typo'd word answered
  false forever is a condition no program can satisfy, and the refusal can
  loosen the day a new arm makes the word real.
- **`held`** `1 6 14 3` — the value itself, or S0714 by name. Plain
  assignment out of a variant already copies whatever it holds, nothing
  included; `held` is for the program that means *"there is something in
  here, and stop me at this line if not."*
- **`clear`** `1 6 14 4` — the receiver becomes nothing, under §6.4's
  storage-slot rule and §8.3's mutating contract: the answer is the
  receiver's new value. Clearing an empty variant answers nothing — the
  method promises a state, not a transition.

**The other reading was declined, and the losing argument is worth keeping.**
Under reading two — nothing as a value only a `variant` can hold —
`satellite.variable.string line = my_file.read_line()` is a type error at end
of file. That reading requires an assignment type-check this language has
nowhere (op_store is checkless; the declared type is a dispatch key, not an
enforced invariant), splits "nothing" into an unassignable state and an
assignable value that are byte-identical in the slot, and forces every
end-of-file loop through extraction ceremony §1.1 exists to abolish. What it
offered — a refusal at the assignment — survives in opt-in form as `held`.

**What M14 and M19 inherit:** `satellite.console.typed()` and
`satellite.variable.file.read_line` answer *a line, or nothing*, assignable
anywhere. The caller who handles the nothing case declares a `variant` and
asks; the caller who declares a `string` has written a legal program that, at
end of file, refuses by name at the line of first use. An empty line and
nothing are two different values — §8.2's arms, told apart by `holding` — and
never one empty string.

---

---

## 9. Errors are part of the interface

*(new — this is a deliberate break from the first satellite, and it is built
**before** the evaluator rather than retrofitted onto it.)*

The first satellite ended with **199 `fail()` call sites**, each composing its own
message string — 224 distinct string literals between them — with no codes, no call
stack and no suggestions. *(Counted in `old_versions/first_satellite/src/` on
2026-08-26: 216 occurrences of `fail(`, less 13 in comments and 4 signatures. String
literals counted with C++ adjacent-literal concatenation merged; without merging it
is 266. `PLAN_ONE.md` said 177 call sites, which nothing in that tree supports.)*

It does have a source excerpt and a caret, and that is worth saying because it is
the half that was done right: three `format_error` implementations — one each for
parse, resolve and evaluation — render the offending line with a caret under it.
What is missing is everything that makes an error *addressable*: a code to look up,
a stack to place it in, a suggestion to act on. That is what retrofitting an error
reporter looks like.

The shape here:

```
{ ErrorCode, Span, vector<Note>, vector<FrameRef> }
```

with **rendering in exactly one place**. Spans on every node. A source excerpt with
a caret. Notes carrying their own spans. And "did you mean" over the trie level that
failed (§4.6).

Errors are **full sentences that name the fix**, not labels. The model:

> `'x1' is a hexadecimal literal, so it cannot name a variable`

### 9.1 Not by throwing

The decision *not* to throw is kept, and it is measured: **8.5 ns as an enum against
1537 ns thrown — 181×.**

But 199 sites where a missed null check is a segfault is the wrong shape. The
replacement is either a sticky machine flag checked at statement boundaries, or a
return type that **cannot be silently dropped**. "Record the error and return
`nullptr`" is what goes.

---

## 10. The console, threads, and windows

### 10.1 The console owns a printer thread

*(Both halves are built. The output half at M10, 2026-09-02, in
`src/satellite_console/`: `display`, the queue, the printer thread and the
barrier. The input half at M14, 2026-09-04 — the reader thread in `reader.cpp`,
`input` `1 5 2`–`1 5 4`, `typed()` `1 5 5`, the terminal's facts and the two
queued escapes — which finished the `console` namespace: §2.2 has no tenth
child. [MILESTONES/M14.md](MILESTONES/M14.md) is the review.)*

Producer threads push whole strings into a locked queue; one printer thread
consumes. **A line stays atomic because the unit queued is a whole string.** There
is a `drain()` barrier before reading input, so a prompt cannot appear before the
output that explains it.

**The barrier is the first step of the shutdown and not a second mechanism.**
*(The author, 2026-09-02.)* The console stops in four steps — **drain** until the
queue is empty, **flush** the fd, **stop** the queue to new work, **join** the
printer. `satellite.console.input` takes the first and carries on;
`satellite.return` from `satellite.main` takes all four. So the same wait is on
the exit path of every program that prints, which is what makes the barrier
buildable in the milestone that has no input yet.

**And the printer creates its own thread**, rather than taking one from PLAN
§4.5's pool. *(The author, 2026-09-02; six places in the plan said the opposite
until that day.)* That pool takes work that FINISHES — a range, a split and a
join — and a printer waits on a queue for the life of the run, so a resident
tenant would hold a worker forever and be counted as available.

**The queue is unbounded, and that is a decision.** A bounded one makes a
producer wait when it fills, which is a threshold nobody chose appearing in the
most-called path in the language — the hidden constant §1.1 refuses and §7.5
rules out in general. The bound is memory, the way a list's is.

**The same shape, reversed, is what non-blocking input is.** *(new, 2026-08-27.)*
§10.3 observes that a GTK main loop is "the shape §10.1 already has, with the
polarity flipped." Flip it once more for input: a **reader thread** blocks on stdin
and pushes whole lines into a queue, and the program asks the queue and gets a line
or nothing, immediately. `satellite.console.typed()` at `1 5 5` is that ask.

This is what lets a program keep running while somebody types at it without the
program ever learning what a terminal mode is — §1.1 applied to input. The blocking
happens on a thread that is not the program's, which is strictly better than the
poll loop the obvious alternative produces. `satellite.console.input` keeps its
existing meaning: ask, and wait.

**STDIN HAS EXACTLY ONE CONSUMER AT ANY INSTANT, AND IT IS THE READER.**
*(The author, 2026-09-04, with M14's other answers.)* `input` rides the same
queue — the invariant above already decided that, since a `getline` on the
walking thread would be the program's own thread blocking on the terminal —
and nothing else in the process reads descriptor 0. The reader never sits bare
in `read(2)` either: it parks in `poll()` on stdin and a control pipe, which
is how a Ctrl-C wakes it no matter which thread the kernel chose (§10.2), how
the shutdown joins it cleanly, and how M22's raw-mode prompt — the language's
other reader, never live at the same time — will park it before taking the
terminal: whoever owns stdin next tells the pipe first.

`satellite.console.display` is ordinary stdout, and the flush is required for the
cases line buffering does not cover. glibc line-buffers stdout only when it is a
tty, and that case needs no help — the newline flushes it. The flush is for the
other two: stdout redirected to a pipe or a file, where glibc buffers fully and
output can sit unwritten indefinitely, and a prompt written with no trailing
newline, which line buffering would hold back on a tty as well.

### 10.2 Ctrl-C means two different things

*(v1: hard-won; do not rediscover.)* The handler is installed **without
`SA_RESTART`**, and `eof()` is what tells a genuinely closed stdin from a read
interrupted by the signal.

Ctrl-C at the prompt cancels the line being typed. Ctrl-C in a running program stops
it at its next statement boundary. Neither is the same as taking the session.

*(The running-program half was built at M11, 2026-09-03, as v1's handler ported
whole: the first press sets a lock-free flag the machine reads at every
statement boundary — S0730 puts the caret under the statement that did not run,
the console drains first so everything the program said is above it, and the
exit is 130 — and a second press leaves at once through `_exit(130)`, the same
number. The prompt half stays M22's, arriving as the byte `0x03` because raw
mode turns ISIG off.)*

*(And M14 gave the flag its reach into blocked waits, 2026-09-04: the handler
also writes one byte into the reader's control pipe — `write(2)`, its one
verb — so a program parked at `satellite.console.input` wakes at once whatever
thread took the signal, answers nothing, and stops at the boundary exactly as
above; `satellite.time.sleep` wakes through plain `EINTR` the same turn. What
`eof()` used to discriminate — a closed stdin against an interrupted read — the
reader's queue now answers structurally: the end and the interrupt arrive as
two different answers and cannot be confused, which retires §6's hard-won
regression by construction rather than by care.)*

### 10.3 GTK is not thread-safe, and there is no threadable alternative

Not SDL, not Qt, not FLTK, not raw Wayland. This is a platform constraint, not a GTK
failing: Win32 gives windows thread affinity, macOS AppKit is main-thread-only, and
Wayland and X11 connections are not safe to share. GTK3's global lock was deprecated
in 3.6 and **GTK4 removed it entirely.**

The supported model is **one UI thread plus a message queue** — which is exactly the
shape §10.1 already has, with the polarity flipped. satellite already has the idiom;
it has not pointed it at a window yet.

So §1.1 decides the surface: the user writes `satellite.window.new(...)` from any
thread and **satellite does the marshalling silently. The user never learns the words
"main thread."**

### 10.4 `satl` with no console hands itself to `satl-term`

*(Decided and built 2026-08-28.)* Started from a file manager, a desktop menu or a
`.satl` file association, `satl` has nowhere to print: its output goes to
`/dev/null` or to the session journal, and **a program that runs correctly and shows
nothing is indistinguishable from one that did not start.** That is §1.1 broken in
the way §1.1 cares about most — the work was done and the person was not told.

So `satl` re-execs `satl-term`, which has a screen, passing `--hold` and its own
arguments through. The window then spawns the `satl` beside it and that copy runs
with a pty for a console.

**The obvious test is `isatty(stdout)` and it is wrong.** A pipeline has a pipe on
stdout and answers *no*, so `satl --words | grep console` would open a window instead
of feeding the pipe — every script on the machine, broken, to fix a case none of them
are in. The question that separates a launcher from a terminal is whether the process
has a **controlling terminal**, and `/dev/tty` is the file that answers it: it fails
with `ENXIO` when there is none, and a pipeline still has the shell's.

**There is no recursion and it is not luck.** `satl-term` spawns its child on a pty,
so the child has a controlling terminal and answers the question the other way the
first time it asks. The condition is a fact about the process rather than a flag
somebody has to remember to clear.

**Six refusals, and each is a case where a window would be the wrong answer**:
`SATL_NO_WINDOW` is set; there is a controlling terminal; stdout is a pipe or a
regular file, so something is deliberately reading; there is no display; there is no
`satl-term` beside us, which is ordinary on a build without gtk4; or the `execv`
failed. `--no-window` is a seventh and is a flag rather than a condition.
`src/programs/window_handover.cpp` is the whole of it and names all six.

### 10.5 Threads

`satellite.variable.thread`. §7's frames are what make a capsule call safe to run on
one; PLAN.md §2.2's arena is what makes walking the program **atomic-free** rather
than merely safe.

---

## 11. `satellite.random`

*(Built at M13, 2026-09-04 — the twelve numbered shapes in
`src/satellite_random/handlers.cpp`, the shapes' arithmetic in `tiers.cpp`,
and the seam and the spin that landed ahead of their milestone at M2 and M8
finally consumed by the language. [MILESTONES/M13.md](MILESTONES/M13.md) is
the review.)*

Three tiers, differing in nothing a program can see except how long they take:

| tier | throwaway window |
|---|---|
| `satellite.random.fast` | 50–300 ms |
| `satellite.random.normal` | 500–600 ms |
| `satellite.random.ultra` | 2000–3000 ms |

*(Windows set 2026-08-27; `fast` widened from 50–100 and `normal` moved up from
250–300.)* **The duration is itself random, drawn inside the window**, so the length
of the spin is not a constant an observer can rely on.

**The throwaway is whole numbers of the size being asked for, not raw words.** A
request for a 40-digit number spins by generating and discarding 40-digit numbers;
a request for 512 digits discards 512-digit ones. The discarded work is the same
work as the answer, which is what makes the window mean anything — and it replaces
the first satellite's arrangement, which folded 32-bit draws into a `uint64_t` and
used that as a *seed* for a second generator. That funnelled all 524,544 bits of the
first generator's state through 32 bits, and v1's own source says so at
`random_numbers/random.cpp:112` and recommends answering from the first generator
instead. This is that recommendation, taken.

**Four numbered shapes on each tier, and WORD_NUMBERS §2.2 is the roster.**
*(2026-09-04. This read "two shapes on each" from the section's first commit —
one commit older than the numbering, and true of the first satellite's surface —
and neither numbering commit touched it. What follows says which numbers are
specified, so the next drift has a date to look wrong against.)*

- `<tier>(digits)` — `1 7 4`, `1 7 7`, `1 7 10`. Specified here, built and
  tested in v1.
- `<tier>(min, max)` — `1 7 5`, `1 7 8`, `1 7 11` — with `.range(min, max)` a
  second spelling of the same number, not a fourth segment (WORD_NUMBERS §2.3).
  The same.
- `<tier>(min, max, step)` — `1 7 6`, `1 7 9`, `1 7 12`. *Specified 2026-09-04:*
  uniform over `min, min + step, ..., max`, and the call is valid **only when
  `step` divides `max - min` exactly** — otherwise it is refused, and the
  refusal names the last value the step reaches: `fast(1, 10, 4)` is refused
  naming 9, and `fast(1, 9, 4)` is what the program meant or the program is
  wrong. That keeps this section's promise — inclusive at both ends — true of
  every range call that answers; a refusal can loosen later, and a draw that
  silently never says `max` is forever.
- `<tier>()` — `1 7 1`, `1 7 2`, `1 7 3`. **A refusal by design** *(the author,
  2026-09-04)*: **"you must supply a digit_count, min and max, or min max and
  step."** A bare tier spelling folds to the same number, and a path is not a
  value. *(The answer not taken, recorded so it is not re-proposed: inferring
  the digit count from the assignment's destination — `n` holding 1000000
  making `n = fast()` a seven-digit draw — was taken far enough on 2026-09-04
  to know M11's slot channel could carry it, and declined the same day: one
  spelling would draw differently depending on the statement around it.)*

```satellite
satellite.variable.number n = satellite.random.ultra(40)
satellite.variable.number m = satellite.random.ultra.range(1, 100)
satellite.variable.number s = satellite.random.fast(1, 10, 3)
```

`ultra(40)` is **uniform over [0, 10⁴⁰)** — zero through forty nines. `.range(low,
high)` is **inclusive at both ends**, which is what makes `.range(1, 100)` able to
answer 100.

**About one draw in ten of a 40-digit request prints 39 digits or fewer**, because a
leading zero is not printed and a uniform draw has one a tenth of the time. That is
what uniform means; a draw that always printed 40 digits would not be one. It is
said out loud here, in the header, and in the test, because it is *not* what the name
promises and someone will otherwise file it as a bug.

### 11.1 The tiers are a statistical character, not a security property

PCG makes no cryptographic claim and its state is recoverable from its output.
**No tier here is secure and none of them is described that way** — not in the
header, not in the help text, and not here. `ultra` is slower and better
distributed. It is not a CSPRNG, and if the language ever wants one it will be a
different path with a different name.

---

## 12. Deliberately deferred

A deferral list is only useful if the things missing from the language are on it,
and equally only if the things on it are still missing.

**[QUAD.md](QUAD.md) put three of these under pressure, and reading QUAD's source
on 2026-08-27 released two of them.** The argument had been that the program this
language exists to express needs sorting, that sorting needs a capsule passed as a
value, and that this forces *a bare name can be a value* out of the list. It does
not.

- **Sorting needs one primitive, not comparators.** All seven of QUAD's sort
  comparators are the same shape — one numeric key descending, ties by identity
  ascending — and `flock.hpp:232`'s own comment says why the tie-break is there
  (`// deterministic ties`, because `std::sort` is not stable). Seven for seven.
  `satellite.container.list.sort_down(key)` at `1 4 2 6` is that primitive.
- **The one place QUAD genuinely stores behaviour is a deferred call, not a bare
  name.** `rack.hpp:22` holds a `std::function` in a field, and eight of its eleven
  uses capture nothing while the other three capture scalars *by value*. §13's
  `satellite.thread.new(capsule_name(args))` already had to express exactly that
  for the language's own reasons, and it is a **call form** rather than a bare name
  used as a value.

So §2 stays shut and the first entry below stands. What QUAD did move is
`satellite.variable.float`, which is in §13 rather than here, and which is on the
critical path.

**AND ONE ENTRY THAT WAS NEVER ON THIS LIST AND SHOULD HAVE BEEN, RECORDED AS
TAKEN RATHER THAN AS DEFERRED.** PLAN §2.5 deferred the **explicit control stack**
from the day the plan was written until 2026-08-31, and this list never carried
it — which is the failure the opening sentence names, one direction round: a thing
missing from the language was not on it. It is not missing any more. M8.5 built it
for the four static passes and M9 for the evaluator, so §7.5's rule is met end to
end. It is written here because a deferral list that only ever grows cannot be
read against the language, and because the argument that released it is worth
finding from this end: a bound is not a cheaper version of a stack, it is a
different product.

- **User-defined generics** — a bare name can be a value, which reopens §2.
  *(Still deferred, 2026-08-27.* The deferred-call form
  `satellite.thread.new(f(x))` does **not** reopen this: `f(x)` is a call, which
  §6.2's postfix loop already produces, and the only new semantics is that one
  handler packages its argument instead of performing the call.)
- **Durations** — `time` is an absolute instant only; subtraction yields a number of
  nanoseconds.
- ~~**The explicit control stack.**~~ **TAKEN 2026-08-31, and it was never on this
  list — which is the failure this section's own opening sentence describes.** *"A
  deferral list is only useful if the things missing from the language are on it."*
  PLAN §2.5 has deferred it since the plan was written, under a heading that says
  *deferred, not dropped*, and it was invisible here — so the one entry that turned
  out to be load-bearing was the one a reader of this list could not find. It is
  listed now, struck, because §7.5's rule cannot be kept without it: a walker that
  keeps its own stack has no depth limit, and every other answer is a bigger
  number.
- **A JIT.** Not built and not planned. satellite is interpreted and there is no
  compile step the user ever runs; PLAN §2's closure compilation happens on the way
  to the first execution and emits callables, not machine code. This is the first
  question a tree-walking interpreter is asked, so the answer belongs in the list
  rather than in someone's memory. **`.satc` is not a counter-example** — it is a
  cache of §4's numbering applied to a source file, written after the program has
  already started, and deleting every one of them costs a walk. [SATC.md](SATC.md).
- **satellite written in satellite.** Not planned, and unlike most entries here it is
  not deferred — it is a consequence of the two decisions above. What self-hosts in
  other languages is a **compiler**: it emits machine code and the output runs with
  the host gone. satellite has refused that twice on purpose — no JIT here, and PLAN
  §2.1 rules copy-and-patch out of scope in one line, *"it is a compiler."* A
  tree-walking interpreter cannot self-host its host away; satellite interpreting
  satellite is slower forever with no compile step to ever cash the loss back in.

  §6.5 already says this about the narrowest possible case — a map's hash and equality
  *"must be native and can never be satellite code"* — and the general rule is the
  same sentence without the hat. **[WORD_NUMBERS.md](WORD_NUMBERS.md) is the map of
  it:** a path has a number because dispatch lands in a native handler, so its 215
  numbered paths are 215 things that are C++ and cannot be anything else. Written in
  satellite, `list.sort()` would need no number, because there would be no
  `handlers[path_id]` to index.

  So the C++ **grows with the language's ambition rather than shrinking**, which is
  §1.1's tie-breaker read from the other side: *do absolutely everything for the user*
  is a promise kept in native code — the base-10⁹ arithmetic, the printer thread,
  SIGINT without `SA_RESTART`, the GTK marshalling nobody ever learns the words for.
  A language that did less for people would have less C++ under it. The consequence
  worth planning for is that this tree's C++ is a permanent artifact with the same
  lifespan as the language, not scaffolding to be removed later — which is what
  PLAN §3's line ceiling and [FORMAT/CXX.md](FORMAT/CXX.md) are for.
- **Bare field access** (`my_object.my_field`) — accessor methods only. A spacesuit
  field is reachable from inside the spacesuit and nowhere else, which is what makes
  `satellite.protected` a statement about the language rather than a comment.
- **`<<` and `>>` operators** — permanently, per §5.5.
- **A static type checker** — types are checked at runtime, at declaration and at
  insertion. Rejecting a malformed *container* type before anything runs is shape
  checking, not type checking.
- **`m[k] = v` and `l[i] = v`** — assignment resolves a storage slot and an index
  expression names none. Fixing one fixes both.
- **`satellite.container.set`, and a deque** — *(considered and declined,
  2026-08-27.)* All five of QUAD's `std::set` are membership tests, which a map with
  no values answers, and both its `std::deque` are bounded ring buffers, which
  `list.remove_first()` at `1 4 2 16` and `truncate(n)` at `1 4 2 14` answer. `1 4 5`
  is free if something later earns a real set.
- **An in-place fast path for container mutation** — every append and set copies the
  whole body, which makes building a container quadratic. Safe only when the slot's
  handle is unshared, and then only for a **frame** slot; a field or a global may
  have a reader holding a snapshot.
- **Garbage collection** — and the original reasoning **no longer holds.** It was
  that refcounting suffices because cycles cannot be constructed: every value was
  immutable, so nothing could be made to point at something pointing back. A
  spacesuit instance is mutable, so two objects can name each other and neither is
  ever freed. This is a real leak, it is the price of reference semantics, and it is
  **not fixable by being careful** — a cycle is a shape a correct program can want.
  The cheap partial answer is a weak-reference field; the complete one is a tracing
  collector over the object table.
- **Passing constructor arguments to a superclass** — a `super(...)` form.
- **User-defined operators**, and a spacesuit `to_string` the printer consults.

---

## 13. What is decided, and what is open

### Decided

- **Return type syntax** is an optional `satellite.returns(TYPE)` after the
  parameter list, defaulting to the `satellite` type — which leaves hello world
  byte-identical. A constructor is the one member forbidden to declare one, since
  what a constructor produces is the object.
- **User capsules are bare.** `fact(3)`, with `satellite.main` the reserved
  exception, which is §1 read straight.
- **Statements are newline-terminated.**
- **Control flow** is `satellite.statement.if/while/for`, so `statement` joins
  `variable`, `container` and `library` as a segment-1 dispatch key and §1 holds with
  no exceptions anywhere.
- **`for` is three-part and C-shaped**, introducing no syntax the language did not
  already have: `;` is ordinary punctuation, the init clause is the same declaration
  a statement produces, and the step clause is the same assignment. All three parts
  are optional, so `for(;;)` is the infinite loop.
- **`satellite.container.list` stays.** Two type namespaces cost the parser one
  extra comparison, and `container` earns it by saying something `variable` cannot:
  this type holds other things and takes a generic parameter.
- **Classes are `satellite.spacesuit`**, with `satellite.protected` / `.public`
  blocks and a bare-name type. Reference semantics. **`satellite.class` is a
  second spelling of it** — an alias row on `1 10`, added 2026-09-09 for §7.7's
  reason about `arguments`: `class` is what every other language calls this and a
  reader writes it before reading a line of this document. `spacesuit` stays the
  word — it is the node, it is what `satl --unparse` prints, and it is what
  `satellite.help` heads the entry with. **The header takes five forms and each
  means one thing**: `IDENT [ "(" [ IDENT ] ")" ] [ ":" ]`, so the empty pair and
  no pair both say *no superclass* and a trailing colon says nothing. What is
  refused is what means nothing definite — an unclosed paren, and a second name
  inside it, which would be multiple inheritance.
- **A unit of includable code is a spaceship.**
- **Operator precedence** is §6.6's four levels, all left-associative, with unary
  `-` and `!` above them. *(Decided at M4, 2026-08-30, because the parser could not
  be written without it — §6's expression rule read `... precedence climbing ...`
  and named no operators.)* **A fifth level is decided and unbuilt**: `!!` joining
  two bit runs, looser than `+` and left-associative like the rest (§6.6, §8.5;
  the author, 2026-09-09).

### Open

- **What `&` means, and whether `!` is really the negation.** *(Opened at M4,
  2026-08-30.)* The lexer will hand over a bare `&` as a Punct and nothing in this
  document says what it is; §6.6 therefore gives it no precedence, so it ends an
  expression and is reported. The question is one question and not two: satellite
  has `==` and `!=` but **no `and`, no `or` and no `not`**, so a program that wants
  to test two things has no way to write it, and whichever answer is chosen — words
  in `words.def` under §1's generating rule, or the C symbols — decides `&`, `!`
  and the missing pair together. **`!` is currently parsed as a unary and given no
  meaning**, which is the smallest guess available and is still a guess.
  M4 chose it; this is where it gets taken back or kept.

- **`satellite.variable.float` — DECIDED 2026-08-27: a `satellite.variable.bool` and
  two `satellite_number`s.** A `positive` flag defaulting to `true`, then left of the
  decimal point and right of it, each an exact base-10⁹ arbitrary-precision magnitude.
  §8.1 now carries the same sign mechanism, so a number and a float agree about what
  negative means by construction rather than by review. That is *"infinitely long in both
  directions"* read literally, and it is the author's decision.

  **The left half is exact and unbounded. The right half is bounded, and that is
  where every hard question lives.** Putting the bound there and only there is what
  makes the representation work:

  - repeated multiplication grows digits *downward* — `activation *= keep` every
    tick, every node, in QUAD's `sky.hpp:355` — so bounding the right half bounds
    the growth without ever truncating a magnitude
  - a program that counts to a trillion and a program that decays a weight for a
    thousand ticks want opposite things, and this gives each of them theirs

  **The right half's length IS the precision**, so precision travels with the value
  rather than living in a global. `satellite.library.system.float_digits`
  (`1 14 2 4`) therefore stops being "the dial" and becomes **the default length of
  the right half** for a value that does not state one. That matters for the program
  this language exists to run: QUAD holds activations at about six significant digits
  (`sky.hpp:461` round-trips its whole state through `operator<<` and works) and
  prints them at two.

  **What this costs to build: no new arithmetic.** PLAN §6.1 has `satellite_number` at
  10 files and 1509 lines, internally closed, porting as-is. A float is composition
  over two of them.

  **~~What is still open is the rounding rule~~ Settled 2026-09-04, at M15,
  by delegation: ROUND HALF AWAY FROM ZERO.** It had to be stated, because the
  answers genuinely differ and QUAD's determinism invariant means a program's
  behaviour depends on it; it could not be avoided by any representation:
  `rack.hpp:59` computes `pow(urgency, exp)` with `exp` always fractional, and
  `x^y` at fractional `y` is irrational — **no pair of exact numbers
  represents it.** The right half has to be *rounded to exist*, which is why
  the rule is part of the type rather than a setting on it. §8.6's rounding
  section carries the rule and the ratification argument — the tree had
  already taken the decision three times, and the two declined rules are
  recorded beside the taken one, M12's shape.

  **§8.6 is the specification** — the invariants, `normalize`, and the four
  operations, of which addition and subtraction turn out to be **exact**.
  **M15 owned this** (PLAN §8; this sentence said M11 until the renumber) and
  landed it on 2026-09-04 — MILESTONES/M15.md is the review, and the retune
  landed beside it: assignment to a `1 14 2` dial is a handler-shaped write
  into the running machine's Policy, evaluator/dispatch.hpp's Assigners.

  *(The two readings not taken, recorded so they are not re-proposed: **numerator and
  denominator** is the rational §8.1 already refuses — denominators grow without
  bound, and QUAD's per-tick decay is exactly that failure. **Significand and
  exponent** is not two numbers at all; §8.1 defines a single `Number` as "a bignum
  significand times a power of ten", so that reading makes float and number the same
  type.)*

- ~~**`satellite.variable.float` — the earlier framing.**~~ It was posed here as a
  choice between *lazy* and *bounded by a precision dial*, and reading
  QUAD's source on 2026-08-27 showed that framing is answering the wrong question.

  **A dial is not a convenience. It is the only way some of these answers exist.**
  `quad_infinity/rack.hpp:59` computes `pow(urgency, exp)` where
  `exp = 0.15 + 4.5 * (1 - t)` — always fractional, 0.15 to 4.65. `x^y` at
  fractional `y` has no exact decimal value at any length, so an exact
  arbitrary-precision type cannot represent it and "lazy" has nothing to be lazy
  about. The result has to be **rounded to exist**, which means a rounding rule is
  part of the type rather than a setting on it.

  **And exactness actively fails on the hot path.** `sky.hpp:355` runs
  `activation *= keep` every tick on every node, where `keep = 0.15 + 0.85·alt²`.
  Exact decimal multiplication adds the operands' digit counts, so a thousand ticks
  is thousands of digits per node — for a value the program prints at `%.2f`.

  **How much precision is actually needed is already on record.** `sky.hpp:461`
  writes the entire mind to its `.sky` file with bare `operator<<` — six
  significant digits — and reads it back, every save. The program round-trips its
  whole state through six digits and keeps working.

  So the open question is now: what is the rounding rule, what is the default
  precision, and where does it live. `satellite.library.system.float_digits` is
  numbered at `1 14 2 4` as the dial's home, beside `division_digits`, which may
  already be the same knob under a narrower name.

  **This also puts §8.1 under pressure and that needs saying where §8.1 is.** Its
  argument — no `double`, exact always, guarded at the C++ type level — is right for
  `satellite.variable.number` and is not available to `satellite.variable.float`.
- ~~**Time.**~~ **Settled 2026-09-04 by the author, three decisions at once.**
  The question was that `satellite.variable.time`, `.date` and
  `satellite.time.now()` must agree on **one clock and one epoch**, and that a
  high-resolution monotonic clock and a wall-clock date are not the same clock
  and cannot both be the one. **They need not both be the one, because only one
  of the two is ever a satellite value.** v1's split is adopted whole: the
  *value* — what `satellite.time.now` answers and a `satellite.variable.time`
  holds — is `system_clock` on the Unix epoch, int64 nanoseconds, because a
  value has to mean something outside the process that read it; the *timer* —
  §11's spin deadlines and `satellite.time.sleep` — is `steady_clock`, because
  NTP can step a wall clock backwards under a running deadline and
  `steady_clock`'s epoch means nothing anyway. *"One clock, one epoch"* is a
  rule about the type, and it holds with nothing competing. The other two
  decisions cannot be read out of the type and are recorded beside it:
  **`satellite.time.sleep(n)` takes seconds** — whole or fractional, so QUAD's
  90 ms tick is `sleep(0.09)`, and `0.09` is an exact `Number` (§8.1), so no
  float and no M15; and **`satellite.time.new` `1 9 2` is designed beside
  `satellite.variable.date` `1 6 7` at M29, the calendar** — an instant
  constructor cannot be designed apart from the date it constructs from, and
  neither can §2.2's zero children under `satellite.variable.time` `1 6 3`,
  which M29 numbers.
- **Error codes: numbered or named?** Numbered is testable and translatable, named
  is readable. Probably both, the way words have an id and a spelling (§4.4).
- **Unknown string escapes** (§5.4) — pass through, reject, or warn. The first
  satellite passed them through and printed a backslash 501 times in one program.
- **What is in `arguments[0]`?** `satellite.main` takes a
  `satellite.container.list<satellite.variable.string>`, and nothing says whether
  its first element is the program name, the current directory, or the first
  argument the user actually typed. The author's first note has it displaying the
  current directory. Left undecided it will be settled by accident at M17, and
  every program written before the accident will disagree with every program
  written after.
- ~~**`satellite.thread.new` against `satellite.variable.thread`.**~~ **Settled
  2026-08-27.** Both exist and they are the established two-part shape — the type
  under `variable`, the constructor under a sibling namespace, exactly as
  `satellite.file.new` and `satellite.time.new` already do. *(Corrected
  2026-09-04: `satellite.time.new` has never existed — `1 9 2` is reserved and is
  designed at M29 beside `date` — so the precedent stands on `satellite.file.new`
  alone, which carries it.)* `satellite.thread` is
  `1 23` and `satellite.thread.new` is `1 23 1`.

  What the settling exposed is bigger than the numbering. The form is

  ```satellite
  satellite.variable.thread my_thread = satellite.thread.new(capsule_name(args))
  ```

  and it parses today with **no change to §6**: `var_decl := type IDENT "="
  expression`, §6.1 dispatches on `path[1] == variable`, and the initialiser is
  §6.2's postfix loop producing a Call over a Call. But `capsule_name(args)` at that
  position is not performed — it is *packaged*. The handler evaluates the arguments
  and stores `(capsule number, argument values)` for the thread to run later.

  **That is a deferred call, and it is the language's own requirement rather than a
  concession to anything.** It is also why §12's deferral of *a bare name can be a
  value* survives: `capsule_name(args)` is a **call form**, not a bare name used as
  a value, so §2 stays shut. Its type is `satellite.variable.capsule` at `1 6 16`.
  `satellite.capsule` `1 2` remains the declaration keyword.

---

*Companions: [PLAN.md](PLAN.md) — architecture, milestones, the build and the
install. [LAYOUT.md](LAYOUT.md) — every file in the tree and what it is for.*
