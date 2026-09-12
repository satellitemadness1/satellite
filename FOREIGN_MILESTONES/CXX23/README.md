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

**Rows 1–56 were the first pass; rows 57–318 were added on 2026-09-12** to take
the catalogue from the headline features to the whole language and standard
library, back to C++98. The new rows carry a *since* column naming the standard
that introduced the feature. Every satellite fact in them was checked that day
against `satl --words`, `satellite.help`, or a probe program run through `satl`,
and the page says so where the answer surprised.

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
| **partial** | satellite has part of it, and the file says which part |
| **open** | no milestone and no refusal yet: **a decision the author has not taken** |

**`open` is new with rows 57–318 and is not a fourth answer a message may give.**
It marks a row the catalogue could not answer honestly without a decision.
Inventing a milestone number would break [../README.md](../README.md)'s promise
rule, and saying *never* would be deciding for the author. Every open row is
listed at the end of this file, and each should leave that list as one of the
three real answers.

---

## The catalogue

### Getting a program to run

| # | C++ feature | satellite | file |
|---|---|---|---|
| 1 | `int main()` | says it | [M1](M1.md) |
| 2 | `#include`, headers | says it | [M2](M2.md) |
| 3 | `std::cout` / `std::print` (C++23) | says it | [M3](M3.md) |
| 4 | namespaces | partial — the language's paths are dotted; a program cannot declare its own namespace yet (open, see [M68](M68.md)) | [M4](M4.md) |

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
| 20 | `&&`, `\|\|` | [M28](../SATELLITE/M28-logical-operators.md) — `!` already works, see [M131](M131.md) | [M20](M20.md) |
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

### The preprocessor and program structure

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 57 | `#define` for a constant | C++98 | says it | [M57](M57.md) |
| 58 | function-like macros | C++98 | never | [M58](M58.md) |
| 59 | `#if`, `#ifdef`, conditional compilation | C++98 | never | [M59](M59.md) |
| 60 | include guards, `#pragma once` | C++98 | never | [M60](M60.md) |
| 61 | `__FILE__`, `__LINE__`, `std::source_location` | C++98 / C++20 | open | [M61](M61.md) |
| 62 | `static_assert` | C++11 | [M34](../SATELLITE/M34-compile-time-evaluation.md) | [M62](M62.md) |
| 63 | `__has_include` | C++17 | never | [M63](M63.md) |
| 64 | `#error` and `#warning` | C++98 / C++23 | never | [M64](M64.md) |
| 65 | `extern "C"` and linkage | C++98 | never | [M65](M65.md) |
| 66 | translation units and the one-definition rule | C++98 | partial | [M66](M66.md) |
| 67 | `inline` functions and `inline` variables | C++98 / C++17 | never | [M67](M67.md) |
| 68 | `using namespace`, using-declarations | C++98 | open | [M68](M68.md) |
| 69 | namespace aliases | C++98 | open | [M69](M69.md) |
| 70 | anonymous namespaces, `static` internal linkage | C++98 | open | [M70](M70.md) |
| 71 | inline namespaces | C++11 | open | [M71](M71.md) |
| 72 | nested namespace definitions `a::b::c` | C++17 | open | [M72](M72.md) |

### Attributes

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 73 | `[[nodiscard]]` | C++17 | open | [M73](M73.md) |
| 74 | `[[noreturn]]` | C++11 | never | [M74](M74.md) |
| 75 | `[[deprecated]]` | C++14 | never | [M75](M75.md) |
| 76 | `[[fallthrough]]` | C++17 | never | [M76](M76.md) |
| 77 | `[[maybe_unused]]` | C++17 | never | [M77](M77.md) |
| 78 | `[[likely]]`, `[[unlikely]]` | C++20 | never | [M78](M78.md) |
| 79 | `[[no_unique_address]]` | C++20 | never | [M79](M79.md) |
| 80 | `[[assume]]` | C++23 | never | [M80](M80.md) |

