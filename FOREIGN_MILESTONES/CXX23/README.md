# C++23 → satellite, feature by feature

**The standard this folder is written against:**

> **C++23 — ISO/IEC 14882:2024.** Published October 2024. The two names are the
> same document: the committee calls it C++23 for the year of feature freeze,
> ISO numbers it for the year of publication. There is no "C++24" as a separate
> standard, and a file here saying `(C++23)` and one saying `14882:2024` mean
> the same thing.

**C++26 is not covered and is marked where it intrudes.** It is in flight at the
time of writing and its feature set is not final, so a row that mentions it says
*(C++26 draft)* and makes no promise. Reflection and pattern matching are the two
most likely to change what this folder should say, and neither is settled.

**Written 2026-09-12 against knowledge current to May 2026.** Anything voted in
after that is missing rather than excluded — if a row seems absent, check the
standard before assuming a decision was made about it.

The goal of this folder is **total coverage**: every feature a C++23 programmer
might type into satellite has a file here saying what happens.

**One file per feature.** `M1.md` is one feature, not one milestone's worth of
work. The number is a catalogue position and nothing else — it is not a
satellite milestone, does not appear in `words.def`, and may be assigned out of
order. Where a feature needs work in satellite, the file names the **satellite**
milestone (`../SATELLITE/M28-...`), and those numbers are real.

**Every row ends in one of three answers**, which is
[ERROR_HANDLING.md](../../ERROR_HANDLING.md) §9's rule:

| | meaning |
|---|---|
| **says it** | satellite has this today; the file gives the line to type |
| **M<n>** | satellite cannot yet; that satellite milestone brings it |
| **never** | satellite will not have this, and the file says why |

---

## The catalogue

### Getting a program to run

| # | C++ feature | satellite | file |
|---|---|---|---|
| 1 | `int main()` | says it | [M1](M1.md) |
| 2 | `#include`, headers | says it | [M2](M2.md) |
| 3 | `std::cout` / `std::print` (C++23) | says it | [M3](M3.md) |
| 4 | namespaces | says it — every path is dotted | [M4](M4.md) |

### Values and types

| # | C++ feature | satellite | file |
|---|---|---|---|
| 5 | fundamental types (`int`, `double`, `bool`) | says it | [M5](M5.md) |
| 6 | `std::string` | says it | [M6](M6.md) |
| 7 | `auto` | never — see file | [M7](M7.md) |
| 8 | `const` / `constexpr` / `consteval` | never (const), M34 (constexpr) | [M8](M8.md) |
| 9 | references `&`, pointers `*` | says it — a spacesuit IS a reference | [M9](M9.md) |
| 10 | `std::optional` | says it — `satellite.variable.variant` | [M10](M10.md) |
| 11 | `std::variant` | says it — `satellite.variable.variant` | [M11](M11.md) |
| 12 | `std::expected` (C++23) | M33 | [M12](M12.md) |
| 13 | structured bindings | M35 | [M13](M13.md) |

### Containers

| # | C++ feature | satellite | file |
|---|---|---|---|
| 14 | `std::vector` | says it — `satellite.container.list` | [M14](M14.md) |
| 15 | `std::map` / `unordered_map` | says it — `satellite.container.map` | [M15](M15.md) |
| 16 | iterators, `begin()`/`end()` | never — index instead | [M16](M16.md) |
| 17 | ranges and views (C++20) | M36 | [M17](M17.md) |

### Control flow

| # | C++ feature | satellite | file |
|---|---|---|---|
| 18 | `if`, `while`, `for` | says it | [M18](M18.md) |
| 19 | `break`, `continue` | [M27](../SATELLITE/M27-break-and-continue.md) | [M19](M19.md) |
| 20 | `&&`, `\|\|`, `!` | [M28](../SATELLITE/M28-logical-operators.md) | [M20](M20.md) |
| 21 | `switch` | M30 | [M21](M21.md) |
| 22 | ternary `?:` | M37 | [M22](M22.md) |
| 23 | `goto` | never | [M23](M23.md) |
| 24 | `if constexpr` | M34 | [M24](M24.md) |

### Objects

