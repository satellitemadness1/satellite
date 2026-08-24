# Implementation Plan — `satellite.variable.thread`

Target surface, exactly as specified:

```
satellite.variable.thread my_thread = satellite.thread.new(capsule_name(arg1, arg2))
my_thread.start()
my_thread.join()
```

Everything below was checked against `src/` unless explicitly marked otherwise. Where the five readers disagree, the disagreement is named and resolved with a reason rather than smoothed over.

---

## 0. The three facts the whole plan rests on

**(a) A tenth variant alternative behind a `shared_ptr` does not move `sizeof(Value)`.** I re-proved this myself, not just from a reader's report: compiling a TU that declares

```cpp
using ValueBase10 = std::variant<std::monostate, bool, Number, Str, ListRef,
                                 ObjectPtr, Time, FilePtr, MapRef, ThreadPtr>;
static_assert(sizeof(ValueBase10) == 40, "grew");
```

against the real `src/satellite_value/value.hpp` under `c++ -std=c++20 -Wall -Wextra -Isrc` compiles clean — both the header's own asserts (`value.hpp:265-267`, `value.hpp:301-305`) and mine hold. The payload is sized by `Number` at 32 bytes; a 16-byte `shared_ptr` does not widen it, and the discriminant is one byte in the existing 7 bytes of tail padding. **This is only true if the handle lives behind a `shared_ptr`.** A `ThreadHandle` held by value (an `std::thread` plus a mutex plus a condvar is 80+ bytes on libstdc++) fires both asserts. If that happens, the fix is the `shared_ptr`, never the assert.

**(b) `satellite.thread.new(worker(1,2))` runs `worker` today, at the call site.** `eval_call` reduces every argument at `src/evaluator/expr.cpp:353-355` before the module path is even flattened (`:357-374`). Two readers independently confirmed this at runtime — the program prints `worker ran` and only then fails with `no such module function: satellite.thread.new`. I verified the code ordering; I did not re-run the program. **Getting this wrong produces a feature that passes any test which only checks the final answer and is a complete no-op concurrently.**

**(c) The two exhaustive `std::visit` sites are the only compile-time tripwires.** Everything else that dispatches over the variant is a `switch (value.index())` with a `default:` arm or a `get_if` chain, and none of it will fail to build. `§10`'s "the compile errors are the feature" is **half true, and the half that is false is where the silent bugs are**. Section 3 lists both halves.

---

## 1. Threading model — decided up front

### 1.1 One Evaluator per worker, constructed *on* the worker

A worker does **not** share, copy, or reuse the spawning `Evaluator`. It constructs a fresh one over the same `const ResolveResult &`:

```cpp
Evaluator worker(resolved, ns, /*echo=*/false);
```

Both existing construction sites already have exactly this shape (`src/interpreter/interp.cpp:231`, `:276`), so nothing new is needed to build one.

Two constraints that are easy to get wrong and invisible when you do:

- **The construction must happen inside the thread function**, not in `.start()` on the spawning thread. `Evaluator::Evaluator` calls `read_max_depth()` (`session.cpp:12-16`), which derives the recursion ceiling from the *calling thread's* stack. Constructing the worker Evaluator on the parent's stack silently gives the worker the main thread's ceiling and defeats the fix in §7.3.
- **`echo_` must be false on a worker.** Echo is a REPL affordance (`session.cpp:78`); a worker echoing injects into the prompt.
- **`ns_` must be inherited from the parent** (normally `"main"`), so a worker's top-level variable writes land in the same `satellite.library` namespace. That sharing is intended — it is §6's whole mechanism.

`Evaluator` today declares no copy/move control at all (`eval.hpp:75-326`), so the implicit copy constructor exists and `Evaluator worker = *this;` compiles. It copies `current_` — a raw pointer into the *parent's live C++ stack* — plus `depth_` mid-recursion and a snapshot of `errors_` that is never merged back. **Delete copy and move explicitly** so the path of least resistance is not the catastrophic one.

### 1.2 What is per-thread, what is shared, and what guards each

| Thing | Kind | Guard |
|---|---|---|
| `Frame` and its `slots` (`eval.hpp:56-58`, `calls.cpp:85-92`) | per-thread, on that thread's C++ stack | **none, and none needed** — see §1.3 |
| `current_`, `current_capsule_`, `current_self_`, `current_suit_` (`eval.hpp:295-307`) | per-thread walker registers | none; per-Evaluator |
| `returned_` (`eval.hpp:285`) | per-thread; `satellite.return`'s only channel | none; per-Evaluator |
| `depth_` (`eval.hpp:315`) | per-thread | none; per-Evaluator |
| `max_depth_` (`eval.hpp:320`) | per-thread, **recomputed on the worker** | none |
| `errors_` (`eval.hpp:284`) | per-thread, merged at join | registry mutex at the merge |
| `output_` (`eval.hpp:282`) | per-thread, merged at join | registry mutex at the merge |
| `division_digits_` (`eval.hpp:325`) | **inherited from the parent**, not re-read | none |
| `ns_`, `echo_` | inherited, read-only | none |
| `resolved_` — `ResolveResult`, `CapsuleInfo`, `SpacesuitInfo`, the `Program`, the `SourceMap` | shared, read-only after `resolve()` | none; `ast.hpp:236-242` states the invariant: `resolve()` must finish, single-threaded, before any evaluation begins |
| `declared_` (`eval.hpp:313`) | **snapshot-copied at spawn** — see §1.4 | none after the copy |
| `satellite.library` | shared, mutable | per-`Variable` `std::mutex write_lock` for writers; readers do one `load(acquire)`; the directory is copy-on-write behind `intern_lock_` (`library.cpp:15-34, 83-98`) |
| `Object` fields | shared, mutable | `Object::write_lock` for writers; `std::atomic<ValuePtr> fields[i]` for readers (`value.hpp:141-152`) |
| `Console` | shared, mutable | `mutex_` around `pending_`; `pace_ns_` is atomic and deliberately outside it (`console.hpp:145-150`) |
| `ValuePtr` handed across threads | shared, **immutable pointee** | atomic refcount only. Caveat: an `ObjectPtr` or `FilePtr` *inside* a Value names something mutable |
| the `Image` (Program + ResolveResult + SourceMap) | shared | a strong `shared_ptr` on every live handle, plus the join barrier (§1.5) |
| the thread registry | shared | its own `std::mutex` |

**Honest caveat on "lock-free":** Reader 3 measured `std::atomic<std::shared_ptr<T>>` on this toolchain as `is_lock_free() == 0` — libstdc++ implements it with an internal spinlock. The guarantee `library.hpp:17-21`, `value.hpp:132-136`, `slots.cpp:138-141` and `design/14:115` actually deliver is *no torn read and no user-visible mutex*, not lock-freedom. Do not reason about the field path as if reads were free — it matters directly to hazard C7.

### 1.3 Reconciling §6 with a worker that has its own frame stack

`§6` says a `Frame` needs no mutex because it is reachable from exactly one thread. `slots.cpp:138-141` says it in code:

> `// No lock, and none needed: a frame is reachable from exactly one thread. That is the whole difference from satellite.library, and it is what turns §6's 1585/1600 into 0/1600.`

A worker thread **does not weaken this — it instantiates it.** A `Frame` is a plain C++ local of `call_capsule` (`calls.cpp:85-92`); a worker has its own C++ stack, therefore its own Frames, therefore each Frame is still reachable from exactly one thread. Two concurrent activations of the *same capsule* need nothing new from `Frame` or `CapsuleInfo`: `slot_count` and `slot_types` are static const data in the `ResolveResult`.

What breaks is not the frame — it is the **register that points at the frame**. `Frame *current_` is a raw pointer into whichever thread's stack is inside `call_capsule`, and `Activation` (`calls.cpp:19-61`) holds *five non-const references to Evaluator members* and restores them in its destructor. Two threads through one Evaluator means thread A's `Activation` destructor writes A's saved `current_` over B's live one; B then reads slot 2 out of A's frame, gets a wrong value with no error, and once A's C++ frame unwinds, `current_` dangles. The bounds check at `slots.cpp:41-47` cannot catch it, because the stale frame is a real, correctly-sized `Frame`.

So the rule to write into `eval.hpp` is sharper than §6's:

> **A frame needs no lock because it is reachable from one thread. What must be per-thread is the register that points at it.**

Giving each worker its own `Evaluator` makes both halves true simultaneously.

**Reader 2 proposed a different shape** — splitting the 16 members into a per-thread `struct Walker` (registers + `returned_` + `errors_` + `output_`) and a shared read-only half, with `Activation` and `DepthGuard` retargeted at the Walker. That is a better long-term factoring and Reader 2 is right that the seam already exists in the code. **I recommend deferring it.** The whole-Evaluator-per-worker route is a strictly smaller change, uses the constructor that already exists, and is what both current construction sites do. The Walker split becomes worth doing when the per-Evaluator duplication of `declared_` and `output_` starts hurting — not before. Say so in the commit message so the next person knows it was a decision.

### 1.4 `declared_` — the one shared thing that is silently wrong

`declared_` is the *only* record of a top-level variable's declared type. It is written at exactly one place (`stmt.cpp:97-98`, on a top-level `VarDecl` with a `SLOT_GLOBAL` slot) and read at `slots.cpp:171-172`. A `VarDecl` inside a capsule body resolves to a frame slot, so **a worker never writes it** — but a worker with its own empty map reads `nullptr` for every global, and the assignment type check at `stmt.cpp:125-131` and the `console.input` out-parameter check at `modules.cpp:823-829` become dead code. Result: a worker assigns a string to a variable declared `satellite.variable.number` and nobody complains.

Two readers found this independently. Neither offered a mitigation I'd ship as-is.

**Recommendation: snapshot-copy `declared_` into the worker Evaluator at spawn time.** It is small (a `Type` per top-level variable), it needs no lock, and it avoids the real data race in the alternative — a *shared* map is written by the main thread at any top-level declaration while a worker reads it, and an `unordered_map` insert against a concurrent read is undefined behaviour, not a lost write.

The semantics this buys must be **written down, not discovered**: a worker sees the global declarations that existed when it was created. A global declared after the spawn is invisible to that worker's type check. That is defensible and consistent with the argument-capture rule in §4; it is not defensible if nobody knows it.

### 1.5 Lifetime: a mandatory join barrier, plus a keepalive

This is the largest architectural decision and it resolves five of the six crash-class hazards at once.

Today `image` is a plain local `shared_ptr` in `run_program` (`interp.cpp:260`), the entire walk happens inside one call on that frame (`:278`), and retention is conditional on **spacesuits only** for a file run (`:293-294`). The ordinary threaded program — capsules, no spacesuit — is precisely the case that is *not* retained. A worker outliving `run_program` walks a freed AST.

**Design:**

1. A per-run `ThreadRegistry { std::mutex lock; std::vector<ThreadPtr> live; }`, created in `run_program` / `run_source`, held by the root `Evaluator` as a `std::shared_ptr<ThreadRegistry>` and **inherited by every worker Evaluator** (a worker may itself spawn).
2. `.start()` pushes a **strong** `ThreadPtr` into the registry. Consequence: the handle can never be destroyed before the barrier runs — which is what defuses the `std::terminate` hazard entirely, including the frame-local form and the cycle-leak form.
3. Every handle carries `std::shared_ptr<const void> image` — a type-erased keepalive on the `Image`, installed by the Evaluator from a new `set_image_keepalive()` that `interp.cpp` calls right after constructing the Evaluator. Type-erased so `thread_handle.hpp` need not know `interp.hpp`. Belt and braces: even a handle that escapes the barrier still has a live tree to walk.
4. **The barrier**: `evaluator.join_all_threads()` inserted in `interp.cpp` immediately after `run()` / `run_entry()` and **before** `console->drain()` (i.e. before `:242` and before `:287`). It loops — draining the registry, joining, re-checking, because a worker may register more — then merges every worker's `errors_` and `output_` into the root's under the registry lock.
5. `~ThreadHandle` joins defensively if still joinable. It should never fire, because the registry guarantees the barrier ran first, but it must **never** call `std::terminate` and must **never** `detach()`.

**Ordering matters and is easy to get backwards:** join, *then* drain. `drain()` (`console.cpp:71-75`) waits only until the queue is empty; it has no concept of a writer that has not queued its line yet. `console.hpp:44-47` already says so in its own words — "the writer must be finished first".

**Cost, stated honestly:** a program that starts a thread and never joins it *blocks at exit* until that thread finishes. That is the right trade for this language. Detaching would trade a visible pause for six nondeterministic crashes, and it makes `satl --run`'s promise that its output is complete un-keepable.

---

## 2. Files to touch, in dependency order

Each entry says what changes and why. Commit boundaries are suggested in §10.

### 2.1 `src/evaluator/slots.cpp` and `src/evaluator/mutators.cpp` — **fix first, before any thread code exists**

This is a pre-existing bug that threads escalate from "a stall on NFS" to "a self-deadlock". Only Reader 3 found it; I verified both sites.

