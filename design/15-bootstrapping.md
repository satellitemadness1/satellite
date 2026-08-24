*satellite design docs, §15 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§14](14-spacesuits.md), On: [§16](16-including-a-file.md).*

---

## 15. Bootstrapping

The goal is not self-hosting for its own sake. It is to **delete the C++**, and the shape of
the answer follows from one fact: an interpreter written in satellite needs a satellite
interpreter to run it, so a native host survives forever. A **compiler** written in satellite
does not.

```
1. write satc (satellite -> C) in satellite; run it under the C++ implementation
2. run satc, still interpreted, on its OWN source -> satc.c -> a native binary
3. have that binary compile its own source again
   -> byte-identical output means a fixpoint, and the C++ compiler can go
```

This is how C, Go and Rust each did it; Rust's first compiler was OCaml.

**Bytecode is the one target that would prevent this**, because bytecode needs a native VM to
execute it. Emitting C source — or eventually machine code — leaves nothing of ours behind.
§10's "no bytecode, the tree stays the tree" is therefore reinforced by the bootstrap plan
rather than challenged by it. C source is the right first target for a second reason:
satellite strings can already hold arbitrary bytes (`SAT_RAW_BASE`), but emitting text needs
none of that, and a C compiler is an optimising backend nobody has to write.

### The boundary: what stays C, and why it must

| | lines | |
|---|---|---|
| **runtime — stays C** | 1,986 | `bignum`, `satellite_string`, `library`, `value`, `system` |
| **compiler — becomes satellite** | 6,061 | `lexer`, `ast`, `parser`, `env`, `eval`, `interp`, `main` |

(Headers and sources both, counted against the current tree. M6 added 166 lines to the
runtime floor and 220 to the compiler; `file` is the one item on the "cannot be written in
satellite" list that was still hypothetical when this table was first drawn.)

Each runtime piece is there because it **cannot** be written in satellite, not because nobody
got round to it:

- **bignum** — satellite's one number type *is* arbitrary-precision decimal, so implementing
  it in satellite needs something underneath it to build limbs from, and there is nothing
  underneath it. Circular.
- **satellite_string** — same argument, one level over.
- **value** — the variant every satellite value is. Circular.
- **library** — atomics, mutexes and a lock-free publish protocol, none of which satellite
  can spell.
- **system**, **file** — syscalls.

The alternative was to add a machine-word primitive visible only to runtime authors, so the
bignum could be written in satellite and the C count go to zero. **Rejected**: it puts a type
in the language that exists purely for implementation reasons, and a language with one number
type is worth more than a language with a smaller runtime. The nice consequence of the split
is that one runtime serves both consumers — the compiler runs on it, and so does every
program the compiler emits.

### Stages, and what each needs

| stage | needs | status |
|---|---|---|
| 0 — lexer in satellite | nothing | **done**, `example/bootstrap/lexer.satl` |
| 1 — parser + unparse, holding `unparse(parse(src)) == src` | nothing | |
| 2 — resolver + C code generation | a map, or accept O(n) symbol lookup | |
| 3 — self-compile to a fixpoint | nothing outstanding: file I/O is **done** (§8.3.1) and §16's include mechanism is **done** (2026-08-19) | unblocked |

Stage 0 was written before planning the rest, on purpose: it is cheaper to find out what the
language cannot say than to predict it. What it found, in order of how much it hurt:

1. **File I/O — fixed.** Source arrived through `argz` because nothing could read a file, and
   **`satellite.include` needed it too**, since including a file means reading one, so both
   were blocked behind the same feature. §8.3 designed it and M6 built it (§8.3.1). A
   satellite program can now open the source it is asked to compile, which is what stage 1
   needs and stage 0 had to work around. Both of the things stage 3 was waiting on are now
   done: this was the one that had a design, and the other — including a spaceship — got its
   name on 2026-08-18 and its loader on 2026-08-19 (§16).
2. **`break` and `continue`.** Five loops, five `going` flags, and every exit condition has to
   be reconstructed by the reader rather than read.
3. **`&&` and `\|\|`.** §3.5's two-character operators are exactly `== <= >= !=`, so the
   logical pair has never been part of the language, and `.and()`/`.or()` chains read
   backwards: `(c >= "a").and(c <= "z").or(is_digit(c))`.
4. **A map — fixed.** `satellite.container.map<K, V>` exists (§8.6), and lookup is O(1)
   through a canonical-key index. Insertion stays O(n), because the body is copied on write
   exactly as a list is — so this closes the complaint below, which was about *lookup*, and
   does not pretend to close more than that. What stage 2 was blocked on was scanning a
   symbol table on every name; that scan is now a hash probe.

   The original entry, kept because it is what the fix was measured against:
   **A map.** Only `list` exists, so every symbol table is a linear scan — correct, but O(n)
   inside a tree walker is what decides whether self-compilation takes seconds or minutes.
5. **String to number.** A ten-way `if` chain stands in for `"3".to_number()`.

What stage 0 did **not** need is the more useful half of the result: spacesuits carry AST
nodes well, `satellite.container.list<token>` works, virtual dispatch substitutes cleanly for
pattern matching, and the exact decimals make the lexer *easier* than the C++ one — there is
no `strtod`, no precision question, and `n = n * 10 + digit` is exact by construction.
