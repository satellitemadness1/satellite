# SATELLITE

An interpreted language with a space theme. One reserved word: `satellite`.

```
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments[0])
    satellite.return(satellite)
}
```

## Status

`satellite_container` (the universal value type) and the lexer are built and
tested. No parser or VM yet.

```
cmake -S . -B build && cmake --build build -j8
./build/test_container      # 73 checks
./build/test_lexer          # 37 checks
./build/bench_dispatch
```

## satellite_container

A 16-byte tagged union. Four fit in a cache line; passing one costs two
register moves.

| type | storage |
|---|---|
| `Nil` `Bool` `Int` `Real` | inline, never allocates |
| `Big` | heap limbs, reached only on int64 overflow |
| `Str` | refcounted, 32-byte inline buffer before the heap |
| `List` | refcounted vector of containers |

Heap payloads are refcounted and destroyed through the type tag — there is no
virtual dispatch anywhere in the value model.

`Int` promotes to `Big` only when an operation actually overflows
(`__builtin_add_overflow`), and demotes back to `Int` as soon as the value fits
again. Ordinary arithmetic never touches the allocator.

### Dispatch: measured, not assumed

The original plan was a pure dispatch table indexed by both type tags. Measured
on a Xeon E5-2670 v3, that turned out to be **slower** than a plain if-chain at
satellite's current type count:

```
PURE INT     if-chain 3.85 ns/op    table 4.23 ns/op   table 1.10x slower
MIXED TYPES  if-chain 10.57 ns/op   table 13.72 ns/op  table 1.30x slower
```

An indirect call through a function pointer is itself an unpredictable branch,
and it blocks inlining. With 7 types, an if-chain fits in the branch predictor
and wins.

So dispatch is now **hybrid**: hot type pairs (`Int+Int`, `Real+Real`) are
inline branches, and the table catches the long tail. Hot code skips the
indirection; adding a new type still means filling in a table row rather than
growing a chain that slows down every operation.

```
PURE INT     hybrid 3.88 ns/op    1.09x faster than the if-chain
MIXED TYPES  hybrid 13.23 ns/op   1.25x slower  (Int+Real still goes to the table)
```

Mixed-type dispatch is the open performance item — more pairs could be pulled
inline, at the cost of a fatter hot function. Worth revisiting once real
satellite programs show which pairs actually occur.

## Decisions made (change freely — both are cheap now, expensive later)

**`"my_string" - "str"` removes the FIRST occurrence** → `"my_ing"`.
Alternatives are all-occurrences or trailing-only. In `sub_str()` in
`src/container.cpp`.

**A character is 32 bits, holding a satellite charmap code.** Not a Unicode
codepoint — an index into satellite's own table:

| code | characters |
|---|---|
| 0 | void / error |
| 1–26 | `a`–`z` |
| 27–52 | `A`–`Z` |
| 53–62 | `0`–`9` |
| 63–72 | `!@#$%^&*()` |
| 73–76 | `-_=+` |
| 77–82 | `[{]}\|` |
| 83–87 | `;:'",` |
| 88–92 | `<.>/?` |
| 93–94 | `` ` `` `~` |
| 95–97 | space, newline, tab |

One character is one code, so `remove(0)` removes the first *character* and
`at(i)` is O(1) with no scanning. Text outside the map converts to code 0 and
renders as `?`.

The cost is 4 bytes per character. The reason to pay it: an 8-bit char closes
the door at 256 possibilities, and we don't yet know what a satellite character
will eventually need to be. Leaving that open is worth more than the memory, and
the speed difference is not observable — the operations are `memcpy` and
`memcmp` either way, just over wider elements.

**Codes 95–97 (space, newline, tab) are an addition, not in the original
table** — `"hello world"` cannot be represented without a space. Move them if
you want them elsewhere; they are defined in one place, `charmap::kTable` in
`src/container.cpp`, and the reverse lookup is built from it automatically.

**`+` with a string on either side produces a string**, so
`satellite.console.display("n: " + 5)` works.

**Unsupported operand pairs return `nil`** rather than crashing. Real error
reporting comes with the interpreter.

## Threading: unresolved, deliberately

The design calls for ~200 supervisor/worker threads pre-positioned across the
user's code. Benchmarks on this machine (12 physical cores, 24 hardware
threads, 30MB shared L3) did not reproduce a gain:

| model | median call latency |
|---|---|
| 12 workers, cold | 10.41 μs |
| 12 workers + prefetch ahead | **6.08 μs** |
| 200 dedicated workers, kept warm | 11011 μs |

Measured over 3,600 calls, the 200-worker configuration showed no improvement
over time (1% slower first round to last). The ~10ms floor tracks the Linux
scheduler timeslice: with 200 threads on 24 hardware threads, any given worker
is descheduled ~88% of the time, so a call waits for the scheduler rather than
finding a ready worker.

But those numbers are all **fine-grained dispatch**, and that turned out to be
the wrong question to ask alone. Scaling the work per call changes the answer
completely:

```
work per call   12thr    24thr    48thr   200thr
1 us              9.8     10.7     13.2     14.7      <- interpreter territory
100 us           42.4     23.3     23.6     22.6
1 ms            101.7     56.2     56.1     55.1
100 ms          522.2    313.9    309.9    293.8

