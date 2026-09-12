# M29 — more than one hop

**Named by** the universal shape `a.b().c()`, which every language in the table
spells the same way.

**Status changed 2026-09-12, the same day this was written.** The first draft
called this the most dangerous of the three because it escaped every static
check and failed only at run time. That is **no longer true** — S0720 is a
check-time diagnostic now. The milestone is still open, because being told
earlier is not the same as being able to write it, but its cost has dropped by
an order of magnitude and [../README.md](../README.md)'s ranking reflects the
old state rather than this one.

## What satellite makes you write instead

One hop from a declared name. A method on the *result* of a method is refused:

```
machine_gb.round()              // fine
machine_gb.round().to_string()  // S0720
```

> `a method on this expression parses and does not run yet -- a selector folds
> only through a declared name -- WORD_NUMBERS.md §1.5's one hop -- so name the
> receiver first`

So every intermediate gets a name:

```
satellite.variable.number subjects = state.call_store_subjects()
out.write_line(subjects.to_string())
```

`+` concatenating a bare number (`"x " + n.round()`) is what makes most display
lines survivable without this, and it is a coincidence rather than a plan.

## What changed, and why it mattered so much

**Before**: `--check` and `--resolve` both passed a chained call, and it died
when the line ran. On `infinity_data_main.satl` — whose build phase runs seven
minutes before it reaches the display lines — that was **seven minutes per
attempt to find a fault that was always statically visible**. Three attempts,
twenty-one minutes, for a mistake the compiler could see the whole time.

**After**: the compiler records the refusal into `eval::Program::deferred` at
compile time, and `--check` runs all four passes and prints it. Measured
2026-09-12 against that exact line:

```
$ satl --check chain.satl
satl: chain.satl:5:73: error S0720: a method on this expression parses and does
      not run yet -- a selector folds only through a declared name
   5 |     satellite.console.display("machine: " + machine_gb.round().to_string())
     |                                                                         ^
exit=1
```

**`Program::ok()` deliberately ignores `deferred`**, so a RUN is bit-for-bit
unchanged and errors.def's S0720 note — that a `satellite.include` in a branch
which never runs is a program that runs — stands exactly as written. The
diagnostic moved; the language did not.

*(It is S0720 and not S0723 for this shape — verified by running it after the
change landed, because the two are easy to confuse and a milestone document
citing the wrong code sends its reader to the wrong row.)*

## What this leaves the milestone worth

**Less than it was, and still real.** The remaining cost is no longer discovery,
it is the writing: every intermediate needs a name, so a line that reads as one
thought in five other languages is three declarations here. In a spacesuit-heavy
program that is most display lines and every `write_line`.

So M29 has moved from *"the trap that eats afternoons"* to *"a verbosity tax the
checker now points at"*. Schedule it accordingly — after
[M27](M27-break-and-continue.md) and [M28](M28-logical-operators.md), which are
still silent until you hit them.

## What it would cost to build

WORD_NUMBERS §1.5's "one hop" is a **resolver** limit and not an evaluator one:
the fold turns `name.method` into a numbered path at resolve time, and an
expression has no name to fold through. The work is in `name_resolver/walk.cpp`
— resolve the receiver's TYPE rather than its name, then fold the second hop
against that type's method table.

## What it must not break

The numbering. A folded selector is a `PathId`, and `1 6 4 9` means `round` on
any number — so the second hop resolves against the first's answered type, which
the resolver already knows. Nothing new is numbered.

**And it must not undo what just shipped.** The check-time diagnostic is the
thing that made this survivable; if chaining lands, the S0720 row is not deleted,
it is narrowed to whatever still cannot fold.
