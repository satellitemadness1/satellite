*satellite design docs, §1 of 19. Index: [DESIGN.md](../DESIGN.md).*
*On: [§2](02-hello-world.md).*

---

## 1. The generating rule

One invariant produces all satellite naming:

> **A dotted path rooted at `satellite` names something the language owns.
> A bare identifier names something the user owns.**

Every other naming rule follows from it:

- Types are language-owned, so they are prefixed: `satellite.variable.time`.
- Variable names are user-chosen, so they are bare: `my_time`.
- Method names are selectors interpreted relative to what precedes them, so they are
  bare: `my_time.some_function()`.
- `satellite.main` is prefixed because the *runtime* chooses and calls that name, not
  the user. A capsule the user writes is bare: `fact(3)`.

The earlier phrasing — "everything the language provides has `satellite` in front of it" —
is not quite the real rule, because `some_function` and `now` are language-provided and
bare. The invariant above is the version that holds everywhere.

`satellite` is the **only reserved word in the language**. Nothing else needs reserving:
`variable`, `library`, `time`, `file`, `list` are special only in the second position of a
satellite-rooted path, so a user may freely name a variable `time`. Declaring anything
named `satellite` is a hard error, checked at the binding site.
