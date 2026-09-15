# M30 — `switch` and `match`

**Named by** `foreign.cpp` rows for `switch` (C, C++, Java) and `match ` (Rust).
Python's `match` (3.10+) belongs here too.

## What satellite makes you write instead

A chain of `if`s, each testing the same value. From `infinity_data_main.satl`:

```
satellite.statement.if(which == 0) { satellite.return("Khorne") }
satellite.statement.if(which == 1) { satellite.return("Tzeentch") }
satellite.statement.if(which == 2) { satellite.return("Nurgle") }
satellite.return("Slaanesh")
```

That is four comparisons where a `switch` is one jump, and — more importantly —
nothing checks that the cases are exhaustive or that none is duplicated. The
last line is a default nobody declared as one.

## What it would cost to build

- `words.def` — `satellite.statement.switch` `1 13 7`, and a `case` shape under it
- parser — a block of cases, each a literal and a body
- evaluator — a jump table when the cases are dense integers, a chain otherwise

**The design question is fallthrough**, and satellite should not have it: C's
default is the one every other language spent thirty years apologising for.

## What it must not break

Rust's `match` is exhaustive over a type and binds parts of the value; that is
pattern matching, not `switch`, and it needs M35 (structured bindings) and a
notion of a sum type. **Ship `switch` first and do not call it `match`** — a word
means one thing everywhere (DESIGN §1), and borrowing Rust's word for C's
feature would break that on arrival.
