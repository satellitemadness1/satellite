# M33 — catching a refusal

**Named by** `foreign.cpp` rows for `try {` (C++), `catch (` (Java), `throw `
(C++/Java); also Rust's `Result` and `?`, and C++23's `std::expected`.

## What satellite makes you write instead

Nothing. A refusal stops the run. There is no `try`, and a program cannot
recover from a file that would not open, a division that does not end, or any
other `S0***` — it can only avoid causing one.

Some of the language is already shaped around this and shaped WELL:

```
satellite.variable.file f = satellite.file.open(path, "read")
satellite.statement.if(f.ok() == satellite.bool.false) { ... }
```

`ok()` and `error()` are a refusal turned into a value on purpose — the file
module answers instead of stopping. **That is the pattern this milestone should
generalise**, not exceptions.

## What it would cost to build

DESIGN §9.1 is explicit that a handler returns a bool and reports through the
machine, and that throwing was measured at **181x the cost of returning an
enum**. So this must not become C++ exceptions internally whatever it looks like
in a program.

The cheap and honest version is C++23's `std::expected` rather than `try`:

- a variant that holds either the answer or the refusal
- a spelling that says "give me the value or the problem" instead of stopping
- `satellite.variable.variant` `1 6 13` already holds "either", and `.holding()`
  already asks which

## What it must not break

The rule that a program cannot silently continue past something it got wrong.
Whatever this becomes, **ignoring the error half must be harder than handling
it** — a `try {} catch {}` swallowing everything is the failure mode, and the
`ok()` pattern above avoids it by making the check the only way to the value.
