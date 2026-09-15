# M34 — `constexpr`

**Named by** `constexpr`, `consteval`, `if constexpr` (C++), `const fn` (Rust),
`final` on a constant (Java).

## What satellite makes you write instead

`satellite.library.<name> = <literal>` is resolved at parse time — `--resolve`
prints "at parse time" beside every one — so the language already has a notion
of a value known before the run. What it does not have is a **computation**
known before the run: `satellite.library.span * satellite.library.span` is not
a library value, and there is no way to fold it into one.

So `infinity_data_main.satl` writes the derived constants out by hand and says
so in a comment, or recomputes them at run time on every pass.

## What it would cost to build

Fold literal arithmetic over `satellite.library` values at resolve time. The
resolver already walks these; what it lacks is an evaluator it may call. The
honest scope is **arithmetic on numbers and strings only** — a general
compile-time interpreter is a second machine, which is what C++ learned the
expensive way between `constexpr` and `consteval`.

## What it must not break

`--resolve`'s claim that a library value comes "from its declaration". A folded
expression has no single declaration to point at, so the provenance line must
say *computed*, or the output starts lying about where numbers came from.