- `slots.cpp:120-132`, `write_slot`'s field arm: `cell->store(std::move(value))` runs inside `std::lock_guard guard(current_self_->write_lock)`. The store releases the atomic's previous `shared_ptr` inside the call, and the cell is normally the sole owner, so `~Value` runs **under the lock**. For a thread-typed field that is `join()` — blocking for the whole lifetime of a worker — while holding the mutex every other method on that object needs, *and* (per §1.2's caveat) while holding libstdc++'s internal `shared_ptr` spinlock.
- `mutators.cpp:112-127`: `ValuePtr current` is declared *inside* the lock scope, so it dies at the closing brace with `write_lock` still held. Verbatim the trap `library.cpp:49-51` documents.

**Fix, already written twice in this tree:** hoist a `ValuePtr doomed;` above the `{ std::lock_guard ... }` scope; take the old value with `exchange` (slots) or `doomed = std::move(current)` (mutators). Copy the shape of `library.cpp:52-56` and `library.cpp:83-90` exactly, including their comments' reasoning about why `exchange()` alone does not fix it (the returned temporary still dies inside the lock).

**Do not** hoist `transform()` out of the mutator lock — `library.cpp:65-82` explains why that turns a correct read-modify-write into a lost one.

### 2.2 `src/satellite_value/thread_handle.hpp` — **new file**

Deliberately **not** included by `value.hpp`. `value.hpp:6-13` pulls `<atomic> <memory> <mutex>` and is included by essentially the whole tree; adding `<thread>` and `<condition_variable>` there makes every TU that touches a `Value` pay for it.

The precedent is split and the choice must be deliberate: `FileHandle` is defined *inline* in `value.hpp:175-187`, while `SpacesuitInfo` is only forward-declared at `value.hpp:22` with an out-of-line accessor. **Follow `SpacesuitInfo`, not `FileHandle`**, because `ThreadHandle` drags in `<thread>` and `FileHandle` drags in nothing.

Include order is clean: `ast.hpp` (verified — it includes only `lexer.hpp` and `satellite_string.hpp`, never `value.hpp`) plus `value.hpp`. This header sits above `value.hpp` and below `env.hpp`. No cycle.

```cpp
// src/satellite_value/thread_handle.hpp
#pragma once
#include "abstract_syntax_tree/ast.hpp"   // Span
#include "satellite_value/value.hpp"      // ValuePtr
#include <atomic> <functional> <mutex> <string> <thread> <vector>

namespace satellite {
struct CapsuleInfo;                        // environment/env.hpp

struct ThreadHandle {
    enum class State { Fresh, Running, Finished, Joined };
    std::atomic<State> state{State::Fresh};

    // Captured at satellite.thread.new; never written again after start().
    const CapsuleInfo *capsule = nullptr;
    std::string        name;
    std::vector<ValuePtr> argv;            // evaluated on the SPAWNING thread
    Span span;

    // Keeps the Image (Program + ResolveResult + SourceMap) alive at least as
    // long as this handle. Type-erased so this header need not know interp.hpp.
    std::shared_ptr<const void> image;

    // Installed by the evaluator at start(): this header must not know
    // eval.hpp, and the evaluator is where call_capsule lives.
    std::function<void(ThreadHandle &)> body;

    std::mutex done_lock;                  // guards the two below
    ValuePtr result;
    std::vector<std::pair<std::string, Span>> errors;

    std::thread os_thread;
    ~ThreadHandle();                       // joins if joinable; NEVER terminates
};
using ThreadPtr = std::shared_ptr<ThreadHandle>;
} // namespace satellite
```

Note the error carry is a `pair` vector rather than `EvalError` — `EvalError` is declared in `eval.hpp:60-63`, above this header. The tidier alternative is to move `struct EvalError` down beside `Span` in `ast.hpp`; that is a clean move but it touches every `eval.hpp` reader, so it belongs in its own commit if you want it.

`std::function` costs ~32 bytes but lives *behind* the `shared_ptr`, so it has no effect on `sizeof(Value)`.

### 2.3 `src/satellite_value/value.hpp`

1. Forward-declare beside `SpacesuitInfo` (`:22`): `struct ThreadHandle;`
2. Alias beside `FilePtr` (`:187`): `using ThreadPtr = std::shared_ptr<ThreadHandle>;`
3. **Append** `ThreadPtr` to `ValueBase` at `:203-204` as **index 9**. The comment at `:199-202` is not decoration — inserting instead of appending renumbers `help_for()` and `module_of()` and silently changes what both mean.
4. Declare (not define) the printer arm in `ValuePrinter` (`:317-344`): `std::string operator()(const ThreadPtr &) const;`
5. **Both `static_assert`s at `:265-267` and `:301-305` stay untouched and keep passing.** Verified in §0(a).
6. **Do not** include `<thread>` or `<condition_variable>` here.

### 2.4 `src/satellite_value/value.cpp`

Add `#include "satellite_value/thread_handle.hpp"` (this TU already includes `environment/env.hpp` for the same reason, `value.cpp:3-9`).

Two **mandatory** additions, both hard compile errors until written:

- **`ValuePrinter::operator()(const ThreadPtr &)`**, beside the `FilePtr` arm at `:108-114`. Copy its shape exactly: one atomic load, format, return. Something like `<thread worker, running>` / `<thread worker, finished>` / `<thread>` for nil. **It must never block, never join, and never take `done_lock`.** `to_string` runs from `satellite.console.display`, the REPL echo, `.to_string()`, `:get`, `:vars`, and *every wrong-type error message* — `value.cpp:27-30` says it prints "from an error message about an object whose construction did not finish". A printer that joined would hang `satl` on a print; one that took the handle's lock would deadlock the REPL against a running worker.
- **`SizeVisitor::operator()(const ThreadPtr &)`**, after the `MapRef` arm at `:223-241`. Bill **only what the handle owns**, guarded by `walk.first_time(handle.get())`, and follow the two precedents literally:
  - `FilePtr` (`:215-221`): "The path, and nothing else… those bytes are the kernel's, and billing a program for them would be reporting on the wrong thing." A worker's kernel stack is the same category. **Do not bill it.**
  - `ObjectPtr` (`:198-210`): children are reached through an atomic load because a size walk is a reader like any other. The captured `argv` is written once before `start()` and never again, so it is safe to walk directly — but say that in the comment, because it is a claim, and if `argv` ever becomes mutable the comment is what makes the race findable.
  - Suggested billing: `name.size()` + `argv.size() * VALUE_HANDLE_BYTES` + `walk.of_child()` per element. The `result` is deliberately *excluded* unless you take `done_lock` — and you must not — so state that `.size()` of a thread does not include its result.

### 2.5 `src/evaluator/types.cpp` — **the two changes without which everything else is dead code**

1. **`module_of` (`:79-100`)**: add `case 9: return "satellite.variable.thread";` before the `default:` at `:98`. This is §7's variant-index → module-path table, and `eval.hpp:333-337` explains why it is a table keyed on the index rather than derived from the type name.
2. **`matches` (`:14-77`)**: in the `type.space == "variable"` arm, add
   ```cpp
   if (type.name == "thread")
       return std::holds_alternative<ThreadPtr>(value) ||
              std::holds_alternative<std::monostate>(value);
   ```
   following the `file` precedent at `:40-44` verbatim, including its comment: *nil satisfies a reference type because a reference may name nothing.* Without this, `satellite.variable.thread t = satellite.thread.new(...)` fails with "cannot initialise … with …".

**Order matters between these two and §2.8.** `call_method` reads `module_of` first and bails at `methods.cpp:51-55` when it is null. If the `ThreadPtr` arm in `methods.cpp` lands before `case 9`, that arm is **unreachable dead code** and the symptom is the misleading `nil has no methods, so .start() has no receiver` — not an obviously unfinished feature.

### 2.6 `src/evaluator/helpers.cpp`

- **`default_of` (`:72-101`)**: `thread` follows `file` (`:86-90`) and falls through to `std::monostate{}` at `:100`. **Make this a decision rather than the accident it is today** — add a comment. A declaration has no capsule to name and nowhere to report a failure, so a declared thread is nil until `satellite.thread.new` fills it. The `matches` arm above is what makes that legal.
- **`value_equals` (`:112-186`)**: **no arm needed.** The `ValueBase` fallback at `:185` compares the `shared_ptr`s, which is identity, and identity is the right equality for a reference type — exactly the `ObjectPtr` argument at `:103-110`. The warning at `:137-141` is about future alternatives with *value* semantics; a thread has reference semantics.
- **`module_constant` (`:245-266`)**: **leave it answering null for `{satellite, thread}`.** Making a module path a value is what broke `satellite.directory.list()` (`:254-259`); `expr.cpp:369-373` would read `satellite.thread.new(...)` as `.new()` called on the value `satellite.thread`.
- **`is_mutator` (`:622-625`)**: **leave it alone.** `start`/`join` are not container mutations. This is a decision, not an omission: `is_mutator` routes to `call_mutator` *before* the receiver is evaluated because a container mutation needs a storage slot to write back through; a thread mutates an OS resource behind a `shared_ptr` and has nowhere to write back, so `call_mutator` would report a storage error. `expr.cpp:392-399` records the mirror-image regression (widening `is_mutator` for the map broke a spacesuit method named `set`).
- **`max_max_depth` (`:680-697`)**: the depth-guard fix — see §7.3.

### 2.7 `src/evaluator/expr.cpp` — **the crux**

Insert the fourth argument-expression match **after the `NamedArg` loop (ends `:292`) and before the capsule arm (`:296`)** — necessarily before the general `eval_args` at `:353-355`.

**Match on the TARGET path first, not on the argument shape.** This is where I resolve a disagreement between Readers 1 and 4. Reader 4 proposed `node.args.size()==1 && holds_alternative<Call>(node.args[0])`; that shape is right for accepting the good case but wrong for everything else, because `satellite.thread.new(5)` and `satellite.thread.new(worker)` then fall through to `eval_args` and land in `call_module`'s terminal `no such module function: satellite.thread.new` (`modules.cpp:731`) — a message that is true and useless. Matching on the target means every wrong spelling reaches **one message written for it**, which is precisely the argument the `NamedArg` arm makes at `expr.cpp:263-272`.

```cpp
// satellite.thread.new(worker(a, b)) — matched on the ARGUMENT EXPRESSION,
// before eval_args, for the same reason the duration form, the help topic, the
// console.input PLACE and the named argument above are: the argument is a CALL
// TO BE MADE LATER, not a value, and eval_args would make it NOW, on this
// thread. Verified before this arm existed: the capsule ran to completion at
// the declaration line and thread.new received its return value.
std::vector<std::string> path;
if (flatten_path(*node.target, path) &&
    join_path(path) == "satellite.thread.new")
    return thread_new(node, span);
```

The four precedents this copies, in the file, in order:

| Precedent | Lines | What it matches on |
|---|---|---|
| `DurationLit` — `display(100ms)` | `expr.cpp:189-207` | argument expression; comment at `:190-193` states the mechanism |
| `satellite.help(topic)` | `:209-247` | a bare WORD via `bare_dotted_path` |
| `console.input(prompt, target)` — a **PLACE** | `:249-261` | argument expression; the closest analogue — the argument names storage, not a value |
| `display(x, end=…)` — `NamedArg` | `:263-291` | scans every position; one accepted shape, one named error for everything else |

### 2.8 `src/evaluator/threads.cpp` — **new file**, and add it to `EVAL_SRCS`

A member-function TU of `Evaluator` (it needs private `call_capsule`). Adding it to `EVAL_SRCS` at `Makefile:310-315` puts it into `OBJS`, `LIBOBJS` **and** `TESTSRCS` in one edit — verified: `LIBOBJS = $(filter-out $(PROGRAMS)/main.o,$(OBJS))` (`Makefile:363`) and `TESTSRCS` includes `$(EVAL_SRCS)` (`:347-351`).

**`Evaluator::thread_new(const Call &node, Span span)`** — declared in `eval.hpp` beside the three existing node-taking handlers (`set_display_pace :210`, `display_with_end :215`, `console_input_into :225`), with the same comment shape explaining that it takes the argument NODE because a capsule call is not a value at this position.

Steps, in order, each with its own refusal (see §6):

1. Exactly one argument, or fail with the shape: *"satellite.thread.new takes one argument: a capsule call, as satellite.thread.new(worker(1, 2))"*.
2. That argument must be a `Call`. A bare `Name` (`satellite.thread.new(worker)`) gets its own message naming the call form.
3. The inner `Call`'s target must be a `Name` whose slot is a capsule. **Mirror `eval_call`'s own four refusals at `:296-333`**, each with a thread-flavoured message: `slot >= 0 || is_field_slot(slot)` → *"… is a variable, not a capsule"*; `SLOT_SUIT` → *"… is a spacesuit; a thread runs a capsule, not a construction"*; `SLOT_METHOD` → *"a thread runs a capsule; a method needs a receiver"*.
4. `const CapsuleInfo *info = resolved_.find(name->text);` — null → *"no such capsule: …"*. The pointer is stable: `capsules` is a node-based `unordered_map` (`env.hpp:185`, `run.cpp:88-92`).
5. **Evaluate the inner call's arguments NOW**, on this thread, in the caller's frame: `eval_args(inner.args, argv)` — the same call the ordinary capsule arm makes at `expr.cpp:343-345`.
6. Re-check arity against `info->param_count` here, so the error carries the declaration line's span rather than surfacing from inside `call_capsule` later. (`resolve()` already arity-checks the inner call *statically* — Reader 4 verified `satellite.thread.new(worker(1, 2))` against a 1-parameter worker reports `worker takes 1 argument, got 2` with the caret on the inner call, today. That check is free and it stays.)
7. Build the handle: capsule, name, argv, span, the image keepalive. **Do not start it.** Return `make_value(Value(std::move(handle)))` — the same inline construction `satellite.file.open` uses at `modules.cpp:90` and `construct` uses at `calls.cpp:219`. There is no `make_file()`/`make_object()` helper and `value.hpp` should not gain a `make_thread()`.

**`Evaluator::start_thread` / `join_thread`** — the bodies behind `methods.cpp`'s arm (§2.9), plus `join_all_threads()` (the barrier) and the worker entry point:

```cpp
// on the worker thread, NOT on the spawning thread
Evaluator worker(resolved_, ns_, /*echo=*/false);
worker.set_console(console_);
worker.inherit_division_digits(division_digits_);   // §7.3: NOT max_depth_
worker.adopt_declared(declared_snapshot);
worker.set_thread_registry(threads_);
ValuePtr r = worker.call_capsule(*h.capsule, h.name, h.argv, h.span);
{ std::lock_guard g(h.done_lock); h.result = r; h.errors = worker.take_errors(); }
h.state.store(State::Finished);
```

Note `call_capsule` needs **no signature change** — it already takes `(const CapsuleInfo&, name, argv, span)` and already re-arity-checks for a runtime caller with no static call site (`calls.cpp:75-83`). Its `returned_` handling is also already correct for this: it reads `returned_` back off the Evaluator *before* the `Activation` destructor restores the caller's (`calls.cpp:113-122`), so `.join()` must capture **`call_capsule`'s return value**, never the worker Evaluator's `returned()` afterwards — that is the restored top-level value, which is also what `status_of` uses for the process exit status.

Also fix the bare assignment at `calls.cpp:161` (`current_suit_ = field.owner ? field.owner : &suit;` inside `construct`'s field-initialiser loop) if you ever do Reader 2's Walker split — it is the one register write that is not `Activation`-managed and would be missed.

### 2.9 `src/evaluator/methods.cpp`

Add a `// --- thread ---` block as one more `if (const ThreadPtr *self = std::get_if<ThreadPtr>(recv.get()))`, modelled line-for-line on the `FilePtr` block at `:241-351`, **including its nil guard** reusing the same message (`:243-246`). Place it before the terminal `has no method` at `:393`.

- `.start()` — `arity(0)`. State machine via `compare_exchange` on `state` so a double start from two threads is refused deterministically, not raced.
- `.join()` — `arity(0)`. Returns the capsule's value.
- Consider `.ok()` and `.error()` in `FileHandle`'s shape (`methods.cpp:254`, `:258`) — a joined thread that failed should be inspectable without the join itself being fatal. Optional for v1; if you skip them, say so.

**`.size()` needs no row here.** `methods.cpp:73-83` places it above the per-type tables precisely so "a type added later gets a correct `.size()` from the exhaustive visitor in value.cpp without anyone remembering to add a row here." But note it is unreachable for a thread until `module_of` has `case 9` — see §2.5.

### 2.10 `src/evaluator/help.cpp`

Three tables, none of them generated, none of them tied to the code by a test. `help.cpp:258-266` states the contract this breaks: *"a help text that drifts from the code is worse than none."*

- **`help_for` (`:267-298`)**: add `case 9:` listing `.start() .join() .size()` before the `default: break;` at `:296`. Without it, `satellite.help(my_thread)` returns `"nil has no methods\n"` — a lie, from the language's only discoverability surface.
- **`help_for_module` (`:68-105`)**: add a `thread` arm listing `.new(capsule(args))`. This is not cosmetic: an empty return is how `modules.cpp:127-139` decides a two-segment path is *not a module at all*, so without it `satellite.thread` and `satellite.thread()` both fail.
- **`help_overview` (`:11-56`)**: add `thread` to the scalar list on the `declare` line and a `thread` row beside the `file` row.

### 2.11 `src/evaluator/modules.cpp`

**Do NOT add a `full == "satellite.thread.new"` arm to `call_module`.** This resolves the Reader 1 / Reader 4 disagreement in Reader 4's favour, with a correction. By the time `call_module` runs, `argv` is already evaluated (`expr.cpp:353`) and the capsule has already been invoked — a `call_module` arm would be the bug, not the feature. With the target-path match in §2.7, every reachable spelling is handled there, and `call_module`'s terminal failure at `:731` remains what an unrecognised `satellite.thread.something_else` correctly hits, for free.

The only change here is that `help_for_module("thread")` (§2.10) makes the two-segment listing path at `:127-139` answer.

### 2.12 `src/evaluator/eval.hpp`

- Declare `thread_new`, `start_thread`, `join_thread`, `join_all_threads`, `set_image_keepalive`, `set_thread_registry`.
- New members: `std::shared_ptr<ThreadRegistry> threads_`, `std::shared_ptr<const void> image_`.
- `= delete` the copy constructor, copy assignment, move constructor and move assignment.
- Rewrite the lifetime contract at `:77-80` and `:113-115`: the `ResolveResult` and the `Console` must now outlive **every worker**, not merely the Evaluator. The join barrier is what makes that true.
- Add the sharpened §6 note from §1.3 beside `Frame` (`:46-58`) and `current_` (`:292-295`).
- Note beside `declared_` (`:309-313`) that a worker gets a snapshot and what that means.

### 2.13 `src/interpreter/interp.cpp`

- After constructing each Evaluator (`:231`, `:276`), call `set_image_keepalive(image)`.
- Insert `evaluator.join_all_threads();` after `run()` / `run_entry()` and **before** `console->drain()` (before `:242` and `:287`).
- The retention conditions at `:246-251` and `:293-294` can stay as they are **only because** every handle now holds a strong keepalive on the `Image`. Say that in a comment where the condition is, or the next reader will "simplify" the keepalive away.
- **Unresolved, flagged by Reader 5, and I did not verify it:** `inherit()` (`:95-104`) copies `CapsuleInfo`/`SpacesuitInfo` **by value** into a fresh `ResolveResult`, but `super`, `owner` and `const Capsule *capsule` stay raw pointers into the *older* image (`env.hpp:70, 107, 123, 136`). If sessions are on, a worker running an inherited definition needs the transitive closure of images, not just its own. The per-handle keepalive holds *one* image. **This is a real gap in the plan for the session path.** The safe v1 answer is to keep `retain()`'s existing behaviour (session runs with capsules already retain unconditionally, `:249-251`) and to note that the file-run path is the one the keepalive covers. Do not claim the session path is solved.

### 2.14 `src/programs/main.cpp`

With the barrier inside `run_program`/`run_source`, `run_file_mode` (`:163-184`) and `run_command` (`:190-214`) need no change — the `Console` at `:176` is destroyed after `run_file` returns, and `run_file` now cannot return with workers alive. **Verify this by reading, not by assuming**, and add a comment at `:174-175` (which already explains that the Console is declared before the run so the printer outlives every write) recording that the join barrier is what extends that guarantee to workers.

### 2.15 `Makefile`

- Add `$(EVAL)/threads.cpp` to `EVAL_SRCS` (`:310-315`) — one edit, three lists.
- Test rules and the TSan gotcha: §9.

### 2.16 `src/bytecode_format/format.def` and `format_test.cpp` — **decide, don't half-do**

`format.hpp`/`format.def` are deliberately outside `HDRS` (`Makefile:332-336`) — no TU includes them, there is no VM. So these rows are **specification-only** and cannot affect the tree walker today. But the registry is frozen and append-only, so if you add rows you must move six asserts, and if you skip them you must say why in the commit message. Reader 4 is the only reader who covered this; the details below are Reader 4's, spot-checked only for the tripwire at `format_test.cpp:118`.

If you add them: `SAT_WORD(100, THREAD, "thread")` (one word serving both the type name and the module segment — the flat-registry precedents are `list`/27 reused at `:429-436` and `main`/4 at `:299-303`), `101 NEW`, `102 START`, `103 JOIN`; `SAT_SELECTOR(START)` and `SAT_SELECTOR(JOIN)` but **not** `THREAD`/`NEW`, with a comment saying why, as `:370` does for `empty`; one `SAT_PATH` row for `satellite.thread.new`, recording the arity-vs-`SAT_VARIADIC` reasoning explicitly (`delete`'s row at `:459-464` argues one value is one unit; kind 5 `CAPSULE_ID` at `:95` argues the other way; **this is an unforced choice that is permanent once written**).

Asserts that move: `format_test.cpp:118` (`kWordIds[last] == 99` → 103 — the deliberate tripwire whose comment at `:110-116` names the procedure), `:152` (`!is_defined_word(100)` → 104), `:178` (selector count 42 → 44), `:198` (**the selector band needs a THIRD band 102..103**; the comment at `:190-197` explicitly forbids widening to a single `id <= 86`), `:271`/`:312` if a path row lands.

### 2.17 Design docs — three stale statements, verified

Optional and separable, but each will actively mislead the next implementer:

- **`design/10-evaluator.md:56-72`** claims two Library fixes are outstanding and cites `library.cpp:36-42` and `:50-59`. **Both landed in commit `dacd7aa` (2026-08-07)** — I read the current `library.cpp:40-56` and `:83-91` and they contain the exact `ValuePtr doomed;` shape §10 prescribes, comments and all. The real instance of that bug is now in `slots.cpp` and `mutators.cpp` (§2.1), which §10 does not mention.
- **`design/14-spacesuits.md:115`** — "a read is a lock-free atomic load". Measured false on this toolchain (§1.2 caveat). Soften to "no torn read, no user-visible mutex". Same wording at `library.hpp:17-21`, `value.hpp:132-136`, `slots.cpp:71`.
- **`design/17-the-abstract-machine.md:210`** — "Next free is **80**" against a registry that runs to 99. Nineteen ids stale, and `format.def:22-24` says prose is corrected *from* the data, never the reverse.

---

## 3. Every exhaustive-dispatch site, and which ones actually stop the build

**§10's argument is that the compile errors are the feature. That is true of exactly two sites. Four more will compile clean and be confidently wrong.** Do not plan as if the compiler will find them.

### 3.1 WILL fail to compile (the intended tripwires)

| Site | Lines | Why it stops |
|---|---|---|
| **`ValuePrinter`** | `value.hpp:317-344`, visited `:346-351` | Nine `operator()` overloads, **no generic `auto` fallback**, deliberately. `:317-326` records the crash that rule prevents: a fallback arm compiled clean under `-Wall -Wextra` and then threw `std::get: wrong index for variant` on the first REPL `:get`. |
| **`SizeVisitor`** | `value.cpp:158-242`, visited `:244-250` | Same rule, same reason, stated at `:158-161`: "a fallback arm compiles clean when a tenth alternative lands and then silently bills it as zero bytes." |

Both must be cast to `ValueBase` first (`Value` merely *inherits*, so `std::variant_size` is not specialised for it) — the existing casts at `value.hpp:350` and `value.cpp:249` already do this.

### 3.2 Will NOT fail to compile, and will be wrong

`-Wswitch` never fires because these switch on `size_t`, not an enum. Verified: `CXXFLAGS = -std=c++20 -Wall -Wextra $(OPT)`.

| Site | Lines | Silent failure | Consequence |
|---|---|---|---|
| **`module_of`** | `types.cpp:79-100`, `default: return nullptr` at `:98` | index 9 → "no module" | `my_thread.start()` fails with `nil has no methods, so .start() has no receiver`. Also poisons `stmt.cpp:33-34`'s condition error and the arity/error prefixes at `methods.cpp:57-71`. **And it makes any `ThreadPtr` arm in `methods.cpp` dead code**, because the `if (!module)` bail at `:51-55` runs first. |
| **`help_for`** | `help.cpp:267-298`, `default: break` at `:296` | falls to `"nil has no methods\n"` at `:298` | `satellite.help(my_thread)` lies, breaking the contract stated at `:258-266`. |
| **`matches`** | `types.cpp:14-77` | not index-based at all — a string chain with `return false` at `:45` | `satellite.variable.thread` matches **nothing**, so the declaration with an initialiser is rejected. |
| **`default_of`** | `helpers.cpp:72-101` | unrecognised `variable` name drops through to `std::monostate{}` at `:100` | `satellite.variable.thread t` with no initialiser is **accepted today and binds nil** (verified at runtime by two readers, exit 0, no diagnostic). Meaning a smoke test of the declaration half passes before any work is done. Keeping nil is the right answer (§2.6) — but make it a decision. |
| `Resolver::check_type` | `scopes.cpp:116-160` | validates container names, never variable names | `satellite.variable.thred` resolves clean, defaults to nil, and dies much later at the wrong line. See §6 for why this is **not** in scope. |

### 3.3 Correct by construction — confirm, don't change

- **`Reg::from_value`** (`reg.hpp:132-146`) falls through to `boxed()` for anything it does not name; `to_value` and `to_string` switch on the 5-value `tag` enum and the HEAP arm delegates to `satellite::to_string`, inheriting `ValuePrinter`'s answer. No edit. (`reg.hpp` is not linked into `satl` — there is no VM.)
- **`value_equals`** (`helpers.cpp:112-186`) — the `ValueBase` fallback at `:185` gives `shared_ptr` identity. No arm. §2.6.
- **Map keys** — `map_key_of` (`maps.cpp:45-61`) is an allow-list of number-or-string with `return false` otherwise, and `scopes.cpp:145-152` already refuses `map<satellite.variable.thread, X>` at resolve time. No edit; add a test that asserts it (§8).

---

## 4. How the capture works, exactly

**`satellite.thread.new(worker(1, 2))` must not run `worker`.** Today it does (§0(b)).

**Mechanism — copied from four precedents that all live in the same function.** The rule they share is stated at `expr.cpp:189-193`:

> *"BEFORE eval_args, and that ordering is the whole mechanism. A duration is not a value, so it has nothing to be reduced to… the pace form is therefore matched on the ARGUMENT EXPRESSION, here, while the tree still says `100ms`."*

A capsule call in this position is the same kind of thing: it is **a call to be made later**, not a value, and by the time `argv` exists the call has already happened. **The closest precedent is `satellite.console.input(prompt, target)` (`expr.cpp:249-261` → `modules.cpp:796-836`)**, because there too the argument names something that is not a value — a PLACE — and the handler takes the *node* and turns it into a `Slot` via `slot_of`. `satellite.thread.new` takes the node and turns it into a `const CapsuleInfo *` via `resolved_.find`.

**What is captured:** the `const CapsuleInfo *` (stable — `env.hpp:185` is a node-based map, `run.cpp:88-92` returns `&found->second`), the capsule's name, the `Span`, and **the already-evaluated argument values**.

**What is NOT deferred: the inner call's arguments.** They are evaluated at the spawn site, on the spawning thread, in the caller's frame — by calling the existing `eval_args(inner.args, argv)`, exactly as the ordinary capsule arm does at `expr.cpp:343-345`. Three reasons, all load-bearing:

1. It preserves the rule `expr.cpp:336-342` spells out — arguments are evaluated in the *caller's* frame, before the callee's frame exists. That is the ordering §6 measured as `fact()` returning 1 for every input when it was wrong.
2. Argument expressions may read frame slots of the spawning capsule, which die when that capsule returns (`calls.cpp:85-89`: "a plain local, so one activation costs one C++ stack frame and one vector, and both die with the call"). Deferring the argument expressions would make them read a dead frame.
3. Diagnostics. Reader 4's point: `satellite.thread.new(worker(end=1))` has `node.args[0]` as a `Call`, not a `NamedArg`, so the `NamedArg` loop at `:273-292` never sees it. Evaluating the inner args at the site keeps that error on the spawning thread with a `Span` the run's own `SourceMap` renders. Deferring them would surface it from a worker later.

**A `ValuePtr` is safe to hand across the boundary** — `shared_ptr<const Value>`, atomic refcount, immutable pointee (`value.hpp:24-27`). One exception to document: an `ObjectPtr` in the captured argv means the worker shares *mutable* state with its parent (`value.hpp:128-136`), governed by §14's `write_lock`/atomic-field protocol, not by the value contract.

---

## 5. Hazards, ranked by whether they crash/corrupt or are merely wrong

### Class A — memory unsafety or process death

| # | Hazard | Mitigation |
|---|---|---|
| **A1** | **Worker outlives the `Image`** and walks a freed AST. For a program with capsules and no spacesuit — the normal threaded program — `run_program` releases the only `shared_ptr` at `:293-295`. Every `const Capsule*`, `const Expr*`, every `CapsuleInfo` the Frame is sized from, and the `SourceMap` errors render against die together. | Join barrier (§1.5) **plus** a per-handle `shared_ptr<const void>` keepalive. Belt and braces, because the barrier can be bypassed by A6. |
| **A2** | **Two threads through one `Evaluator`.** `Activation` holds five non-const references to Evaluator members and restores them in its destructor (`calls.cpp:19-61`); thread A's unwind overwrites thread B's `current_`, which then dangles into a dead C++ stack. `slots.cpp:41-47`'s bounds check cannot catch it. | One Evaluator per worker, constructed **on** the worker; `= delete` copy and move. |
| **A3** | **`errors_.push_back` from two threads** (`session.cpp:18-21`) — vector reallocation race, i.e. heap corruption, not a lost message. Polled from everywhere (`stmt.cpp:13`, `calls.cpp:114`, `session.cpp:48`). | Falls out of A2 — per-Evaluator `errors_`, merged under the registry lock at join. |
| **A4** | **`std::terminate` from `~ThreadHandle`** on a joinable thread. Two shapes, both verified by Reader 5 with a standalone probe (exit 134): the frame-local form aborts mid-run; the top-level form (handle in `satellite.library`, a function-local static at `library.cpp:7-11`) aborts **after `main` returned 0**, so a correct program reports a crash. `abort()` skips `~Console`, so everything in `pending_` is destroyed unprinted. | Registry holds a strong `ThreadPtr`, so the handle cannot die before the barrier; `~ThreadHandle` joins defensively; **never `detach()`**. |
| **A5** | **A worker still running during static destruction** — `Library::instance()` and `images()` are both function-local statics with unspecified relative destruction order. Nondeterministic, non-reproducible-under-a-debugger crashes. | The barrier runs inside `run_*`, long before `exit()`. |
| **A6** | **A `shared_ptr` cycle through one `Object`** — handle → argv → `Value(ObjectPtr)` → `fields[i]` → `Value(ThreadPtr)` → handle — suppresses `~ThreadHandle` entirely, so nothing aborts and a live worker reaches `exit()`. Leaks ~8 MB of stack per cycle with no diagnostic. §12 already concedes objects can close cycles. | The registry's strong reference means the **thread** is joined at the barrier regardless of the cycle. The **memory** still leaks — that is §12's acknowledged leak and this plan does not fix it. Say so. |
| **A7** | **`~Value` under `Object::write_lock`** (`slots.cpp:120-132`, `mutators.cpp:112-127`). For a thread-typed field, reassignment runs `join()` while holding the object's mutex *and* libstdc++'s internal `shared_ptr` spinlock. Self-deadlock, not a stall. **Only Reader 3 found this; I verified both sites.** | §2.1 — the `doomed` hoist, already written twice in `library.cpp`. **Do this first, in its own commit.** |
| **A8** | **Depth guard past a worker's cliff.** See §7.3. Under `ulimit -s unlimited` the same recursion raises a clean language error on the main thread and **segfaults** on a worker — the worst asymmetry, because the guard appears to work when tested. | `min(this thread's real stack, RLIMIT_STACK-or-8MB)`, plus constructing the worker Evaluator on the worker. |
| **A9** | **Worker writing to a destroyed `Console`** — a stack local of `run_file_mode` (`main.cpp:176`) held by the Evaluator as a raw `Console *` (`eval.hpp:283`). `drain()` proves the queue is empty, never that writers are done (`console.hpp:44-47`). At the REPL it is worse: `run_command` hands the worker the *prompt's* Console (`main.cpp:203-205`), so leaked output lands in later prompt lines. | Barrier before `drain()`; update `console.hpp:44-47`'s contract to name the join precondition. |

### Class B — wrong answers, no memory unsafety

| # | Hazard | Mitigation |
|---|---|---|
| **B1** | **`output_ += text` shredding** when no `Console` is attached (`session.cpp:31-38`). `satl --run` attaches one (`main.cpp:176-177`), but `run_program(..., nullptr)` does not — **which is the path every test and every embedder uses**. Threads would be line-atomic under the CLI and shredded under test: the worst split, because the tests are where it would be caught. | Per-Evaluator `output_`, merged at join under the registry lock. **This changes ordering semantics**: with a Console, output interleaves line-by-line in real time; without one, a worker's output appears as a block at join. **Document that difference — do not hide it.** Thread output ordering is only defined when a Console is attached. |
| **B2** | **`declared_` empty on the worker** → global assignment type checks silently vanish (`stmt.cpp:125-131`, `modules.cpp:823-829` become dead code). | Snapshot at spawn (§1.4), with the "sees declarations that existed at spawn" semantics written down. |
| **B3** | **`module_of` returns null for index 9** → `.start()` says `nil has no methods`, and the `methods.cpp` arm is dead code. | `case 9` (§2.5), landing **before or with** the `methods.cpp` arm. |
| **B4** | **`help_for` default** → `satellite.help(my_thread)` lies. | `case 9` in `help.cpp` (§2.10). |
| **B5** | **`division_digits_` re-read from the Library on the worker** — a program that changes the knob between spawn and start gets different division precision on the two threads for the same source: silently non-deterministic on thread timing. | **Inherit `division_digits_` from the parent; recompute `max_depth_` on the worker.** These go opposite ways and it is counterintuitive — comment it. |
| **B6** | **Two threads in `satellite.console.input`** race on `std::getline(std::cin, …)` and both call `console_->drain()`, each blocking on the other's queue (`modules.cpp:749-757`). | **Refuse.** See §6. |
| **B7** | **`display(x, end="")` is not line-atomic.** §9's guarantee is per-`write()`, not per-line (`console.hpp:52-58`). `modules.cpp:872` emits whatever the program built; the whole-line form at `:711` is safe. | Document as a language-level caveat. Not a bug — `modules.cpp:764` already gets it right for the prompt path. |
| **B8** | **`satellite.variable.thread t` with no initialiser binds nil and always will.** | Deliberate, following `file`. Comment it in `default_of` so it reads as a decision. |

### Class C — discoverability and drift

- **C1** `help_for_module` and `help_overview` are hand-written with no test tying them to the code. `helpers.cpp` claims the overview "is checked against the code below rather than written once and left to rot" — that is a comment, not a check.
- **C2** The three stale design statements (§2.17). `design/10` is the dangerous one: it will send a reader to fix something already fixed and away from `slots.cpp`/`mutators.cpp`, where the bug actually is now.
- **C3** `design/17:210`'s "Next free is 80" against a registry at 99.

---

## 6. What to REFUSE rather than build

This language refuses rather than clamps — `§18`'s `MAX_RANDOM_DIGITS`, `§8.6`'s missing-key rule, `format.def`'s new `delete` word instead of reusing `remove`, a number refusing `.length()`. Apply the same habit here.

**In the surface:**

1. **No `.detach()`.** It is what would make every Class-A hazard live again, and it makes `satl --run`'s promise that its output is complete un-keepable.
2. **`.start()` twice** → *"this thread has already started"*. Not a silent no-op, not a restart. Enforce with `compare_exchange` so a double start from two threads refuses deterministically.
3. **`.join()` before `.start()`** → *"this thread has not started"*. Not a silent success.
4. **`.join()` twice** → *"this thread has already been joined"*. The alternative — caching the result and returning it again — is defensible and kinder; I recommend refusing because a second join is nearly always a bug, but **this is a real choice and should be recorded either way** rather than falling out of whichever is easier to write.
5. **`satellite.console.input` from a worker** → refuse. Two threads in `std::getline` is a line delivered to the wrong reader, with no error anywhere. The check is cheap: the worker Evaluator knows it is a worker.
6. **A thread as a map key** — already refused by the allow-list (`maps.cpp:45-61`) and at resolve time (`scopes.cpp:145-152`). No code; **add a test that pins it**, because it is currently correct by accident of the allow-list's shape.
7. **Four distinct refusals for a bad `satellite.thread.new` argument** — non-call, bare capsule name, spacesuit construction, method, variable — each with its own message, mirroring `eval_call:296-333`. Not one generic "bad argument".
8. **`system.max_threads`, refused rather than queued.** A program that spawns unbounded threads costs ~8 MB of address space each (measured), and the memory watchdog fires at `min_free_mb = 4096` (`main.cpp:157`) and ends the process with `_exit(2)` at `system.cpp:360` — **skipping the Console drain entirely**, so the program's queued output vanishes and in `satl-term` the window and its whole scrollback go with it (§9's still-outstanding bug at `design/09:26-30`). Give it a `satellite.library.system.max_threads` knob with the same shape as `max_depth` — a default and a derived ceiling — and **refuse `.start()`** past it with a message naming the knob. Do not block, do not queue, do not clamp.

**In the implementation:**

9. **Do not relax `sizeof(Value) == 40`.** If it fires, someone put the handle inline; the fix is the `shared_ptr`. `value.hpp:258-264` puts a number on what relaxing it costs: a million-element number list goes from 38 MB to 91 MB.
10. **Do not add `satellite.thread.new` to `call_module`.** §2.11.
11. **Do not add a variable-type-name allow-list to `Resolver::check_type` in this work.** It is tempting — it would catch `satellite.variable.thred` — but `satellite.variable.zzz x` runs clean today (verified at runtime by two readers), so adding the check newly rejects programs that currently work. `scopes.cpp:110-115` records exactly this discipline for the container half. **Separate commit, separate note about what it newly rejects.** Readers 1 and 4 disagreed here; Reader 4 is right about scope.
12. **No mutex, lock, channel, or thread-local in v1.** `satellite.library` is already the safe shared-write mechanism §6 and §7 built and measured. Adding a second synchronisation surface before anyone has used the first one is speculative.

---

## 7. Three things that are wrong today and get worse with threads

### 7.1 `~Value` under `Object::write_lock`
§2.1 / A7. Fix first, own commit, no thread code required to justify it.

### 7.2 The Console's concurrency test has no sanitizer coverage
`§9` claims "It is clean under ThreadSanitizer" (`design/09:188-190`), but `console_test` is built with plain `$(CXX)` and `$(OPT)` (`Makefile:691-692`); `library_test_tsan` is the **only** sanitized binary (`Makefile:171`). The Console is the piece every worker touches on every display. Adding `console_test` to the TSan set is cheap and in scope.

### 7.3 The depth guard, on a worker's stack

`max_max_depth()` (`helpers.cpp:680-697`) derives the ceiling from `getrlimit(RLIMIT_STACK)` — a **process** limit describing the **main thread**. `RLIM_INFINITY` and a `getrlimit` failure both collapse to `STACK_LIMIT_UNKNOWN`, read as a flat 8 MB (`:682-684`, `system.cpp:256-263`).

**Reader 5's measurement contradicts the usual assumption and is the more precise finding:** glibc gives a new `std::thread` a stack of exactly `RLIMIT_STACK` when that limit is *finite*, so the ceiling is accidentally correct at 1 MB, 8 MB, 64 MB and 256 MB. **Exactly one row is wrong: `ulimit -s unlimited`**, where the worker gets glibc's 2 MB default while `read_max_depth()` hands back `min(DEFAULT_MAX_DEPTH=2000, 3000) = 2000`.

**Where the readers disagree:** Reader 2 says the guard sits "~4x past the real cliff"; Reader 5 says 2.7x. Neither is wrong — Reader 5 divides the 2 MB by the conservative constant `CEILING_BYTES_PER_UNIT = 2796` to get 750 units; Reader 2 divides by the *measured* -O2 cost of ~3169 bytes per 3-unit activation (~1056 bytes/unit) to get ~666 activations ≈ 2 MB, i.e. right at the edge at -O2 and roughly 2x past at -O0. **The multiplier does not need to be resolved to apply the fix**, and the direction is the same in both accounts. Say this in the commit message rather than picking one number.

**Fix, using a function that already exists in the tree and is already wired to the language:**

```
bytes = min( thread_stack_bytes(this thread).total ,
             stack_limit_bytes() == STACK_LIMIT_UNKNOWN ? 8MB : stack_limit_bytes() )
```

`thread_stack_bytes()` is `system.cpp:135-161` (`pthread_getattr_np(pthread_self())` / `pthread_attr_getstack`), already consumed by `satellite.system.memory.this.available()` at `modules.cpp:230-244`.

**The `min()` is not optional.** Reader 5 measured `pthread_getattr_np` on the **main** thread under `ulimit -s unlimited` reporting a ~44 TB region; using it unguarded would set the main ceiling to billions of units — the exact cliff overshoot the current code exists to avoid. The `min()` reproduces today's answer on every finite row (3000 / 24001 / 96006 / 375) and changes only the broken one (2000 → 750 on the worker).

Also update the comment at `helpers.cpp:673-676`. Its "unlimited is not unbounded — the main thread's stack still stops where the next mapping begins" argument is correct **and does not transfer** to a worker's fixed-size mmap. The comment being right about the main thread is why nobody noticed.

---

## 8. `thread_test` — what it must prove

New file `src/evaluator/thread_test.cpp`. Unlike `library_test.cpp`, it **constructs real Evaluators and runs satellite source** — that is the coverage gap `eval.hpp:186-189` and `design/07:57-60` both name outright ("library_test drives the Library directly from C++ and never constructs an evaluator, so the field arm has no ThreadSanitizer coverage").

**Semantics — the ones the feature is:**

1. **`satellite.thread.new(worker(1,2))` does not run `worker`.** Have `worker` set a `satellite.library` variable; assert it is unset immediately after the declaration line and set only after `.join()`. **This is the regression the entire design turns on** (§0(b)) and it must be the first test in the file.
2. **Arguments are evaluated at the spawn site**: an argument expression reading a frame local of the spawning capsule sees the value **at spawn time**, not at start time. Mutate the local between `new` and `start` and assert the worker saw the old one.
3. **`.join()` returns the capsule's `satellite.return` value** — not the restored top-level `returned_`.
4. **A worker's error surfaces at the joining statement**, with the worker's message and a span that renders against the run's `SourceMap`.
5. **A worker error on a never-joined thread still reaches the run's report** via the barrier.
6. **A program that starts a thread and never joins it exits 0 with all output present** — the barrier's whole job.

**Concurrency — the properties `library_test` and `console_test` prove one level down:**

7. **N workers incrementing one `satellite.library` variable through a mutator lose no increments.** The eval-level analogue of `library_test.cpp:144-151`.
8. **N workers mutating one spacesuit field concurrently lose nothing** — the field arm (`slots.cpp:120-132`, `mutators.cpp:112-127`) that has no TSan coverage today.
9. **Every line from every worker arrives whole through one Console**, in the `console_test.cpp:109-160` shape (8 × 250; all lines arrive, none torn, each thread's own lines stay in relative order) — but driven from satellite source through `satellite.console.display`.
10. **Recursion on a worker raises the language stack-depth error, not a segfault** — and this test must be run **explicitly under `ulimit -s unlimited`**, because that is the one broken row (§7.3). A test run at the default 8 MB proves nothing here.
11. **A thread stored in a spacesuit field, then reassigned, does not deadlock** — the A7 fix.
12. **`.size()` on a running thread returns without blocking**, and `to_string` of a running thread (printed from the main thread while the worker runs) returns without blocking.

**Refusals — one test each:** double start; join before start; double join; `satellite.thread.new` of a method, of a construction, of a variable, of a bare name, of a non-call, with wrong arity; `satellite.console.input` from a worker; a thread as a map key; `system.max_threads` exceeded.

**Note on the harness:** the test must call `set_console()` on every Evaluator it builds, or `emit()` falls to the unsynchronised `output_ +=` at `session.cpp:36` — which means a thread test written naively against `check_output` would run on precisely the unsafe path. **And** one test should deliberately exercise the *no*-Console path, to prove the per-Evaluator `output_` merge (B1) works and to pin the ordering semantics it produces.

---

## 9. Building it under ThreadSanitizer

Copy `Makefile:645-647` verbatim, changing only the names:

```make
$(EVAL)/thread_test_tsan: $(EVAL)/thread_test.cpp $(TESTSRCS) $(HDRS)
	$(TSAN_CXX) $(TESTFLAGS) $(PCGFLAGS) -I$(SRC) -O1 -g -fsanitize=thread \
	    -o $@ $(EVAL)/thread_test.cpp $(TESTSRCS)
```

Why this needs **no new sources**: `TESTSRCS` (`Makefile:347-351`) already contains `$(EVAL_SRCS)` plus the parser, env, loader and interp — the entire interpreter. Adding `threads.cpp` to `EVAL_SRCS` (§2.8) puts it here automatically. It compiles from source rather than from `$(LIBOBJS)` because `-fsanitize=thread` has to be on **every** translation unit it links (`Makefile:361-363`), and it uses `-O1 -g` rather than `$(OPT)`.

`TSAN_CXX` is probed by actually *linking* an empty `main` with `-fsanitize=thread` (`Makefile:217-222`), because `$(CXX)` is not guaranteed to ship a TSan runtime — the clang-24 build here is built without compiler-rt.

**THE GOTCHA — verified, and it silently defeats the whole test.** `Makefile:171` and `:708-710`:

```make
TSAN_TEST  = $(if $(filter-out 0,$(TSAN)),$(LIBRARY)/library_test_tsan)
...
test: $(TESTBINS) $(TSAN_TEST)
	./$(LIBRARY)/library_test
	$(if $(TSAN_TEST),./$(TSAN_TEST))
```

The obvious wiring — appending `thread_test_tsan` to `TSAN_TEST` — expands line 710 to `./first second`, making the second binary an **argv to the first**. It is never run, and `make test` passes. **The worst possible failure mode for a concurrency test.** Either:

- rewrite `:710` as `for t in $(TSAN_TEST); do ./$$t || exit 1; done`, or
- give `thread_test_tsan` its own explicit `./$(EVAL)/thread_test_tsan` run line.

Also required:
- add `$(EVAL)/thread_test` to `TESTBINS` (`:701-706`) and a run line in `test:` for the non-sanitized build;
- add both aliases in the block at `:749`;
- name `thread_test_tsan` **outright** in `clean` (`:1069`), not through `$(TSAN_TEST)` — for the same `TSAN=0` reason the comment at `:1067-1068` gives for `library_test_tsan`.

`TESTFLAGS` already carries `-pthread` (`:352`), so nothing is needed there.

**Also add `console_test` to the TSan set** (§7.2) — it is the same one-rule copy and it closes the gap where `§9`'s cleanliness claim is currently a one-off manual result.

**Baseline:** Reader 3 ran the existing `./src/satellite_library/library_test_tsan` on this machine and it passes clean (`PASS: 160000 increments intact, 434134 lock-free reads, 165 variables`, exit 0). So a TSan failure after this work is a real regression, not pre-existing noise.

---

## 10. Suggested commit order

1. **`doomed` hoist in `slots.cpp` + `mutators.cpp`** (A7). Self-contained pre-existing bug fix; no thread code needed to justify it.
2. **Depth guard `min()` fix in `helpers.cpp`** (A8/§7.3), with the `ulimit -s unlimited` measurement in the message and the corrected comment.
3. **`console_test` under TSan** (§7.2).
4. **The variant + the two compile-error arms**: `thread_handle.hpp`, `value.hpp` index 9, `value.cpp` printer + size arms. Builds and does nothing.
5. **`module_of` case 9, `matches`, `default_of`, `help_for`, `help_for_module`, `help_overview`** — the four dispatch tables. `satellite.variable.thread t` now declares, prints and helps correctly, still nil.
6. **The registry, the barrier, `set_image_keepalive`, the deleted copy/move, the `eval.hpp` contract rewrite, `interp.cpp` wiring.** No user-visible change; the machinery threads will need.
7. **`threads.cpp` + the `expr.cpp` interception + `methods.cpp` arm.** The feature lands here, with all refusals.
8. **`thread_test.cpp` + Makefile wiring** (including the `TSAN_TEST` loop fix). Could reasonably be folded into 7.
9. **`format.def` rows + the six `format_test` asserts** — or an explicit note saying they are deferred and why.
10. **Design doc corrections** (§2.17).

---

## 11. Open questions and things I did not verify

Stated plainly so nobody mistakes them for settled:

- **The session path's transitive lifetime (§2.13).** `inherit()` copies `CapsuleInfo`/`SpacesuitInfo` by value but their `super`/`owner`/`capsule` pointers still point into *older* retained images. A per-handle keepalive holds one image. **This plan does not solve threads-plus-sessions**; it solves threads on the file-run path and relies on the session path's existing unconditional retention. Reader 5 raised it; I did not verify `inherit()` myself.
- **Runtime observations I took from readers and did not re-run:** that `satellite.thread.new(worker())` runs `worker` today; that `satellite.variable.thread t` exits 0 silently; that `satellite.variable.zzz x` runs clean; that a nil-valued declared thread already gives `nil has no methods, so .start() has no receiver`; the `pthread_attr_getstack` measurements; the `std::terminate` probes; the `atomic<shared_ptr>` lock-free probe; the `library_test_tsan` baseline. Each was reported with a concrete transcript and they are mutually consistent, but they are second-hand here.
- **`.join()` twice** — refuse vs. return the cached result. I recommend refuse; it is a genuine choice.
- **`P_THREAD_NEW`'s arity** — `1` (one value is one unit, per `delete`'s row at `format.def:459-464`) vs `SAT_VARIADIC` (a capsule id plus N argument registers, per kind 5 at `:95`). Permanent once written. `format.def` records the P_HELP wrong-arity episode as a known gap rather than a silent fix; do the same here.
- **`system.max_threads`' default and whether its ceiling should be derived** (from `RLIMIT_NPROC`, or from available memory the way `max_depth` derives from `RLIMIT_STACK`). I recommend the refusal exists; the number is not settled.
- **Reader 2's `Walker` split** is a better long-term factoring than one-Evaluator-per-worker. I recommend deferring it. If the per-Evaluator duplication of `declared_` and `output_` turns out to hurt, that is the refactor to reach for, and `calls.cpp:161`'s bare `current_suit_ =` is the write that a mechanical split will miss.