### Types and literals

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 81 | `char` and character literals `'a'` | C++98 | partial | [M81](M81.md) |
| 82 | `wchar_t`, `char16_t`, `char32_t` | C++98 / C++11 | [M45](../CXX26/M45.md) | [M82](M82.md) |
| 83 | `char8_t` and `u8""` literals | C++20 | [M45](../CXX26/M45.md) | [M83](M83.md) |
| 84 | the integer types: `short`, `int`, `long`, `unsigned` | C++98 | says it | [M84](M84.md) |
| 85 | `long long` and fixed-width `<cstdint>` types | C++11 | never | [M85](M85.md) |
| 86 | `float`, `double`, `long double` | C++98 | partial | [M86](M86.md) |
| 87 | integer overflow and wraparound | C++98 | never | [M87](M87.md) |
| 88 | hex `0xFF` and binary `0b1010` literals | C++98 / C++14 | says it | [M88](M88.md) |
| 89 | digit separators `1'000'000` | C++14 | open | [M89](M89.md) |
| 90 | raw string literals `R"(...)"` | C++11 | open | [M90](M90.md) |
| 91 | escape sequences `\n`, `\t`, `\"` | C++98 | says it | [M91](M91.md) |
| 92 | user-defined literals `10_km`, standard literals `"s"s`, `1s` | C++11 / C++14 | never | [M92](M92.md) |
| 93 | `bool`, `true`, `false` | C++98 | says it | [M93](M93.md) |
| 94 | `enum` | C++98 | open | [M94](M94.md) |
| 95 | `enum class` | C++11 | open | [M95](M95.md) |
| 96 | `using enum`, `std::to_underlying` | C++20 / C++23 | open | [M96](M96.md) |
| 97 | `typedef` and `using` type aliases | C++98 / C++11 | open | [M97](M97.md) |
| 98 | `union` | C++98 | never | [M98](M98.md) |
| 99 | bit-fields | C++98 | never | [M99](M99.md) |
| 100 | `sizeof` | C++98 | never | [M100](M100.md) |
| 101 | `alignof`, `alignas` | C++11 | never | [M101](M101.md) |
| 102 | `decltype`, `decltype(auto)` | C++11 / C++14 | never | [M102](M102.md) |
| 103 | return type deduction `auto f()` | C++14 | never | [M103](M103.md) |
| 104 | trailing return types `auto f() -> int` | C++11 | says it | [M104](M104.md) |
| 105 | `static_cast` and numeric/string conversions | C++98 | says it | [M105](M105.md) |
| 106 | `dynamic_cast` and `typeid` | C++98 | open | [M106](M106.md) |
| 107 | `reinterpret_cast` | C++98 | never | [M107](M107.md) |
| 108 | `const_cast` | C++98 | never | [M108](M108.md) |
| 109 | implicit conversions and narrowing | C++98 / C++11 | partial | [M109](M109.md) |
| 110 | `volatile` | C++98 | never | [M110](M110.md) |
| 111 | the `mutable` keyword | C++98 | never | [M111](M111.md) |
| 112 | `nullptr` and `NULL` | C++98 / C++11 | says it | [M112](M112.md) |
| 113 | uninitialised variables | C++98 | says it | [M113](M113.md) |
| 114 | `std::byte` | C++17 | says it | [M114](M114.md) |
| 115 | aggregate initialisation `Point p{1, 2}` | C++98 / C++11 | open | [M115](M115.md) |
| 116 | designated initialisers `.x = 1` | C++20 | open | [M116](M116.md) |
| 117 | brace initialisation and `std::initializer_list` | C++11 | open | [M117](M117.md) |
| 118 | `std::any` | C++17 | says it | [M118](M118.md) |
| 119 | `std::pair`, `std::tuple`, `std::tie`, `std::apply` | C++98 / C++11 / C++17 | [M35](../SATELLITE/M35-structured-bindings.md) | [M119](M119.md) |
| 120 | `std::array` and C arrays | C++98 / C++11 | says it | [M120](M120.md) |
| 121 | multidimensional arrays and `std::mdspan` | C++98 / C++23 | says it | [M121](M121.md) |
| 122 | `std::span` | C++20 | [M36](../SATELLITE/M36-ranges.md) | [M122](M122.md) |
| 123 | `std::string_view` | C++17 | says it | [M123](M123.md) |

