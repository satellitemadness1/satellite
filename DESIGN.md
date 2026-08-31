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

This program is the target of milestone 8 (PLAN.md §8).

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
  unwritten paths as working and break §1.1's *never behind their back*. The walk
  therefore prints a node only when `handlers[path_id]` (§4.5) is non-null: **the
  trie is what exists, the handler table is what works**, and help reads both. PLAN
  M18 is where this is built and carries the argument in full.
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
cannot occur.** Bit shifts, if ever needed, are
`satellite.variable.number.shift_left(n)`, consistent with §1 — under
`satellite.variable.number`, where every other number operation lives, and not
under a top-level `satellite.number`, which does not exist. *(Corrected
2026-08-27; this was the only place in either document that implied one.)*

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

include_decl   := "satellite" "." "include" "(" expression ")"

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

spacesuit_decl := "satellite" "." "spacesuit" IDENT [ "(" IDENT ")" ] suit_block
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

### 7.4 A slot is never reused across scopes

A redeclaration in one scope **rebinds the name to a fresh slot** rather than being
an error. The fresh slot is not an implementation detail: a spacesuit is a reference
type, so reusing the old slot would leave every handle already taken to the first
instance pointing at the second — and a list built by that idiom would read back as
*n* copies of its last element **with no error anywhere.**

### 7.5 Recursion is bounded, and the bound is derived

*(v1: measured, and the first guess was wrong.)* A default depth of ~10000 sat past
both stack cliffs, so the guard could never fire and the segfault it existed to
prevent was exactly what a runaway recursion got.

The default is **2000**. One activation costs 3 units and ~3169 bytes at -O2, and
the ceiling is derived from `RLIMIT_STACK` rather than fixed: 3000 on an ordinary
8 MB stack, ~24,000 on 64 MB. `ulimit -s` is therefore the knob for how deep a
program may recurse, and it is **outside the language on purpose.**

The numbers live beside the code that uses them, never only here.

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

#### The name is any of six spellings

`arg`, `args`, `argz`, `argument`, `arguments` and `argumentz` all become the
special variable inside the program that declares one. The author writes
`arguments`; the others exist because people type what they type, and a language
whose tie-breaker is *do absolutely everything for the user* does not make somebody
lose an afternoon to a plural.

This is the **inverse of §4.4's interner** and needs saying plainly, because §4.4
describes the opposite arrangement. There, two nodes share one spelling — `list`
under `container` and `list` under `directory` are different nodes that happen to
be spelled alike. Here, **one node answers to six spellings.** Deduplication is
many-nodes-one-string; aliasing is one-node-many-strings, and the table has to hold
both directions.

It is also the one place a **bare** identifier is language-owned, which §1's
generating rule otherwise forbids. The rule survives because the user still writes
the name: the language does not introduce `arguments`, it *recognises* the name the
user chose for `satellite.main`'s parameter when that name is one of the six.

#### What it holds

Nested, not flat — `memory` answers on its own *and* has children:

```
arguments.username              the login name
arguments.memory                free memory
arguments.memory.total          total memory
arguments.machine               what the processor is
arguments.machine.cpu           the same, asked for directly
arguments.machine.cores         physical cores, a satellite number
arguments.machine.threads       hardware threads, a satellite number
```

A node that is both a value and a parent is the general case here rather than a
special one, and it is the same shape `satellite.container` already has — a bare
form and a set of children, which is what the `(0)` in WORD_NUMBERS.md marks.

#### Three surfaces, one set of facts

`arguments.machine.threads`, `arguments.machine.cores` and `arguments.memory.total`
are **the same numbers** as `THREAD_COUNT`, `CORE_COUNT` and `MEMORY_MAX` in the
configuration, and the same numbers again as codes 97, 98 and 99 in
`satellite_string`'s live code table (PLAN §6). Three ways to ask, one place that
knows — `system_facts`.

They must not be allowed to disagree. Whatever the configuration finally says, it
is a *setting* and the machine is a *fact*, and a program that asks
`arguments.machine.threads` is asking what the machine has, not what the config was
told. If the two ever need to differ, they need two different names.

**And the configuration writes these three names, which it could not until
2026-08-31.** `CORE_COUNT=arguments.machine.cores` is the machine's own answer, in
the spelling above and in any of the six; `CORE_COUNT=12` is twelve. It is the bare
form and not the rooted path, for the reason this section already gives — a program
writes the name it gave `satellite.main`'s parameter, never
`satellite.library.main.arguments`, and a configuration that demanded the rooted
spelling would be asking for one the language does not use. PLAN §4.5.4 has what
*not* having it cost: satl read all three facts on every run, because the file had
no way to say "the machine" and something had to fill the values in before it was
opened.

