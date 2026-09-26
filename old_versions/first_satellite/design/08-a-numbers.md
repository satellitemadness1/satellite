*satellite design docs, §8, part 1 of 4. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§7](07-method-calls-indexing-slicing.md), On: [§8 part 2](08-b-time-file-and-window.md).*

---

## 8. Types

Each type is a C++ struct with named fields, exposed through accessor methods.

| satellite type | C++ representation | notes |
|---|---|---|
| `satellite.variable.bool` | `bool` | exists; written `satellite.bool.true` / `.false`, see §8.4 |
| `satellite.variable.number` | `Number` (arbitrary precision) | **replaces `double`** — see §8.1 |
| `satellite.variable.string` | `SatString` | exists; 16-bit code table, see §8.5 |
| `satellite.container.list<T>` | `std::vector<ValuePtr>` | exists; children shared by pointer |
| `satellite.container.map<K, V>` | `MapBody` behind a handle | exists; insertion-ordered; keys restricted, see §8.6 |
| `satellite.variable.time` | `struct Time { int64_t ns; }` | absolute instant, UTC |
| `satellite.variable.file` | `shared_ptr<FileHandle>` | reference type |
| `satellite.variable.window` | `shared_ptr<WindowHandle>` | reference type |
| `satellite` | the runtime singleton | `satellite.return(satellite)` |

### 8.1 Numbers — one type, arbitrary precision

"Technically infinite" should mean **exact arbitrary-precision decimal** (bignum significand
× 10^exp), named `satellite.variable.number`. Not rationals (denominators grow without bound
and collide with the memory watchdog), not integer-only (`1/3` → 0 is unacceptable), not double.

**`double` must leave the variant.** **Verified by doing the migration on a copy of the
project**: replacing `double` with a 32-byte
`Number { long long sig; int exp; shared_ptr<const BigInt> big; }` leaves
`sizeof(ValueBase)` unchanged at 40 bytes, and the migrated `library_test` passes 160,000
concurrent increments TSan-clean. Only four source files plus the Makefile change.

Three traps, each found only by compiling:

1. After removing `double`, `Value v = 3.14` (`main.cpp:41`, and again at `main.cpp:94`)
   does **not** fail to compile — it silently truncates to `3`, with zero warnings under
   the project's actual `-Wall -Wextra -O2`.
2. The obvious guard `Number(double) = delete;` then breaks `Number(42)`, because deleted
   overloads still participate in resolution. What works is a template constructor
   constrained to integral types **plus** deleted float overloads.
3. "Keep both `double` and `Number`" is worse than it looks: `Value v = 4096.0` silently
   selects `double` while `Value v = 42` is a hard compile error. That would split
   `satellite.library` into two numeric representations and make the watchdog quietly ignore
   any user retuning of `min_free_mb`.

Implement with **base-10^9 limbs**, not base 2^32 — decimal I/O and 10^k scaling become
trivial, which is most of a decimal type's work. Roughly 400 lines for integer-only
`+ - * / %` and decimal I/O; ~800 for full decimal.

**Staging:** ship v1 with the `Number` *type name and semantics* even if the first
implementation is int64-backed, so adding full precision later is invisible to programs
already written.

### 8.1.2 What landed, and the three traps in the flesh

**Done**, in `bignum.hpp` / `bignum.cpp`. `sizeof(ValueBase)` is unchanged at 40 bytes and
the whole suite passes, including `library_test` under TSan.