### Operators and expressions

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 124 | arithmetic `+ - * /` | C++98 | says it | [M124](M124.md) |
| 125 | `%` | C++98 | says it | [M125](M125.md) |
| 126 | compound assignment `+=`, `-=`, `*=`, `/=` | C++98 | open | [M126](M126.md) |
| 127 | increment and decrement `++`, `--` | C++98 | open | [M127](M127.md) |
| 128 | bitwise `&`, `\|`, `^`, `~` | C++98 | open | [M128](M128.md) |
| 129 | shifts `<<` and `>>` | C++98 | says it | [M129](M129.md) |
| 130 | comparison `==`, `!=`, `<`, `<=`, `>`, `>=` | C++98 / C++20 | says it | [M130](M130.md) |
| 131 | logical not `!` | C++98 | says it | [M131](M131.md) |
| 132 | unary minus | C++98 | says it | [M132](M132.md) |
| 133 | the comma operator | C++98 | never | [M133](M133.md) |
| 134 | `sizeof...` on a parameter pack | C++11 | [M31](../SATELLITE/M31-user-generics.md) | [M134](M134.md) |
| 135 | order of evaluation and undefined behaviour | C++98 | never | [M135](M135.md) |
| 136 | pointer arithmetic | C++98 | never | [M136](M136.md) |
| 137 | member access `.`, `->`, `.*`, `->*` | C++98 | says it | [M137](M137.md) |
| 138 | scope resolution `::` | C++98 | partial | [M138](M138.md) |
| 139 | parentheses and precedence | C++98 | says it | [M139](M139.md) |
| 140 | string concatenation with `+` | C++98 | says it | [M140](M140.md) |

### Statements

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 141 | range-based `for` | C++11 | open | [M141](M141.md) |
| 142 | `do … while` | C++98 | open | [M142](M142.md) |
| 143 | `else if` | C++98 | says it | [M143](M143.md) |
| 144 | an initialiser in `if` and `switch` | C++17 | open | [M144](M144.md) |
| 145 | a declaration in the `for` header | C++98 | says it | [M145](M145.md) |
| 146 | block scope and shadowing | C++98 | says it | [M146](M146.md) |
| 147 | `return;` from a `void` function | C++98 | says it | [M147](M147.md) |
| 148 | `return` out of a loop | C++98 | says it | [M148](M148.md) |
| 149 | semicolons and several statements on one line | C++98 | never | [M149](M149.md) |
| 150 | several declarators `int a = 1, b = 2;` | C++98 | never | [M150](M150.md) |
| 151 | block comments `/* … */` | C++98 | open | [M151](M151.md) |
| 152 | `if consteval` | C++23 | [M34](../SATELLITE/M34-compile-time-evaluation.md) | [M152](M152.md) |

### Declarations and lifetime

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 153 | global variables | C++98 | says it | [M153](M153.md) |
| 154 | `static` local variables | C++98 | says it | [M154](M154.md) |
| 155 | `thread_local` | C++11 | open | [M155](M155.md) |
| 156 | local functions and local classes | C++98 / C++11 | never | [M156](M156.md) |
| 157 | forward declarations and prototypes | C++98 | says it | [M157](M157.md) |
| 158 | headers and source files | C++98 | never | [M158](M158.md) |
| 159 | recursion | C++98 | says it | [M159](M159.md) |

