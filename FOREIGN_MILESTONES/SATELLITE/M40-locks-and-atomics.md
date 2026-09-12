# M40 — mutexes, atomics, and telling a program it raced

**Named by** `std::mutex`, `std::atomic`, `std::lock_guard` (C++), `synchronized`
(Java), `Arc<Mutex<T>>` (Rust), `threading.Lock` (Python).

**This is the most serious milestone in this folder**, and unlike the others it
is not about convenience. Satellite ships threads today with no protection and
no diagnostic.

## What is true right now

Verified 2026-09-12: **no `mutex`, no `atomic`, no `lock_guard` anywhere under
`src/satellite_containers/`**, and refcounting is not atomic either. So:

- two threads appending to one list is a **corrupted list, not a slow one**
- two threads sharing any spacesuit race on its refcount
- `satellite.library.n = satellite.library.n + 1` is read-add-write, which
  `example/threads.satl` §4 states plainly is not atomic

M23 was honest about this — "a data race in the PROGRAM and not in the language"
— but it is said in a milestone document, not where somebody writes the bug.

## What satellite makes you write instead

**Share nothing.** `infinity_data_main.satl` gives each of its 23 workers a
private `earth_shard` and shares exactly one object, an `infinity_data` the
threads only read. That discipline works and the file comments it at the site,
but the language cannot check it and a reader arriving from Rust will assume it
does not have to.

## What it would cost to build

Three options, cheapest first, and the third is the most satellite-like:

1. **`satellite.variable.mutex`** with `lock()` / `unlock()`. Familiar, and puts
   the burden back on the programmer C++-style.
2. **Atomic refcounts**, which is not optional if spacesuits may cross threads
   at all — the cost falls on every single-threaded program, so measure first.
3. **Make the containers refuse.** A list that notices it is reachable from two
   threads and STOPS matches DESIGN §9.1's "report and answer false" far better
   than a lock somebody must remember to take. A data race becomes a refusal
   with a line number instead of a corrupted answer.

## What it must not break

Single-threaded speed. §7.2's globals already measured lock-free reads against
locked ones and found them indistinguishable (3,000,000 reads, 1.19–1.25 s
either way) — that measurement is the template, and any option here owes the
same one before it ships.