The representation is §8.1's, exactly: `long long sig; int exp; shared_ptr<const BigInt>
big`, 32 bytes, with a **null `big` as the fast path**. That split is what keeps the
migration cheap at run time — a loop counter, an index and every small literal live entirely
in the `long long`, so `i = i + 1` is an add and an overflow check with no allocation and no
atomic. Measured on the §14 benchmark: **2.79 s against the double version's 2.79 s**, which
is to say the exactness is free at this workload.

`BigInt` is base 10^9 little-endian limbs, per §8.1's reasoning: this backs a decimal type,
so decimal I/O and scaling by 10^k are most of the work, and both are limb shifts in base
10^9 against full base conversions in base 2^32. Division is long division one **decimal**
digit at a time rather than Knuth's algorithm D in base 10^9 — the inner loop runs at most
nine times, and getting nine comparisons right is a different proposition from getting
base-10^9 quotient estimation and its correction step right.

**All three traps fired.** They were predicted on paper and every one of them turned up as a
real compile error during the migration:

1. `Value v = 4096.0` in `main.cpp` — the memory watchdog's `min_free_mb` default. Under the
   double version this compiled clean and silently truncated. It is now a compile error.
2. The constrained template plus deleted float overloads is what makes that error appear
   while `Number(42)` still works. `bool` is excluded from the template so `Value v = true`
   still selects the variant's `bool`.
3. `system.cpp`'s watchdog read `get_if<double>(min_free_mb)` — precisely the site §8.1
   warned would "quietly ignore any user retuning" if both representations coexisted.

**What is and is not exact.** Addition, subtraction and multiplication are exact with no
bound on digits, and so is division that terminates — which includes every division by a
power of ten, so a count of nanoseconds becomes an exact number of seconds. Division that
does not terminate is rounded to `satellite.library.system.division_digits` significant
figures, default 34 (decimal128's precision). That is the only rounding anywhere in the
language, and it is unavoidable rather than chosen.

### 8.1.1 How a number renders

**The rule is "print the value", never "print N significant digits."** A whole number
prints whole; a fraction prints the shortest digit string that reads back as the same
number.

This is phrased as a rule about *values* on purpose, and that is what let §8.1's migration
happen underneath it: the `double` became an arbitrary-precision decimal, "print the value"
went on meaning the same thing, and programs written before it kept printing what they
printed. A rule phrased in significant digits would not have survived. The *implementation*
this section used to name did not survive it, and where it went belongs here rather than
only in a code comment. It was `std::to_chars` in `std::chars_format::fixed`, in
`value.cpp`; there is no `to_chars` anywhere in the tree today. Rendering is
`Number::to_string` in `bignum.cpp`, because how a number renders is a property of the type
rather than of the printer — which is the reason `value.cpp` no longer answers the question
at all.

**The notation boundary was restated, and it moved.** It was `[1e-6, 1e21)`, on the grounds
that outside it digits stop being informative and `1e308` in fixed notation is 309
characters. That was true of a `double`, where a 33-digit integer had no 33 informative
digits to give. An exact decimal has them, so `bignum.cpp` rewrote the boundary in the terms
the reasoning had always really been about:

> switch to scientific when the padding zeros would outnumber the information, never when
> the information itself is long.

The thresholds are 20 trailing and 5 leading zeros, which are the old bounds restated — 20
trailing zeros is `1e20`, 5 leading is `1e-6` — so every value that printed fixed before
still does. What changed is what happens above them. `1e308` is one significant digit and
308 zeros and stays scientific; `30!` is 26 significant digits and 7 zeros and prints whole:

```
265252859812191058636308480000000
```

That value is 2.65e32, comfortably outside `[1e-6, 1e21)`, so the old rule would have
rendered it `2.6525285981219105863630848e+32` — hiding the exact answer the type went to
the trouble of computing, which is the one thing an exact type must not do.

**Verified defect this fixes.** The original `snprintf(buf, sizeof buf, "%g", d)` printed
six significant digits, so `123456789` displayed as `1.23457e+08` and `2000000 + 1` as
`2e+06` — wrong values, through the only output path the language has, since
`satellite.console.display`, the REPL echo, `.to_string()` and every error message that
quotes a number all arrive at the same function. Note that neither obvious alternative
works: `%.17g` round-trips but renders `0.1` as `0.10000000000000001`, and `to_chars`'
own `general` mode picks whichever notation is *shorter*, so it returns `1.23456789e+08`
for the same number. `fixed` was the one that was exact and readable at once, which is where
the `double` era settled; `Number::to_string` inherits the requirement rather than the
function.

Exactness is shown rather than hidden: `0.1 + 0.2` renders `0.3`, because §8.1's decimal
migration made `0.3` the value that is actually there. Before it the same expression
rendered `0.30000000000000004`, which was equally the value that was actually there, in a
type that could not hold the other one. Neither answer was ever a rounding rule bolted onto
the printer, which is the point: the printer never decided anything.
