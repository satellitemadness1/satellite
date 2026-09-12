# Python 3.12 → satellite, feature by feature

**The standard this folder is written against:** **Python 3.12** (October 2023),
with 3.13 features marked where they intrude. Python has no ISO standard; the
reference is the CPython language reference for that version.

**Written 2026-09-12 against knowledge current to May 2026.**

Python is the language most new satellite readers arrive from, and the one whose
habits break hardest — because the two disagree about *declaration*. Python
declares nothing; satellite declares everything, including the type.

## The catalogue

| # | Python | satellite | answer |
|---|---|---|---|
| 1 | `print(x)` | `satellite.console.display(x)` | says it |
| 2 | `def f():` | `satellite.capsule f()` | says it |
| 3 | `class C:` | `satellite.spacesuit C()` | says it |
| 4 | `import x` | `satellite.include(satellite)` | says it |
| 5 | `if __name__ == "__main__"` | `satellite.capsule satellite.main(...)` | says it |
| 6 | `x = 5` (no declaration) | `satellite.variable.number x = 5` | says it |
| 7 | `True` / `False` / `None` | `satellite.bool.true` / `.false` / a variant holding nothing | says it |
| 8 | `elif` | nest an `if` inside the `else` | says it |
| 9 | `for x in xs:` | `satellite.statement.for (i = 0; i < xs.size(); i = i + 1)` | says it |
| 10 | `and` / `or` / `not` | — | [M28](../SATELLITE/M28-logical-operators.md) |
| 11 | `break` / `continue` | — | [M27](../SATELLITE/M27-break-and-continue.md) |
| 12 | `lambda` | — | [M32](../SATELLITE/M32-capsules-as-values.md) |
| 13 | `try` / `except` | — | [M33](../SATELLITE/M33-catching-a-refusal.md) |
| 14 | `match` (3.10+) | — | [M30](../SATELLITE/M30-switch-and-match.md) |
| 15 | list comprehensions | — | M32, and a decision about syntax |
| 16 | `self` | — | never: satellite has no `this`; a handle is passed in |
| 17 | decorators | — | M32 |
| 18 | generators / `yield` | — | never planned |
| 19 | f-strings | — | never: build the line with `+` |
| 20 | duck typing | — | never: every declaration names its type |

**The one to read first is 16.** There is no `self` and no `this` anywhere in
satellite — a member capsule cannot name the object it belongs to. An object
that must hand out a reference to itself is *told its own handle from outside*,
once, by whoever made it. `infinity_data_main.satl` does exactly this and the
comment there explains the price.
