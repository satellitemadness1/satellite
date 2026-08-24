*satellite design docs, §6 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§5](05-grammar.md), On: [§7](07-method-calls-indexing-slicing.md).*

---

## 6. Scope model — the most important decision

**Verified blocker.** `Library` keys are literally `<function_name>.<var_name>`
(`library.cpp:44-48`), and `intern()` returns the same `shared_ptr<Variable>` for that key
forever (`library.cpp:22-34`). So a capsule has **exactly one slot per local for the whole
program**, with no per-call storage.

**Recursion collapses.** Simulated against the real `library.cpp`, a recursive `fact` built
on `Library::set/get` returns **1 for every input** — 1, 2, 3, 5, 10 all yield 1, because
the base case's write to `fact.n` is the last one standing. The failure is
evaluation-order-dependent: caching `n` in a C++ temporary before the recursive call makes
the same source return 6 correctly, which is the worst possible property for a bug.

**Concurrency is worse, and it is the part that would not be seen coming.** Eight threads
running a capsule with *no recursion and no shared state* produced **1585 wrong results out
of 1600**. The write lock does not help: `satellite.library` provides *atomicity* (no torn
value) but not *isolation* (private storage), and locals need isolation.

This cannot be reframed as deliberate "capsule-static" semantics, because parameters are
locals too — `argz` would be a program-wide static.

### A redeclaration in one scope rebinds — §19.3

Added 2026-08-24, and it is a change to this section's rules rather than a footnote
on them. Declaring a name twice in one scope used to be an error; it now **rebinds
the name to a fresh slot**. The reasoning is in §19.3, and the half worth carrying
here is that the fresh slot is not an implementation detail: §14 makes a spacesuit
a reference type, so reusing the old slot would leave every handle already taken to
the first instance pointing at the second, and a list built by that idiom would read
back as *n* copies of its last element **with no error anywhere**. This section
already promised what makes it correct — slots are never reused across scopes — so a
new slot per declaration is the grain of the design below rather than a new rule.

### The fix: slot-indexed call frames

`satellite.library` is reserved for **shared/global state**. Every capsule call gets its own
frame; locals resolve to integer slot indices **statically, before execution** — in a
separate `resolve()` pass (`env.hpp`), not in the parser.

The original wording here said "at parse time", which reads as a contradiction of §5's
"keep the parser resolution-free" and is not one: what matters is that a name is resolved
*once, ahead of the tree walk* rather than looked up on every read. Putting it in the parser
would not work anyway — **a capsule may call one defined further down the file, and mutual
recursion (`is_even`/`is_odd`) is unresolvable in single-pass recursive descent.** The
resolver therefore runs in three passes: capsule names first, then top-level statements,
then each body. Two properties fall out: the parser's purity survives untouched, and a
resolver bug reports `unknown variable in capsule: x` *before* anything executes instead of
producing a wrong value somewhere inside the walk.

```cpp
struct Frame {
    std::vector<ValuePtr> slots;   // no mutex, no atomic: reachable from one thread
};
```

Three things make this the clear choice:

1. **`library.hpp` / `library.cpp` need zero logic changes** — one doc comment and a
   `function_name` → `namespace_name` rename. Nothing stress-tested is discarded. The
   Library is not being demoted; it is being pointed at the data it was designed for.
   Its properties (permanent identity, cross-thread sharing, lock-free reads, atomic
   read-modify-write) are exactly what globals need and exactly the opposite of what
   locals need.
2. **Frames are faster.** 200k × `fact(15)`: **160 ms with frame slots vs 1131 ms through
   the Library — 7.1× faster**, and the only correct option. Correctness here is not a tax.
3. **It does not violate "no bytecode VM."** The tree stays the tree; a variable-reference
   node just holds an integer instead of a string.

### Rejected alternatives

- **Key mangling (`main#3.x`)** — looks cheapest, is catastrophic. `intern()` copies and
  rehashes the entire directory per new key (`library.cpp:30`), which is quadratic:
  1000 keys 54.7 ms, 4000 keys 2.6 s, **16000 keys 70.5 seconds**. There is also no
  `erase()` anywhere in the Library, so frame keys leak permanently.
- **Declaring recursion unsupported** — does not fix the concurrency failure at all
  (1585/1600 wrong with zero recursion), is not statically enforceable once lists can hold
  capsule references, and the runtime already recurses: `to_string` in `value.hpp:36-58`
  walks nested lists, and `library_test.cpp:28-31` exercises it deliberately.

Add a configurable `satellite.library.system.max_depth` (default ~10000, read exactly like
`system.min_free_mb`) so runaway recursion raises a satellite stack-overflow error instead
of segfaulting the C++ stack.

**Built, and the ~10000 above was wrong** — it sat past both stack cliffs, so the guard could
never fire and the segfault it exists to prevent was exactly what a runaway recursion got. The
default is 2000, one activation costs 3 units and ~3169 bytes at -O2, and the ceiling is
derived from `RLIMIT_STACK` rather than fixed: 3000 on the ordinary 8 MB stack, ~24,000 on
64 MB, ~24,577,781 on 64 GB. `ulimit -s` is therefore the knob for how deep a program may
recurse, and it is outside the language on purpose. The measurements are in src/evaluator/helpers.cpp,
above `DEFAULT_MAX_DEPTH` and `max_max_depth()`.

**Capsule definitions live in a separate `CapsuleTable`, not the Library.** A bare capsule
name cannot even form a legal Library key — **verified**: `set_path("fact")` is rejected,
because `normalize_path` (`library.cpp:68-76`) requires a dot on both sides. Capsules are
written once at parse time and never reassigned; they need none of the Library's machinery.
