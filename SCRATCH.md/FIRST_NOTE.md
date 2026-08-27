# Converting `plans/madness/first_note.txt`

**This file is scratch**, and it is a ledger for one job: the user asked that their
first note be converted into the permanent documents and the original deleted.
Delete this file once the "not yet captured" column below is empty.

The original is preserved in git — it landed in `eef1fbb`, so `git show
eef1fbb:plans/madness/first_note.txt` returns it whole, forever. That is what makes
deleting it safe rather than lossy. Its text is also reproduced verbatim at the
bottom of this file, so the conversion can be checked without leaving the tree.

---

## What the note says, and where it now lives

| the note says | already captured? |
|---|---|
| interpreted, no compile step | **yes** — PLAN §2.3, *"no compile step the user ever runs or waits for"* |
| tree-walking AST, parse then execute | **yes** — PLAN §2.1's table, row 1 |
| limited error handling in v1, wants that changed | **yes**, and it became structural: PLAN M5 builds the error reporter *before* the evaluator, and DESIGN §9 makes errors part of the interface |
| keep v1's working state and its speed, improve where we can | **yes** — PLAN §6, "What we keep from the first satellite" |
| `satellite.return()` optional, `satellite.main` must have `satellite.return(satellite)` | **yes** — DESIGN §3, in the user's own phrasing |
| the eventual type list (`number`, `string`, `float`, `bool`, `list`, `map`, `variant`, `file`, `time`, `date`) | **mostly** — DESIGN §8 and §12, PLAN §8's "Later" list |
| the three random tiers with digit counts | **yes** — DESIGN §11 |

## What the note says that is NOT captured anywhere

These are the reason the conversion is not simply a deletion.

### 1. `satellite.thread.new(...)` — a top-level `thread` namespace

The note's example is:

    satellite.variable.thread my_thread = satellite.thread.new(my_capsule_name(some_str))

That is **two different things sharing a word**: `satellite.variable.thread` is the
type, and `satellite.thread.new` is the constructor that makes one — under a
top-level `satellite.thread`, not under `variable`.

Everywhere else in the current documents only `satellite.variable.thread` appears
(DESIGN §10.4, PLAN M12). **`satellite.thread` is not in DESIGN, not in PLAN, and
not in WORD_NUMBERS.md.** It needs a number, and the pattern it sets — a type under
`variable` with its constructor under a sibling namespace — is worth checking
against `satellite.file.new` (which is already numbered `1 8 1`) and
`satellite.time.new` (`1 9 2`). Those two follow exactly the same shape, so the
note is consistent with the numbering and the numbering is missing a word.

### 2. The user's clock preference

> *"though everything about time is not yet worked out; I usually go with the
> high_precision_clock"*

DESIGN §13 lists Time as open and says `satellite.variable.time`, `.date` and
`satellite.time.now()` *"must agree on one clock and one epoch before any of them
is built."* It does **not** record that the user has a preference. That belongs in
§13's Time bullet — it is half an answer to a question the document is holding
open, and losing it means asking them again.

### 3. No JIT, and none planned

The note says so outright. PLAN §2.1 rules out *bytecode* and lists what was
considered, but a JIT is never named — the nearest is copy-and-patch, dismissed as
*"out of scope — it is a compiler."* Since a reader's first question about a
tree-walking interpreter is "why not JIT it", the note's answer deserves to be in
§2.1's table or §7 rather than in a deleted file.

### 4. `arguments[0]` is the current directory

The note's example comments `satellite.console.display(arguments[0]) // displays the
current directory...`. What `satellite.main`'s `arguments` list actually contains at
index 0 — program name, current directory, or first user argument — is not stated in
DESIGN anywhere. It is a small thing that will be decided by accident when M8 is
written unless it is written down first.

---

## The original, verbatim

Kept so that the conversion above can be audited without `git show`, and so that
nothing in it is lost to a paraphrase.

```text
satellite is an interpreted programming language, so there is no compile step.

It doesn't currently have a Just-In-Time compiler, and one is not currently planned

It is a tree-walking ast eval -> parse > execute

There is only limited error handling in the first satellite, I want to change that

an example syntax:

satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments[0]) // displays the current directory...

    satellite.variable.string some_str = "Hello, World!"

    satellite.variable.thread my_thread = satellite.thread.new(my_capsule_name(some_str))

    satellite.return(satellite)
}

satellite.capsule my_capsule_name(satellite.variable.string str_input)
{
    satellite.console.display(str_input)

    satellite.return()
}

// satellite.return() is not required, only satellite.main *must* have satellite.return(satellite)

I want to keep a lot from the first version of satellite, because it worked, and it was also fast...

So I want to keep it's working state, and the speed of it; But I also want to make improvements
where we can as we pull from the old source code.

satellite will eventually support:

satellite.variable.number
satellite.variable.string
satellite.variable.float (an infinitely long (in both directions,) floating point number
satellite.variable.bool a "TRUE" or "FALSE" variable
satellite.container.list (a list of objects)
satellite.container.map (variable amounts of objects)
satellite.variable.variant (C++'s std::variant)

satellite.variable.file
satellite.variable.time
satellite.variable.date
satellite.random.fast(99) // 99 fast digits
satellite.random.normal(99) // 99 random digits normally
satellite.random.ultra(99) // an ultra secure random number;

satellite.time.now() // though everything about time is not yet worked out; I usually go with the high_precision_clock
```