### Classes, in detail

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 160 | `this` | C++98 | never | [M160](M160.md) |
| 161 | member functions calling each other | C++98 | says it | [M161](M161.md) |
| 162 | `static` data members | C++98 | partial | [M162](M162.md) |
| 163 | `static` member functions | C++98 | says it | [M163](M163.md) |
| 164 | `const` member functions | C++98 | never | [M164](M164.md) |
| 165 | `friend` | C++98 | never | [M165](M165.md) |
| 166 | nested classes | C++98 | open | [M166](M166.md) |
| 167 | `explicit` constructors | C++98 / C++11 | never | [M167](M167.md) |
| 168 | delegating constructors | C++11 | open | [M168](M168.md) |
| 169 | inheriting constructors `using Base::Base` | C++11 | open | [M169](M169.md) |
| 170 | default member initialisers | C++11 | says it | [M170](M170.md) |
| 171 | member initialiser lists | C++98 | open | [M171](M171.md) |
| 172 | copy constructors, copy assignment, the rule of three/five/zero | C++98 / C++11 | never | [M172](M172.md) |
| 173 | `= default` and `= delete` | C++11 | never | [M173](M173.md) |
| 174 | conversion operators `operator bool()` | C++98 / C++11 | never | [M174](M174.md) |
| 175 | `operator[]`, `operator()`, multidimensional `operator[]` | C++98 / C++23 | never | [M175](M175.md) |
| 176 | abstract classes and pure `virtual` | C++98 | [M31](../SATELLITE/M31-user-generics.md) | [M176](M176.md) |
| 177 | `override` | C++11 | partial | [M177](M177.md) |
| 178 | `final` | C++11 | never | [M178](M178.md) |
| 179 | multiple inheritance | C++98 | open | [M179](M179.md) |
| 180 | virtual inheritance and the diamond | C++98 | never | [M180](M180.md) |
| 181 | `std::enable_shared_from_this` | C++11 | never | [M181](M181.md) |
| 182 | `std::weak_ptr` | C++11 | open | [M182](M182.md) |
| 183 | object slicing | C++98 | never | [M183](M183.md) |
| 184 | object layout, standard-layout types, EBO | C++98 / C++11 | never | [M184](M184.md) |

### Templates, in detail

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 185 | function templates | C++98 | [M31](../SATELLITE/M31-user-generics.md) | [M185](M185.md) |
| 186 | class templates | C++98 | [M31](../SATELLITE/M31-user-generics.md) | [M186](M186.md) |
| 187 | template specialisation, full and partial | C++98 | [M31](../SATELLITE/M31-user-generics.md) | [M187](M187.md) |
| 188 | variable templates | C++14 | [M31](../SATELLITE/M31-user-generics.md) | [M188](M188.md) |
| 189 | alias templates | C++11 | [M31](../SATELLITE/M31-user-generics.md) | [M189](M189.md) |
| 190 | class template argument deduction (CTAD) | C++17 | never | [M190](M190.md) |
| 191 | SFINAE and `std::enable_if` | C++98 / C++11 | never | [M191](M191.md) |
| 192 | `<type_traits>` | C++11 | never | [M192](M192.md) |
| 193 | non-type template parameters, `auto` template parameters | C++98 / C++17 | [M31](../SATELLITE/M31-user-generics.md) | [M193](M193.md) |
| 194 | template template parameters | C++98 | never | [M194](M194.md) |
| 195 | abbreviated function templates `void f(auto x)` | C++20 | never | [M195](M195.md) |
| 196 | `requires` clauses | C++20 | [M31](../SATELLITE/M31-user-generics.md) | [M196](M196.md) |
| 197 | explicit instantiation and `extern template` | C++98 / C++11 | never | [M197](M197.md) |
| 198 | CRTP | C++98 | never | [M198](M198.md) |
| 199 | perfect forwarding and `std::forward` | C++11 | never | [M199](M199.md) |

### Functions, in detail

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 200 | pass by value and pass by reference | C++98 | partial | [M200](M200.md) |
| 201 | function pointers | C++98 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M201](M201.md) |
| 202 | pointers to members | C++98 | never | [M202](M202.md) |
| 203 | lambda captures and init-capture | C++11 / C++14 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M203](M203.md) |
| 204 | generic lambdas | C++14 | [M31](../SATELLITE/M31-user-generics.md) | [M204](M204.md) |
| 205 | `std::bind`, `std::bind_front`, `std::invoke` | C++11 / C++17 / C++20 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M205](M205.md) |
| 206 | `std::move_only_function` | C++23 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M206](M206.md) |
| 207 | C variadic functions `...` | C++98 | never | [M207](M207.md) |
| 208 | `constinit` | C++20 | [M34](../SATELLITE/M34-compile-time-evaluation.md) | [M208](M208.md) |
| 209 | `std::reference_wrapper` and `std::ref` | C++11 | says it | [M209](M209.md) |

