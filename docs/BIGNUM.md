# satellite_number's big form — status and the boost decision

## Do we have infinite integers?

Yes, already, and they were never strings. `BigInt` (`include/satellite/bignum.hpp`)
is `std::vector<uint64_t>` limbs, little-endian, no trailing zero limb. An int64
operation that overflows promotes to `Big`; `big::normalize()` demotes back to
`Int` the moment the value fits again, so the fast path stays fast after a
temporary excursion.

Strings are likewise already unbounded — sequences of charmap codes, no ceiling.

## Boost was evaluated and rejected, on measured runtime grounds

boost-devel is installed. The first benchmark said `boost::cpp_int` was **5x
faster at add**. That benchmark was wrong, and the way it was wrong is worth
recording: it let boost accumulate into one reused `sink` object across 200k
iterations, while ours allocated a fresh refcounted `BigInt` every time.

satellite's value model **cannot** reuse a sink. Every arithmetic result is a
new `Container` pointing at a new heap `Obj`. So the benchmark was measuring an
advantage satellite could never collect. Re-run with boost allocating a fresh
heap object per result, exactly as satellite must:

| op | ours | boost | ratio |
|---|---|---|---|
| add 128-bit | 13.5 ms | 11.0 ms | 1.22x |
| add 1024-bit | 17.5 ms | 17.0 ms | **1.03x** |
| add 8192-bit | 2.6 ms | 1.9 ms | 1.38x |
| mul 1024-bit | 10.3 ms | 9.9 ms | **1.03x** |
| mul 8192-bit | 26.7 ms | 17.5 ms | 1.53x |
| to_string 1024-bit | 126.9 ms | 155.8 ms | **0.81x** (ours faster) |

The 5x was allocation, not arithmetic. **Decision: keep the hand-written
BigInt.** The only real gap is 8192-bit multiply (1.53x), which is boost
switching to Karatsuba; a cutoff in `mul_limbs` would close it if a workload
ever needs it.

Note this is *not* the compile-time argument — that one was raised and
overruled, correctly, and is dropped. This is runtime, in satellite's actual
value model, which is the only axis that counts.

## What landed

`src/bigdiv.cpp` now provides, all tested in `tests/test_bignum.cpp`
(48 checks, ASan + UBSan clean):

- **`big::divmod`** — Knuth TAOCP 4.3.1 Algorithm D in base 2^64, with short
  division for a single-limb divisor. Truncated semantics: the quotient rounds
  toward zero and the remainder takes the sign of the dividend, so `-7/2 == -3`
  and `-7%2 == -1`. Returns false on a zero divisor rather than inventing a
  value. Either out-pointer may be null.
- **`big::pow_u64`** — square-and-multiply, with a limb ceiling so that
  `2 ** 10000000000` returns nil instead of exhausting memory.
- **`big::shl` / `big::shr`** — magnitude shifts, so `shr` truncates toward zero.
- **`big::from_string`** — decimal text, folded 19 digits at a time because
  10^19 is the largest power of ten that fits a limb. Accepts a leading sign and
  `_` separators, exactly as the lexer spells them.

`src/big_limbs.hpp` holds the limb primitives (`trim`, `cmp_limbs`, `add_limbs`,
`sub_limbs`, `mul_limbs`, `mul_add_small`) shared by `bignum.cpp` and
`bigdiv.cpp`, so the two files cannot drift. It lives in `src/` rather than
`include/` deliberately — everything under `include/` is preprocessed by every
`satellite.cxx` block at runtime.

### How the division is actually tested

Not by a list of hand-picked examples. Algorithm D's correction path (step D6,
where the estimated quotient limb turns out one too large and the divisor has to
be added back) fires roughly once in 2^64 random divisions — no set of small
cases will ever reach it, and omitting it produces code that passes every casual
check and is silently wrong forever after.

So the real test is the identity: 4,000 random divisions with operands up to 8
limbs wide, each verified by computing `q*b + r` and checking it equals `a`,
that `|r| < |b|`, and that `r` never disagrees in sign with `a`.

## Still missing

1. **`div_table` and `mod_table` wiring in `ops.cpp`.** `divmod` exists but the
   language cannot reach it: `div_table[Big][Big]` is still `op_unsupported`,
   and there is no mod table at all, so `Tok::Percent` still has nowhere to go.
2. **Lexer promotion.** `src/lexer.cpp` keeps the digits of an over-large
   literal in `t.text` and parks `ival` at 0. `big::from_string` is now the
   function that would promote it, but nothing calls it yet — that belongs to
   the compiler, which does not exist.
3. **Karatsuba**, if 8192-bit multiply ever becomes a real workload.
