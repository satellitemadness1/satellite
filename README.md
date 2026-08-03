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

Just started. `satellite_container` — the universal value type — is built and
tested. Nothing else exists yet.

```
cmake -S . -B build && cmake --build build -j8
./build/test_container      # 42 checks
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

**String indices are bytes, storage is UTF-8.** `remove(0)` removes the first
*byte*. A separate codepoint API can sit alongside it. Deciding this later is
the single most painful migration in language design, so it is decided now and
written down.

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

Two findings worth keeping either way:

- **Looking ahead works.** Speculative prefetch of the next capsule beat cold
  dispatch 6.08μs vs 10.41μs on 12 threads. Readiness is real; it just comes
  from prefetch and precomputed state rather than from thread count.
- **Physical cores beat hardware threads.** 12 workers (22.83μs) beat 24
  workers (28.08μs) — hyperthread pairs contend for L1 and execution ports. The
  default should be physical core count, not `hardware_concurrency()`.

None of this is settled, because real satellite programs don't exist yet and
synthetic capsules may not predict them. So the worker count will be a runtime
parameter — `satellite.workers = N`, defaulting to physical cores — and nothing
in the container, the call convention, or the bytecode may assume a thread
count. Capsules do not own threads; tasks are assigned to workers. Set it to
200 when there is real code to measure, and the question answers itself.

Benchmarks live in `bench/`.

## Next

1. `satellite_string` proper — rope representation above a size threshold, so
   `a + b` on large strings is O(1)
2. Lexer — Go-style newline-as-terminator; `satellite` is an identifier after
   `.` and a keyword otherwise
3. Single-pass compiler to bytecode, resolving `satellite.library.x.y` to slot
   indices at compile time
4. Register-based VM with computed goto
5. Scheduler, with the worker count as a knob
