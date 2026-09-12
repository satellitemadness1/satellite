# M35 — taking a value apart in one line

**Named by** `auto [a, b] =` (C++17), `let (a, b) =` (Rust), `a, b = ` (Python),
and every `for k, v in dict.items()`.

## What satellite makes you write instead

One name per line, and an accessor for each part:

```
satellite.variable.number first = pair.call_first()
satellite.variable.number second = pair.call_second()
```

A capsule answers exactly one value (`satellite.returns(type)` is singular), so
returning two things means a spacesuit with two `call_` verbs — which is three
declarations to hand back a pair.

## What it would cost to build

Two halves that can ship separately:

1. **destructuring a container** — `satellite.variable.number a, b = list` over
   a list of known length; parser and resolver only
2. **a capsule answering more than one value**, which is the deeper one and
   needs a tuple type the numbering does not have yet

## What it must not break

DESIGN §1's "a word means one thing everywhere". `[a, b]` borrowed from C++
would collide with subscripting, which satellite already spells `l[i]`.
