# Assembly → satellite

**No standard, and no milestones.** This folder exists to say *no* clearly, which
[../README.md](../README.md) argues is a real answer rather than a gap.

**Written 2026-09-12.**

## What is recognised

`src/error_reporter/foreign.cpp` matches `mov `, `section .text`, `global _start`,
`syscall` and `int 0x80` — x86-64 AT&T and Intel both, since the distinctive
tokens are the same.

## The answer

> satellite has no inline assembly and none is planned — it is an interpreted
> language with no machine layer to reach. `satellite.bits` is the lowest it goes.

## Why this is not a gap to be filled

**There is nothing to emit into.** Satellite compiles to numbered ops walked by
an evaluator; there is no native code buffer for an `asm` block to sit in, and
adding one would mean a JIT — a different project, not a milestone.

**The thing assembly is usually reached for is already here.**
`satellite.variable.bits` and `satellite.variable.hex` do shifts, masks and bit
runs (`shift_left`, `shift_right`, `binary`, `hex`), which is what most inline
assembly in application code actually wants.

**And direct syscalls have a door.** `satellite.system` answers the machine's
facts and `satellite.file` opens files; a program needing more than those is
asking for a C extension, which is also not this.

## If you genuinely need it

Write the C++, put it behind a numbered path in `src/`, and it becomes part of
the language rather than an escape hatch out of it. That is how every module in
`src/satellite_*` got here.