### Errors, in detail

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 210 | `assert` from `<cassert>` | C++98 | [M42](../CXX26/M42.md) | [M210](M210.md) |
| 211 | custom exception types and `std::exception` | C++98 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M211](M211.md) |
| 212 | `std::terminate`, `std::abort` | C++98 | partial | [M212](M212.md) |
| 213 | `std::error_code` and `errno` | C++98 / C++11 | partial | [M213](M213.md) |
| 214 | `std::exception_ptr`, `rethrow`, nested exceptions | C++11 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M214](M214.md) |
| 215 | `std::stacktrace` | C++23 | partial | [M215](M215.md) |
| 216 | `std::unreachable` | C++23 | never | [M216](M216.md) |

### Memory

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 217 | `malloc`, `calloc`, `realloc`, `free` | C++98 | never | [M217](M217.md) |
| 218 | placement `new` | C++98 | never | [M218](M218.md) |
| 219 | allocators and `std::pmr` | C++98 / C++17 | never | [M219](M219.md) |
| 220 | `memcpy`, `memset`, `memcmp` | C++98 | never | [M220](M220.md) |
| 221 | `std::make_unique`, `std::make_shared` | C++11 / C++14 | never | [M221](M221.md) |
| 222 | running out of memory, `std::bad_alloc` | C++98 | partial | [M222](M222.md) |

### Standard library: utilities

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 223 | `std::swap`, `std::exchange` | C++98 / C++14 | says it | [M223](M223.md) |
| 224 | `std::min`, `std::max`, `std::clamp`, `std::abs` | C++98 / C++17 | says it | [M224](M224.md) |
| 225 | `std::hash` and user types as map keys | C++11 | [M38](../SATELLITE/M38-operator-overloading.md) | [M225](M225.md) |
| 226 | `std::optional` monadic `and_then`, `transform`, `or_else` | C++23 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M226](M226.md) |
| 227 | `std::expected` monadic operations | C++23 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M227](M227.md) |
| 228 | `std::getenv` | C++98 | says it | [M228](M228.md) |
| 229 | `std::system` | C++98 | open | [M229](M229.md) |
| 230 | `std::exit` and the exit code | C++98 | says it | [M230](M230.md) |
| 231 | signals `<csignal>` | C++98 | never | [M231](M231.md) |
| 232 | command-line option parsing | C++98 | says it | [M232](M232.md) |

### Standard library: strings and text

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 233 | `find` and `substr` | C++98 | says it | [M233](M233.md) |
| 234 | `starts_with`, `ends_with`, `contains` | C++20 / C++23 | says it | [M234](M234.md) |
| 235 | `std::to_string`, `std::stoi`, `std::stod` | C++11 | says it | [M235](M235.md) |
| 236 | `std::to_chars`, `std::from_chars` | C++17 | says it | [M236](M236.md) |
| 237 | `toupper`, `tolower` | C++98 | says it | [M237](M237.md) |
| 238 | split, trim and replace | C++20 (`views::split`) | says it | [M238](M238.md) |
| 239 | `std::stringstream`, `std::ostringstream` | C++98 | never | [M239](M239.md) |
| 240 | `std::regex` | C++11 | open | [M240](M240.md) |
| 241 | indexing a string `s[i]`, `s.at(i)` | C++98 | says it | [M241](M241.md) |
| 242 | ordering strings `<`, `compare` | C++98 | open | [M242](M242.md) |
| 243 | `std::locale` and `<codecvt>` | C++98 / C++11 | [M45](../CXX26/M45.md) | [M243](M243.md) |

