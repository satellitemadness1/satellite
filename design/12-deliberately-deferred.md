*satellite design docs, §12 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§11](11-build-order.md), On: [§13](13-decisions.md).*

---

## 12. Deliberately deferred

Everything below is still deferred, and three things are deliberately not on it: **file I/O**,
which is §8.3.1 and M6, **a way to write a `bool`**, which is §8.4, and **including a
spaceship**, which is §16 and shipped on 2026-08-19. None of the three was ever listed here —
file I/O and includes were tracked as their own sections, and the bool literals were tracked
nowhere at all, which is how `satellite.variable.bool` sat in §8's table from the first draft
with no way to name either of its two values. A deferral list is only useful if the things
missing from the language are on it, and equally only if the things on it are still missing.

- **User-defined generics** — a bare name can be a value, which reopens §4's ambiguity.
- **Durations** — `time` is an absolute instant only; `a.minus(b)` returns a number of nanoseconds.
- **Bare field access** (`my_window.height`, `my_object.my_str`) — accessor methods only.
  §14 keeps this: a spacesuit field is reachable from inside the spacesuit and nowhere else,
  which is what makes `satellite.protected` a statement about the language rather than a
  comment.
- **`<<` / `>>` operators** — permanently, per §3.5.
- **A static type checker** — types are checked at runtime, at declaration and insertion. The
  resolver does now reject a malformed *container* type before anything runs (unknown name,
  wrong generic arity, an illegal map key), which is shape checking and not type checking: it
  is what makes reading a map's second type argument safe at every runtime site.
- **`m[k] = v`, and `l[i] = v`** — assignment resolves a storage slot and an index expression
  names none. Both error identically today; fixing one fixes both, and neither is urgent.
- **An in-place fast path for container mutation** — every `.append` and `.set` copies the
  whole body, which makes building a container quadratic (§8.6 has the measurement). Safe only
  when the slot's handle is unshared, and then only for a **frame** slot; a field or a global
  may have a reader holding a snapshot.
- **`.empty()`** — id 56 exists in the registry with no implementation for any type. Adding it
  for the map alone would be a worse asymmetry than leaving it.
- **Garbage collection** — **this claim no longer holds and §14 is what broke it.** The
  original reasoning was that `shared_ptr` refcounting is sufficient because cycles cannot be
  constructed: every Value was immutable, so no Value could be made to point at something
  that points back. A spacesuit instance is mutable, so two objects can now name each other
  and neither is ever freed. This is a real leak, it is the price of reference semantics
  (§14), and it is not fixable by being careful — a cycle is a shape a correct program can
  want. Revisit when object graphs get large enough to matter; the cheap partial answer is a
  weak-reference field, the complete one is a tracing collector over the object table.
- **Passing constructor arguments to a superclass** — a `super(...)` form. Today every
  constructor above the most derived one must take none, which resolve() reports (§14).
- **User-defined operators and a spacesuit `to_string` the printer consults** — an instance
  prints as `<my_class_name>`, and a spacesuit that wants more says so with a method that
  the caller calls.
