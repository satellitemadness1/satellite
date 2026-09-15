# M27 — `break` and `continue`

**Named by** `foreign.cpp` rows for `break` and `continue` (C, C++, Java, Rust,
Python — all five spell these the same).

## What satellite makes you write instead

There is no way out of a loop but its own condition, so a loop that stops early
has to make the condition false, and the counter it jumps becomes unusable as
the answer. From `infinity_data_main.satl`, 2026-09-12, at the call site:

```
// The language has no `break` -- `satellite.statement` is if, for, while and
// else and that is the whole list -- so ending a loop early means making its
// condition false, and here that is jumping the counter to the end.
// `people_built` above is why that is safe: the count that gets reported is
// kept separately and is not this one.
satellite.statement.if(satellite.system.memory.main("mb") > ceiling_mb)
{
    shard.call_set_room_ran_out()
    made = count            // <- this is the break
}
```

**The cost is a second variable and a comment explaining it**, in every loop
that can stop early. The program above needed it three times.

`continue` is worse: the body after it has to move inside an `if`, which indents
the rest of the loop for a reason the reader has to reconstruct.

## What it would cost to build

The evaluator compiles onto an explicit control stack (`SCRATCH.md/NO_LIMITS.md`,
M9), so both are a jump to a known offset rather than a new machinery:

- `words.def` — two rows under `satellite.statement`, `1 13 5` and `1 13 6`
- `parser/parser_control_flow.cpp` — accept them as statements
- `evaluator/compile_statements.cpp` — emit a jump to the loop's end or its step
- the refusal for one outside a loop, which is a new error code

## What it must not break

DESIGN §2's reservation rule: `break` and `continue` become reserved words under
`satellite.statement` and cannot be a program's own names. Both are already
impossible as bare names, so nothing existing collides.
