# M36 — ranges and pipelines

**Named by** `std::ranges`, `\|` views (C++20), `.iter().map().collect()` (Rust),
`.stream()` (Java), list comprehensions (Python).

## What satellite makes you write instead

An index loop and a second list:

```
satellite.container.list<satellite.variable.number> out = satellite.container.list()
satellite.variable.number i = 0
satellite.statement.for (i = 0; i < in.size(); i = i + 1)
{
    satellite.variable.number one = in[i]
    out.append(one * 2)
}
```

## What this depends on

**M32 first.** Every pipeline takes a function as an argument, and a capsule is
not a value yet — so `map` has nothing to be given. `list.sort_up(key)` is the
shape the language already wants and the reason M32 matters more than this does.

**M29 second.** A pipeline is chained calls by definition, and one hop refuses.

So this milestone is mostly *those two plus a handful of list methods*, and
should not be scheduled before them.

## What it must not break

Laziness is the part to leave out. C++'s views are lazy and that is where their
difficulty lives; a satellite `map` that builds a new list eagerly is honest
about its cost and needs no lifetime rules to go with it.
