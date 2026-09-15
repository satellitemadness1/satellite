# M28 — `&&`, `||` and `not`

**Named by** `foreign.cpp` rows for `&&`, `||`, ` and `, ` or `.

## What satellite makes you write instead

Nothing. There is no spelling. `satellite.statement` is if, for, while and else,
and a condition is one expression — so two conditions are two nested `if`s:

```
satellite.statement.if(until_check == 0)
{
    satellite.statement.if(satellite.system.memory.main("mb") > ceiling_mb)
    {
        ...
    }
}
```

and a `while` that wants two conditions cannot nest at all — it needs a `bool`
computed before the loop and recomputed at the end of every pass.

**Measured cost, 2026-09-12:** three probes were written before it was believed.
`a < 5 && b < 5` is `S0201: expected ')' to close the condition, and found
Punct(&)`; `a < 5 and b < 5` is the same error on `Word(and)`. The third probe
invented `satellite.operator.and(...)`, which **passed `--check`** and failed at
`--resolve` with `S0521` — a path that does not exist.

## What it would cost to build

- `lexer.cpp` — `&&` and `||` as punctuation; `and`/`or`/`not` as reserved words
- `parser/parser_expressions.cpp` — two precedence levels below comparison
- `evaluator/compile_expressions.cpp` — **short-circuit**, which is the whole
  design question: `a && b` must not evaluate `b` when `a` is false, so it is a
  jump and not a two-argument operation

## What it must not break

DESIGN §1's naming rule. `and` as a reserved word takes a name programs may be
using; `&&` takes nothing. If the reservation is unacceptable, ship `&&` and
`||` alone and let `not` wait — an asymmetry worth having over a broken program.