| # | C++ feature | satellite | file |
|---|---|---|---|
| 25 | `class` / `struct`, members | says it — `satellite.spacesuit` | [M25](M25.md) |
| 26 | access control | says it — `protected` / `public` blocks | [M26](M26.md) |
| 27 | constructors / destructors | partial — see file | [M27](M27.md) |
| 28 | inheritance, `virtual` | partial — a suit holds a copy of its super | [M28](M28.md) |
| 29 | `new` / `delete`, RAII | never — refcounted, no manual allocation | [M29](M29.md) |
| 30 | smart pointers | never — a spacesuit already is one | [M30](M30.md) |
| 31 | move semantics, `&&`, `std::move` | never — see file | [M31](M31.md) |
| 32 | operator overloading | M38 | [M32](M32.md) |
| 33 | `<=>` (C++20) | M38 | [M33](M33.md) |
| 34 | "deducing this" (C++23) | never — there is no `this` | [M34](M34.md) |

### Generic code

| # | C++ feature | satellite | file |
|---|---|---|---|
| 35 | `template<typename T>` | [M31](../SATELLITE/) — user generics | [M35](M35.md) |
| 36 | concepts (C++20) | M31, after generics | [M36](M36.md) |
| 37 | parameter packs / fold expressions | M31 | [M37](M37.md) |

### Functions

| # | C++ feature | satellite | file |
|---|---|---|---|
| 38 | free functions | says it — `satellite.capsule` | [M38](M38.md) |
| 39 | overloading | M39 | [M39](M39.md) |
| 40 | default arguments | M39 | [M40](M40.md) |
| 41 | lambdas / closures | M32 | [M41](M41.md) |
| 42 | `std::function`, callbacks | M32 | [M42](M42.md) |

### Errors

| # | C++ feature | satellite | file |
|---|---|---|---|
| 43 | `try` / `catch` / `throw` | M33 | [M43](M43.md) |
| 44 | `noexcept` | never | [M44](M44.md) |

### Concurrency

| # | C++ feature | satellite | file |
|---|---|---|---|
| 45 | `std::thread`, `join()` | says it — M23 built it | [M45](M45.md) |
| 46 | `std::mutex`, `lock_guard` | M40 — **nothing locks today** | [M46](M46.md) |
| 47 | `std::atomic` | M40 | [M47](M47.md) |
| 48 | `std::async` / futures | M40 | [M48](M48.md) |
| 49 | coroutines (C++20) | never | [M49](M49.md) |

### The rest of the standard library

| # | C++ feature | satellite | file |
|---|---|---|---|
| 50 | `std::format` (C++20) | never — `+` builds the line | [M50](M50.md) |
| 51 | file streams | says it — `satellite.file` | [M51](M51.md) |
| 52 | `<chrono>` | partial — `satellite.time` | [M52](M52.md) |
| 53 | `<random>` | says it — `satellite.random` | [M53](M53.md) |
| 54 | `<algorithm>` (sort, find) | partial — list has its own | [M54](M54.md) |
| 55 | modules (C++20) | never — one include | [M55](M55.md) |
| 56 | inline assembly | never | [M56](M56.md) |

---

## Satellite milestones this folder asks for

Counted here so the total is visible in one place. Each is defined in
[../SATELLITE/](../SATELLITE/).

| milestone | brings | asked for by |
|---|---|---|
| M27 | `break`, `continue` | M19 |
| M28 | `&&`, `\|\|`, `not` | M20 |
| M29 | method chaining | everywhere |
| M30 | `switch` / `match` | M21 |
| M31 | user generics | M35, M36, M37 |
| M32 | a capsule as a value | M41, M42 |
| M33 | catching a refusal | M12, M43 |
| M34 | `constexpr` | M8, M24 |
| M35 | structured bindings | M13 |
| M36 | ranges | M17 |
| M37 | ternary | M22 |
| M38 | operator overloading | M32, M33 |
| M39 | overloading, default arguments | M39, M40 |
| M40 | mutexes, atomics, futures | M46, M47, M48 |

**M40 is the one to look at first**, and not because C++ programmers ask for it:
there is no mutex and no atomic anywhere under `satellite_containers/`, so two
threads appending to one list is a corrupted list and not a slow one, and
nothing in the language says so. That is a correctness gap in a language that
already ships threads, independent of anything in this folder.