### Standard library: containers

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 244 | `std::deque` | C++98 | says it | [M244](M244.md) |
| 245 | `std::list`, `std::forward_list` | C++98 / C++11 | says it | [M245](M245.md) |
| 246 | `std::set`, `std::unordered_set` | C++98 / C++11 | partial | [M246](M246.md) |
| 247 | `std::multiset`, `std::multimap` | C++98 | partial | [M247](M247.md) |
| 248 | `std::stack` | C++98 | says it | [M248](M248.md) |
| 249 | `std::queue` | C++98 | says it | [M249](M249.md) |
| 250 | `std::priority_queue` | C++98 | partial | [M250](M250.md) |
| 251 | `std::flat_map`, `std::flat_set` | C++23 | never | [M251](M251.md) |
| 252 | `push_back`, `emplace_back` | C++98 / C++11 | says it | [M252](M252.md) |
| 253 | `insert` and `erase` at a position | C++98 | says it | [M253](M253.md) |
| 254 | `std::erase`, `std::erase_if` | C++20 | partial | [M254](M254.md) |
| 255 | map access: `operator[]`, `find`, `at`, `contains`, `insert_or_assign` | C++98 / C++17 / C++20 | says it | [M255](M255.md) |
| 256 | walking a map's keys and values | C++98 / C++17 | says it | [M256](M256.md) |
| 257 | `reserve`, `capacity`, `shrink_to_fit` | C++98 / C++11 | partial | [M257](M257.md) |
| 258 | `resize` | C++98 | partial | [M258](M258.md) |
| 259 | `front`, `back`, `size`, `empty`, `clear` | C++98 | says it | [M259](M259.md) |

### Standard library: algorithms

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 260 | `std::find`, `std::find_if` | C++98 / C++11 | partial | [M260](M260.md) |
| 261 | `std::count`, `std::count_if` | C++98 | partial | [M261](M261.md) |
| 262 | `std::accumulate`, `std::reduce`, `std::ranges::fold_left` | C++98 / C++17 / C++23 | partial | [M262](M262.md) |
| 263 | `std::min_element`, `std::max_element` | C++98 | says it | [M263](M263.md) |
| 264 | `std::reverse` | C++98 | says it | [M264](M264.md) |
| 265 | `std::sort` with a comparator, `std::stable_sort` | C++98 | partial | [M265](M265.md) |
| 266 | `std::binary_search`, `std::lower_bound` | C++98 | partial | [M266](M266.md) |
| 267 | `std::transform`, `std::for_each` | C++98 | [M36](../SATELLITE/M36-ranges.md) | [M267](M267.md) |
| 268 | `std::remove_if` and the erase–remove idiom | C++98 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M268](M268.md) |
| 269 | `std::all_of`, `std::any_of`, `std::none_of` | C++11 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M269](M269.md) |
| 270 | `std::iota`, `std::fill`, `std::generate` | C++98 / C++11 | says it | [M270](M270.md) |
| 271 | `std::shuffle`, `std::sample` | C++11 / C++17 | partial | [M271](M271.md) |
| 272 | `std::copy`, `std::copy_if` | C++98 / C++11 | partial | [M272](M272.md) |
| 273 | `views::zip`, `enumerate`, `chunk`, `slide`, `ranges::to` | C++23 | [M36](../SATELLITE/M36-ranges.md) | [M273](M273.md) |
| 274 | parallel algorithms and execution policies | C++17 | [M43](../CXX26/M43.md) | [M274](M274.md) |
| 275 | `views::join_with` and joining strings | C++23 | says it | [M275](M275.md) |

### Standard library: numerics

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 276 | `sqrt`, `pow`, `floor`, `ceil`, `round`, `trunc` | C++98 / C++11 | says it | [M276](M276.md) |
| 277 | `sin`, `cos`, `tan`, `exp`, `log` | C++98 | open | [M277](M277.md) |
| 278 | `std::numbers::pi` and mathematical constants | C++20 | open | [M278](M278.md) |
| 279 | `std::gcd`, `std::lcm` | C++17 | open | [M279](M279.md) |
| 280 | `std::midpoint`, `std::lerp` | C++20 | says it | [M280](M280.md) |
| 281 | `std::numeric_limits` | C++98 | never | [M281](M281.md) |
| 282 | `<bit>`: `popcount`, `countl_zero`, `rotl`, `bit_width`, `byteswap` | C++20 / C++23 | open | [M282](M282.md) |
| 283 | `std::bit_cast` | C++20 | partial | [M283](M283.md) |
| 284 | `std::endian` | C++20 | says it | [M284](M284.md) |
| 285 | `std::complex` | C++98 | never | [M285](M285.md) |
| 286 | `std::valarray` | C++98 | never | [M286](M286.md) |
| 287 | `std::ratio` | C++11 | never | [M287](M287.md) |
| 288 | NaN, infinity, `std::isnan`, rounding modes | C++98 / C++11 | open | [M288](M288.md) |
| 289 | mathematical special functions | C++17 | never | [M289](M289.md) |
| 290 | `rand()` and `srand()` | C++98 | says it | [M290](M290.md) |
| 291 | random distributions `normal_distribution`, `poisson_distribution` | C++11 | open | [M291](M291.md) |

