*satellite design docs, §10 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§9 part 2](09-b-the-console-and-windows.md), On: [§11](11-build-order.md).*

---

## 10. Evaluator

Tree-walking, per R5. No bytecode VM. Three findings carry hard measurements.

### Split `Expr` from `Value`

The `value.hpp` comment proposes folding AST node kinds into the same variant. **Drop it,
and keep the spirit.** Measured: adding the AST kinds grows every runtime `Value` from
**40 to 96 bytes** — a 1M-number list goes 38 MB → 91 MB. Under R2 there is also no
`satellite.variable.<something>` for "call node", so the type checker would need arms
meaning "unreachable".

Use a separate `Expr` variant that deliberately mirrors `Value`'s idiom. Recover full
homoiconicity later via **one** new alternative rather than fifteen:

```cpp
struct Quoted { ProgramPtr prog; const Expr *node; };   // satellite.variable.expression
```

That is the whole of the `satellite_expression` idea, at zero cost today. A prototype of the
split runs hello world end to end.

### `satellite.return` is a status enum, not an exception

**Measured: 1537 ns per return as a C++ exception vs 8.5 ns for a `Flow` enum — 181×.**
At one return per call that is seconds of pure unwinding in any recursive program. Reserve
exceptions for genuine errors.

### Caret positions must never come from `decode()`

**Verified**: `decode()` is not injective (`\home` is 1 SatChar, 5 source bytes, 13 display
bytes) and **not even stable** — `decode({SAT_CWD})` returned 97 bytes, then 1 byte after a
`chdir`, for the same SatChar. Produce a `byte_of[]` side table during `encode_raw` mapping
each SatChar index back to its source byte offset, and drive carets from that.

### Fix `to_string` before adding any type — **done**

**Verified blocker, since fixed.** `to_string` tested four alternatives with `get_if` then fell
through to an unconditional `return decode(std::get<SatString>(v));`. Adding a `Time`
alternative compiled with **zero warnings** under the project's own flags and then threw
`std::get: wrong index for variant` on first print. The REPL's `:get` and `:vars` both call
it, so the first `:get main.my_time` would have crashed the interpreter.
v1 adds four to six alternatives, so this would have fired four to six times.

`to_string` is now `std::visit` over an exhaustive overload set — one `operator()` per
alternative, **no generic `auto` fallback**, since a fallback silently reintroduces the bug.
It was the highest-value twenty lines in the codebase, and it paid: `Time`, `Number`,
`FilePtr` and `ObjectPtr` all arrived afterwards, and each one was a compile error in
`ValuePrinter` rather than an archaeological dig through a crash.

### Two more Library fixes, filed "before resources land" — resources have landed

**Neither is done, and §8.3.1's `file` is what makes them live.** They were cheap when every
Value was a number, a string, a bool or a list; a `satellite.variable.file` in
`satellite.library` is a real descriptor closed under a real lock.

- **No Value destructor may run while a write lock is held.** **Verified**: both
  `set_key` (`library.cpp:36-42`) and `update` (`library.cpp:50-59`) destroy the previous
  Value under the lock. Harmless for a `double`; with a file it means `close(fd)` under the
  lock, which can block for seconds on NFS/FUSE; with a window it means GTK teardown from a
  non-GTK thread, which is undefined behavior. In `update`, declare `ValuePtr doomed;`
  *before* `var`, scope the lock inward, and end with `doomed = std::move(current);`. In
  `set_key`, use `exchange()` and let the old pointer die after the lock.
- **Tighten `normalize_path`.** It silently accepts >2-segment paths and fabricates a
  garbage variable rather than erroring. After stripping the prefix, require
  `key.find('.') == key.rfind('.')`. Three lines, and worth doing now while the only caller
  is the REPL.

### The `main.x` collision

`normalize_path` makes the `satellite.library.` prefix optional, so `main.x` and
`satellite.library.main.x` are literally the same variable — **verified: identical pointer**.
But §7 needs `main.x` to mean "receiver `main`, member `x`". Two rules claim one token stream.

**Resolution:** the bare two-segment form is illegal in program source. Source text requires
the full `satellite.library.main.x`. Keep `normalize_path`'s prefix-optional behavior scoped
to the REPL `:get`/`:set` debug commands, which `main.cpp:61-62` already documents as
temporary. That is where they still are, and they are still the only callers.
Once the parser exists, stop feeding `get_path`/`set_path` raw dotted strings from user code
entirely — the evaluator should walk the resolved chain and call the typed
`Library::get/set/update` with two already-separated segments.
