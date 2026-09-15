# Rust 1.8x → satellite, feature by feature

**The standard this folder is written against:** **Rust 2021 edition**, stable
channel as of early 2026. Rust has no ISO standard; the reference is the Rust
Reference for that edition.

**Written 2026-09-12 against knowledge current to May 2026.**

Rust is the closest of the five in spirit — both languages are explicit, both
refuse rather than guess — and the furthest in mechanism, because satellite has
no ownership model at all.

## The catalogue

| # | Rust | satellite | answer |
|---|---|---|---|
| 1 | `println!("{}", x)` | `satellite.console.display(x)` | says it |
| 2 | `fn f()` | `satellite.capsule f()` | says it |
| 3 | `fn main()` | `satellite.capsule satellite.main(...)` | says it |
| 4 | `struct S` + `impl S` | one `satellite.spacesuit S()` | says it |
| 5 | `let` / `let mut` | `satellite.variable.number x = 0` | says it |
| 6 | `Vec<T>` | `satellite.container.list<T>` | says it |
| 7 | `HashMap<K,V>` | `satellite.container.map<K,V>` | says it |
| 8 | `use std::...` | `satellite.include(satellite)` | says it |
| 9 | `std::thread::spawn` / `join` | `satellite.thread.new` / `.start()` / `.join()` | says it |
| 10 | `Option<T>` | `satellite.variable.variant`, `.holding()` | says it |
| 11 | `Result<T, E>` / `?` | — | [M33](../SATELLITE/M33-catching-a-refusal.md) |
| 12 | `match` | — | [M30](../SATELLITE/M30-switch-and-match.md) |
| 13 | closures `\|x\|` | — | [M32](../SATELLITE/M32-capsules-as-values.md) |
| 14 | generics `fn f<T>` | — | [M31](../SATELLITE/M31-user-generics.md) |
| 15 | traits | — | M31, then a decision |
| 16 | `&` / `&mut` / borrow checker | — | never: a spacesuit is a reference, and nothing is checked |
| 17 | lifetimes `'a` | — | never: refcounting decides lifetime at run time |
| 18 | `Arc` / `Mutex` | — | M40, and **nothing locks today** |
| 19 | iterators / `.map().filter()` | — | M29 for the chaining, M32 for the closures |
| 20 | `unsafe` | — | never |

**The one to read first is 18.** Rust's whole promise is that data races are a
compile error. Satellite ships threads and has **no mutex, no atomic and no
checking** — a Rust reader will assume a guarantee that is not there. See
[../CXX23/M46.md](../CXX23/M46.md), which says it in full.