### Standard library: input, output and files

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 292 | `std::cin >>` | C++98 | says it | [M292](M292.md) |
| 293 | `std::getline(std::cin, line)` | C++98 | says it | [M293](M293.md) |
| 294 | `std::cerr` and `std::clog` | C++98 | open | [M294](M294.md) |
| 295 | `std::endl`, `std::flush`, stream manipulators | C++98 | says it | [M295](M295.md) |
| 296 | binary file I/O, `seekg`, `tellg` | C++98 | open | [M296](M296.md) |
| 297 | `std::filesystem`: exists, current directory, listing | C++17 | says it | [M297](M297.md) |
| 298 | `std::filesystem::remove` | C++17 | says it | [M298](M298.md) |
| 299 | `create_directory`, `rename`, `copy` | C++17 | open | [M299](M299.md) |
| 300 | `std::osyncstream`: output from several threads | C++20 | open | [M300](M300.md) |

### Standard library: time

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 301 | `std::this_thread::sleep_for` | C++11 | says it | [M301](M301.md) |
| 302 | `system_clock::now()` and subtracting two times | C++11 | partial | [M302](M302.md) |
| 303 | `<chrono>` calendar and time zones | C++20 | partial | [M303](M303.md) |
| 304 | `steady_clock` and measuring intervals | C++11 | open | [M304](M304.md) |

### Standard library: concurrency, in detail

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 305 | `std::jthread` and `std::stop_token` | C++20 | partial | [M305](M305.md) |
| 306 | `std::condition_variable` | C++11 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M306](M306.md) |
| 307 | `std::shared_mutex`, `std::scoped_lock` | C++17 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M307](M307.md) |
| 308 | semaphores, `std::latch`, `std::barrier` | C++20 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M308](M308.md) |
| 309 | `std::call_once` | C++11 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M309](M309.md) |
| 310 | `std::promise`, `std::packaged_task` | C++11 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M310](M310.md) |
| 311 | `std::thread::hardware_concurrency` | C++11 | says it | [M311](M311.md) |
| 312 | `std::atomic<std::shared_ptr>` | C++20 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M312](M312.md) |

### The C heritage

| # | C++ feature | since | satellite | file |
|---|---|---|---|---|
| 313 | C strings: `char *`, `strlen`, `strcpy`, `strcmp` | C++98 | never | [M313](M313.md) |
| 314 | `printf` and `scanf` | C++98 | says it | [M314](M314.md) |
| 315 | `setjmp` and `longjmp` | C++98 | never | [M315](M315.md) |
| 316 | plain `struct` with only data | C++98 | says it | [M316](M316.md) |
| 317 | `FILE *`, `fopen`, `fgets`, `fclose` | C++98 | says it | [M317](M317.md) |
| 318 | `<ctime>`: `time()`, `strftime` | C++98 | partial | [M318](M318.md) |

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
| M31 | user generics | M35, M36, M37, M134, M176, M185, M186, M187, M188, M189, M193, M196, M204 |
| M32 | a capsule as a value | M41, M42, M201, M203, M205, M206, M226, M268, M269 |
| M33 | catching a refusal | M12, M43, M211, M214, M227 |
| M34 | `constexpr` | M8, M24, M62, M152, M208 |
| M35 | structured bindings | M13, M119 |
| M36 | ranges | M17, M122, M267, M273 |
| M37 | ternary | M22 |
| M38 | operator overloading | M32, M33, M225 |
| M39 | overloading, default arguments | M39, M40 |
| M40 | mutexes, atomics, futures | M46, M47, M48, M306, M307, M308, M309, M310, M312 |
| [M42](../CXX26/M42.md) | contracts | M210 |
| [M43](../CXX26/M43.md) | composing concurrent work | M274 |
| [M45](../CXX26/M45.md) | a declared string encoding | M82, M83, M243 |