#### Open

- **Which spellings, exactly, and what happens to the seventh?** Declaring a
  parameter named `argv` gets a plain list with no properties, silently. Under §9
  that silence is wrong — the language should say so.
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
| `satellite.variable.time` | absolute instant, UTC | §13, open: one clock, one epoch |
| `satellite.variable.file` | handle | reference type |
| `satellite.variable.thread` | handle | reference type |
| `satellite.variable.capsule` | `(capsule number, argument values)` | a deferred call; §13, and `1 6 16` |
| `satellite.variable.variant` | — | deferred; PLAN.md §8, "Later" |
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

### 8.3 Strings

`SatString` over a 16-bit code table, which *is* the language's alphabet (§5). A
string interner is separate and serves §4.4.

### 8.4 Maps

Insertion-ordered. Key types are restricted, and the restriction is not a
preference about extensibility — it is §6.5's deadlock one level down.

### 8.5 Binary and hexadecimal

`x00FF` and `b1010` are real types with literals, and **the width is part of the
value**: `x0009` is not `x9`. That is the whole reason they are not number literals
in another base. `hexadecimal` is the language's one alias, for `hex`.

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
`+` `−` negate `abs` `trunc` `floor` `ceil` `%` comparison `min` `max` `is_integer`
(which is just `R == 0`). These are safe in a loop forever. Addition and subtraction
being here is the property `double` does not have.

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

#### Still open

**The rounding rule** — truncate, half-up, or half-even. It cannot be avoided by any
choice of representation: `rack.hpp:59` computes `pow(urgency, exp)` with `exp` always
fractional, and `x^y` at fractional `y` is irrational, so **no pair of exact numbers
represents it** and `R` must be rounded to exist. QUAD's determinism invariant means a
program's behaviour depends on which rule is chosen. PLAN §8 puts the float in **M11**,
which cannot land until it is.

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

Producer threads push whole strings into a locked queue; one printer thread
consumes. **A line stays atomic because the unit queued is a whole string.** There
is a `drain()` barrier before reading input, so a prompt cannot appear before the
output that explains it.

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

Two shapes on each:

```satellite
satellite.variable.number n = satellite.random.ultra(40)
satellite.variable.number m = satellite.random.ultra.range(1, 100)
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

- **User-defined generics** — a bare name can be a value, which reopens §2.
  *(Still deferred, 2026-08-27.* The deferred-call form
  `satellite.thread.new(f(x))` does **not** reopen this: `f(x)` is a call, which
  §6.2's postfix loop already produces, and the only new semantics is that one
  handler packages its argument instead of performing the call.)
- **Durations** — `time` is an absolute instant only; subtraction yields a number of
  nanoseconds.
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
  blocks and a bare-name type. Reference semantics.
- **A unit of includable code is a spaceship.**
- **Operator precedence** is §6.6's four levels, all left-associative, with unary
  `-` and `!` above them. *(Decided at M4, 2026-08-30, because the parser could not
  be written without it — §6's expression rule read `... precedence climbing ...`
  and named no operators.)*

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

  **What is still open is the rounding rule, and it is not a detail.** Truncate,
  round-half-up, or round-half-even — and whichever it is, it must be stated, because
  the answers genuinely differ and QUAD's determinism invariant means a program's
  behaviour depends on it. It cannot be avoided by any choice of representation:
  `rack.hpp:59` computes `pow(urgency, exp)` with `exp` always fractional, and `x^y`
  at fractional `y` is irrational — **no pair of exact numbers represents it.** The
  right half has to be *rounded to exist*, which is why the rule is part of the type
  rather than a setting on it.

  **§8.6 is the specification** — the invariants, `normalize`, and the four
  operations, of which addition and subtraction turn out to be **exact**. **M11 owns
  this** (PLAN §8) and cannot land until the rounding rule is chosen.

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
- **Time.** `satellite.variable.time`, `.date` and `satellite.time.now()` must agree
  on **one clock and one epoch** before any of them is built. The author's stated
  leaning is a high-precision clock, which settles the resolution question but not
  the epoch — and not the harder one underneath it, that a high-resolution
  monotonic clock and a wall-clock date are not the same clock and cannot both be
  the one.
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
  `satellite.file.new` and `satellite.time.new` already do. `satellite.thread` is
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
