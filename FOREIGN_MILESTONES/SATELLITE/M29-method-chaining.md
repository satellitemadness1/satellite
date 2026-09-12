# M29 — more than one hop

**Named by** the universal shape `a.b().c()`, which every language in the table
spells the same way.

## What satellite makes you write instead

One hop from a declared name. A method on the *result* of a method parses and
refuses at run time:

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

**This one is the most dangerous of the three** because `--check` *and*
`--resolve` both pass it. It fails only when the line runs, which on a program
with a seven-minute build phase means seven minutes per attempt.

`+` concatenating a bare number (`"x " + n.round()`) is the workaround that
makes most display lines survivable, and it is a coincidence rather than a plan.

## What it would cost to build

WORD_NUMBERS §1.5's "one hop" is a *resolver* limit, not an evaluator one: the
fold turns `name.method` into a numbered path at resolve time, and an expression
has no name to fold through. The work is in `name_resolver/walk.cpp` — resolve
the receiver's TYPE rather than its name, then fold the second hop against that
type's method table.

## What it must not break

The numbering. A folded selector is a `PathId`, and `1 6 4 9` means `round` on
any number — so the second hop resolves against the first's answered type, which
the resolver already knows. Nothing new is numbered.
