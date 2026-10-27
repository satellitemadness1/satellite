# QUANTUM_PLAN — quantum bits, and their config first

Written 2026-10-25 from the author's messages that day; QUANTUM-0 approved as a plan the same day. Snapshot before
the first edit: `~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-25-before-quantum-0`
(BUILD 0009).

---

## 1. The author's design

```
satellite.variable.quantum q = satellite.quantum.new(10)
```

- `q` holds every pattern 0000000000..1111111111 at once: *"it has to be a real quantum bit"*, and *"the bigger the
  quantum bit is, the longer the operation takes"*. It is a real state vector, 16 bytes a weight (the author chose 16).
- It may use a share of the machine's RAM, then the same share again on disk, with a warning when it reaches the disk.
- A program holds at most **64 quantum values** at once (*"satellite.container.list<quantum_bit> quantum_bit1 - 64"*;
  answered 2026-10-25, *"the first thing"*). That each `new(n)` takes up to `quantum.max` = 128 qubits is the plan's
  reading, not yet the author's (Section 6, 1).

The config first: *"we need to add to config a few things before we even begin"*. His rulings for it:
- `arguments.quantum.ram`, also spelled `arguments.quantum.memory`, defaults to 65.
- `arguments.quantum.drive`, also spelled `arguments.quantum.disk`, defaults to 65.
- `arguments.quantum.max = 128`, enforced *"no matter what machine you're on"*.
- A worked-out maximum: *"max_number_of_quantum_bits = 33 or whatever it is"*.

Word numbers, from words/satellite_words.hpp's next-free rows: `satellite.quantum` takes **1 38**, and
`satellite.variable.quantum` **1 6 25**. QUANTUM-1 takes them when the type is built.

---

## 2. QUANTUM-0, as built

The settings and the figures, and nothing else: no word, no quantum value.

- **Three rows** in satellite/config/satellite_config.hpp: `arguments.quantum.ram` 65, `arguments.quantum.drive` 65,
  `arguments.quantum.max` 128.
- **Two settings** in kSettings (satellite/arguments/argument_settings.hpp): `quantum.ram` and `quantum.drive`, whole
  numbers, percentages of the machine's RAM, read as satl starts (KEY-10's rule). Typed at the prompt one is saved in
  config.ini for good; a line in a file is refused as too late (35).
- **A most.** A setting may have one now, judged wherever its least is. These two are 1 to 100: 0 and 101 are refused,
  37 at the prompt and 20 in config.ini, naming the range.
- **Two spellings, one setting.** `quantum.memory` is `quantum.ram` and `quantum.disk` is `quantum.drive`
  (kSettingSpellings): read, written, saved and known to S016 as the setting itself. config.ini holds it once, under
  its own name; a write under either spelling replaces a line under the other.
- **`quantum.max`** is the author's row, read and never written. QUANTUM-1 enforces it.
- **Two figures, worked out at every start** in `Arguments::gather()` from the same bytes as `memory.total`, and never
  kept in config.ini, so they cannot go stale:
  - `arguments.quantum.possible` = floor(log2(total x (ram + drive) / 100 / 16)): the most qubits tied together in
    RAM and disk, his *"max_number_of_quantum_bits"*. **32** here (MemTotal 65,741,076 kB).
  - `arguments.quantum.possible.ram` = floor(log2(total x ram / 100 / 16)): the same in RAM alone, where the disk's
    warning will begin. **31** here.
  - A share typed at the prompt is saved, and the figures follow it from the next start.

check.sh's rows follow "a capsule declared at the prompt changes a setting for good".

---

## 3. The milestone after it

- **QUANTUM-1** — the type and `new(n)`: words 1 38 `satellite.quantum` and 1 6 25 `satellite.variable.quantum`.

What comes after QUANTUM-1, and in what order, is the author's to name (Section 6, 6).

---

## 4. A figure corrected

65% of RAM plus 65% on disk is 87.5 GB on this machine. That holds **32** qubits tied together, not 50: 50 tied
qubits would need 18 petabytes. And 128 tied together (2^128 weights) fits on no disk. So `quantum.max = 128` works
only if qubits stay separate until an operation ties them: `new(128)` then costs 4 KB, and RAM plus disk caps how many
are tied together at once (32 here). QUANTUM-0 records both numbers.

---

## 5. Not in this plan

- `arguments.object_bytes_max` (32 GiB, read by nothing) is unchanged. Quantum has its own limits.
- The 64 values a program holds: QUANTUM-1's to enforce, with no row before then.

---

## 6. For the author

1. ~~**The 64.**~~ **Answered 2026-10-25:** *"the first thing"*. 64 is the most quantum values a program may hold at
   once. That answer settled only the 64. **Still open:** the plan reads `quantum.max` = 128 as the most qubits ONE
   `new(n)` takes (satellite_config.hpp says so as the plan's reading) -- is that his meaning?
2. **Displaying all possibilities.** `new(10)` displays 1,024 patterns, each with how likely it is. `new(128)` has
   2^128, more than could ever print. Should it be every pattern up to some size and a summary past it, or only the
   patterns that can come up? **The author, 2026-10-25: *"not sure".*** The recommendation put to him: every
   possibility with its chance up to 1,024 of them, and past that one line saying how many there are, with the 16
   likeliest.
3. **Possibilities made of strings.** For example `satellite.quantum.new({"red", "green", "blue"})` makes 3
   possibilities in 2 qubits, with the 4th pattern never used. That is his first example, `new(3)` = 2 bits, losing 1.
4. **Start state.** Every pattern equally likely, or all zeros?
5. **Operations and their names.** Measure; spread a qubit; tie two; turn a phase.
6. **The milestones after QUANTUM-1 -- proposed, the author's to name and order.** Operations; the disk and its
   warning; possibilities made of strings.
7. **The figures are worked out, not stored.** He wrote that they would be *"stored into config.ini"*.
   `quantum.possible` and `quantum.possible.ram` are worked out at every start instead, because of the 2026-10-03
   ruling that a measured machine figure kept in config.ini goes stale (machine_facts.hpp, the whole memory moving
   with a kernel update or a resized machine). The 65s are the stored settings: `quantum.ram` and `quantum.drive`,
   saved in config.ini when a person sets them at the prompt.
