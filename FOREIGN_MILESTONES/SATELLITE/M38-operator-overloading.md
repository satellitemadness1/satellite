# M38 — operators on a spacesuit

**Named by** `operator+`, `operator<<`, `operator<=>` (C++20), `impl Add`
(Rust), `__add__` / `__eq__` (Python).

## What satellite makes you write instead

A named verb: `a.call_plus(b)`. Which is arguably *better* and is certainly
clearer — but it means a spacesuit can never be compared or added the way a
number is, so no user type can be sorted by the list's own `sort_up`.

## What it would cost to build

The evaluator dispatches `+` on the value's type today. Extending that to look
for a named capsule on a spacesuit is a lookup, not a new mechanism.

**The design question is which operators**, and the answer should be short:
`+`, `==`, `<`. Everything else — `<<`, `[]`, `()`, conversion operators — is
where C++ overloading became unreadable, and none of it is needed to make a
user type sortable and comparable.

## What it must not break

DESIGN §1 again. `+` already means "add numbers" and "join strings", and a
third meaning defined per type is exactly the ambiguity the naming rule exists
to prevent — so a spacesuit's `+` must be **declared** and not inferred, and a
type without one must refuse rather than guess.