I/O-bound, 500us storage wait
                 92.5 ms          ->        5.9 ms    <- 15.7x faster
```

So there is no single correct worker count. There are three regimes:

- **Fine-grained CPU work** (interpreter instructions, ~ns each): few workers.
  200 loses to 12 by 1.5x — per-dispatch overhead dominates.
- **Coarse CPU work** (≥100us per call): the gain arrives at hardware-thread
  count and then goes flat. 24 / 48 / 200 land within noise of each other.
- **I/O-bound work** (streaming a model too big for RAM): massive
  oversubscription wins by **15.7x**. Blocked threads do not consume cores, so
  hundreds of them are correct here.

That last regime is the one an AI workload actually lives in, and it is the
strongest case for the original 200-worker design.

Corrections to earlier claims in this file's history: "physical cores beat
hardware threads" was drawn from a latency test and does not hold for
throughput — 12 to 24 threads is a real 1.8x win. And speculative prefetch does
work: 6.08us vs 10.41us cold, on 12 threads. Readiness is real; it comes from
prefetch and precomputed state as well as from thread count.

None of this is settled, because real satellite programs don't exist yet and
synthetic capsules may not predict them. So the worker count is a runtime
parameter — `satellite.workers = N` — and nothing in the container, the call
convention, or the bytecode may assume a thread count. Capsules do not own
threads; tasks are assigned to workers.

Better still, the count should follow the workload class, which the language
already has a syntax for. `slow/medium/fast` on `satellite.thread.new` and
`satellite.thread.listen` can carry an I/O-vs-CPU hint, so a capsule streaming
model weights gets hundreds of workers while one running tight interpreter
loops gets a couple of dozen. That makes the priority classes load-bearing
rather than decorative.

Benchmarks live in `bench/`.

## Lexer

Two rules give the language its shape, and both are tested:

**`satellite` is the only reserved word — and only when not preceded by a
dot.** After a dot it is an ordinary identifier, which is what makes the escape
hatch work:

```
satellite.library.my_capsule.satellite
^keyword                     ^just a name
```

Every other word belongs to the user: `capsule`, `class`, `return`, `if`,
`while` all lex as plain identifiers.

**A newline ends a statement only when the previous token could finish one.**
Go's rule. A line ending in `+` or `(` continues. Newlines inside `( )` and
`[ ]` never terminate, so argument lists span lines freely; braces deliberately
do not suppress termination, since statements inside a capsule body do end.

## Next

1. Parser — capsules, spacesuits, `satellite.library` paths
2. Single-pass compiler to bytecode, resolving `satellite.library.x.y` to slot
   indices at compile time
3. Register-based VM with computed goto
4. Ropes for large-string concatenation, so `a + b` is O(1) above a threshold
5. Scheduler, with the worker count as a knob