**M40 is the one to look at first**, and not because C++ programmers ask for it:
there is no mutex and no atomic anywhere under `satellite_containers/`, so two
threads appending to one list is a corrupted list and not a slow one, and
nothing in the language says so. That is a correctness gap in a language that
already ships threads, independent of anything in this folder.

---

## The open rows — to talk through, one at a time

These rows have no honest answer until the author decides something. They are
grouped here so the decisions can be taken together where they share one.
Several share one:

- **program namespaces** decide 68–72 and half of row 4
- **constructor arguments** decide 115, 116, 168, 169 and 171, alongside row 27
- **`enum`** decides 94–96, and it is the one that most changes M30
- **operators satellite lacks** (`+=`, `++`, bitwise) are 126–128 and 282
- **range-based `for`** (141) is the single row most likely to matter to a
  reader arriving from any of the five languages

| row | feature |
|---|---|
| [M61](M61.md) | `__FILE__`, `__LINE__`, `std::source_location` |
| [M68](M68.md) | `using namespace`, using-declarations |
| [M69](M69.md) | namespace aliases |
| [M70](M70.md) | anonymous namespaces, `static` internal linkage |
| [M71](M71.md) | inline namespaces |
| [M72](M72.md) | nested namespace definitions `a::b::c` |
| [M73](M73.md) | `[[nodiscard]]` |
| [M89](M89.md) | digit separators `1'000'000` |
| [M90](M90.md) | raw string literals `R"(...)"` |
| [M94](M94.md) | `enum` |
| [M95](M95.md) | `enum class` |
| [M96](M96.md) | `using enum`, `std::to_underlying` |
| [M97](M97.md) | `typedef` and `using` type aliases |
| [M106](M106.md) | `dynamic_cast` and `typeid` |
| [M115](M115.md) | aggregate initialisation `Point p{1, 2}` |
| [M116](M116.md) | designated initialisers `.x = 1` |
| [M117](M117.md) | brace initialisation and `std::initializer_list` |
| [M126](M126.md) | compound assignment `+=`, `-=`, `*=`, `/=` |
| [M127](M127.md) | increment and decrement `++`, `--` |
| [M128](M128.md) | bitwise `&`, `\|`, `^`, `~` |
| [M141](M141.md) | range-based `for` |
| [M142](M142.md) | `do … while` |
| [M144](M144.md) | an initialiser in `if` and `switch` |
| [M151](M151.md) | block comments `/* … */` |
| [M155](M155.md) | `thread_local` |
| [M166](M166.md) | nested classes |
| [M168](M168.md) | delegating constructors |
| [M169](M169.md) | inheriting constructors `using Base::Base` |
| [M171](M171.md) | member initialiser lists |
| [M179](M179.md) | multiple inheritance |
| [M182](M182.md) | `std::weak_ptr` |
| [M229](M229.md) | `std::system` |
| [M240](M240.md) | `std::regex` |
| [M242](M242.md) | ordering strings `<`, `compare` |
| [M277](M277.md) | `sin`, `cos`, `tan`, `exp`, `log` |
| [M278](M278.md) | `std::numbers::pi` and mathematical constants |
| [M279](M279.md) | `std::gcd`, `std::lcm` |
| [M282](M282.md) | `<bit>`: `popcount`, `countl_zero`, `rotl`, `bit_width`, `byteswap` |
| [M288](M288.md) | NaN, infinity, `std::isnan`, rounding modes |
| [M291](M291.md) | random distributions `normal_distribution`, `poisson_distribution` |
| [M294](M294.md) | `std::cerr` and `std::clog` |
| [M296](M296.md) | binary file I/O, `seekg`, `tellg` |
| [M299](M299.md) | `create_directory`, `rename`, `copy` |
| [M300](M300.md) | `std::osyncstream`: output from several threads |
| [M304](M304.md) | `steady_clock` and measuring intervals |
