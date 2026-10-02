#pragma once
// satellite/satellite_random/pcg_512.hpp -- pcg32_k16384 at 512 bits: the generator behind
// satellite.random, made from the vendored PCG templates without a line of them changed.
//
// THE AUTHOR, 2026-10-02: "the generator in /vendor has a 16386k variant that I wanted to
// use, it's 32_16386 but I wanted to make it 512_16386 so it's a 512-bit x 16386 generator".
// pcg32_k16384 (pcg_random.hpp, its last typedef) is
//
//     extended<14, 16, setseq_xsh_rr_64_32, oneseq_rxs_m_xs_32_32, true>
//
// a 64-bit LCG whose 32-bit output (XSH-RR) is XORed with one of 2^14 = 16384 extension
// words, each word a little 32-bit generator of its own that steps every 2^16 outputs.
// This file is the same template with 512 written where 32 was:
//
//     extended<14, 16, setseq_xsl_rr_1024_512, oneseq_rxs_m_xs_512_512, true>
//
// a 1024-bit LCG whose 512-bit output (XSL-RR, the permutation PCG uses once a state is
// "split across registers", as pcg64's is) is XORed with one of 16384 512-bit words. The
// table is 16384, not 16386: PCG indexes it with the state's low 14 bits, so it is a power
// of two by construction. State: 128 bytes of LCG, 128 of stream, 1 MiB of table.
// Period, by pcg's own period_pow2(): 2^(1024 + 16384 * 512) = 2^8389632.
//
// ONLY ONE TRANSLATION UNIT INCLUDES THIS (random_source.cpp), plus the harness. It pulls
// in the Apache-2.0 header, and 003's rule carries over: nothing above the generator names
// a PCG type, so the dependency has one file to be replaced in (random_source.hpp is the
// seam).
//
// THREE THINGS UPSTREAM DID NOT WRITE, each supplied here beside its reason:
//
//   PCG_BITCOUNT_T. pcg counts bits in a uint8_t by default, and 512 does not fit one:
//       `constexpr bitcount_t bits = sizeof(itype) * 8` would be 0 at 512 bits and the
//       rotate would be by the wrong mask. pcg_extras.hpp names the knob for exactly this
//       ("If you're using a nonstandard generator of a larger size, you can set
//       PCG_BITCOUNT_T"); it is set to unsigned int, which changes nothing at 32 bits --
//       random_cases.cpp proves pcg32 and pcg32_k16384 still answer upstream's own
//       expected output.
//   THE INTEGER TYPES, wide_unsigned<8> and <16> (wide_unsigned.hpp).
//   THE CONSTANTS, which pcg looks up by type (default_multiplier<T>): the five below,
//       pi's digits by make_random_constants.py, and the cells' inverse multiplier worked
//       out by the compiler and proved by static_assert.
//
// Written 2026-10-02.

#ifndef PCG_BITCOUNT_T
#define PCG_BITCOUNT_T unsigned int
#endif

#include <pcg_random.hpp>   // vendor/pcg-cpp/pcg-cpp-0.98/include, through -isystem (make_support/060-compile.mk)

#include "random_constants.hpp"
#include "wide_unsigned.hpp"

namespace satellite004 {

using uint512 = wide_unsigned<8>;
using uint1024 = wide_unsigned<16>;

// The inverse of an odd number modulo 2^512, by Newton's iteration: x = x * (2 - a * x)
// doubles the number of correct low bits each round, and an odd a is its own inverse to 3
// bits (a * a = 1 mod 8), so nine rounds reach 768 > 512.
constexpr uint512 inverse_of_odd(const uint512 &a)
{
    uint512 x = a;
    for (int round = 0; round < 9; ++round)
        x = x * (uint512(2) - a * x);
    return x;
}

inline constexpr uint1024 kUltraMultiplier = uint1024::of_limbs(random_constants::ultra_multiplier);
inline constexpr uint1024 kUltraIncrement = uint1024::of_limbs(random_constants::ultra_increment);
inline constexpr uint512 kCellMultiplier = uint512::of_limbs(random_constants::cell_multiplier);
inline constexpr uint512 kCellIncrement = uint512::of_limbs(random_constants::cell_increment);
inline constexpr uint512 kCellMixMultiplier = uint512::of_limbs(random_constants::cell_mix_multiplier);
inline constexpr uint512 kCellMixUnmultiplier = inverse_of_odd(kCellMixMultiplier);

static_assert((kUltraMultiplier.limb[0] & 7) == 5, "a PCG multiplier is 5 (mod 8)");
static_assert((kCellMultiplier.limb[0] & 7) == 5, "a PCG multiplier is 5 (mod 8)");
static_assert((kUltraIncrement.limb[0] & 1) == 1, "an LCG increment is odd");
static_assert((kCellIncrement.limb[0] & 1) == 1, "an LCG increment is odd");
static_assert(kCellMixMultiplier * kCellMixUnmultiplier == uint512(1), "the cells' multiplier has its inverse");

} // namespace satellite004

// WHAT pcg LOOKS UP BY TYPE. These are specialisations of upstream's own templates, in its
// namespace, which is how C++ asks for them -- pcg_random.hpp's PCG_DEFINE_CONSTANT writes
// exactly this shape for uint8_t to pcg128_t.
namespace pcg_detail {

template <> struct default_multiplier<satellite004::uint1024> {
    static constexpr satellite004::uint1024 multiplier() { return satellite004::kUltraMultiplier; }
};
template <> struct default_increment<satellite004::uint1024> {
    static constexpr satellite004::uint1024 increment() { return satellite004::kUltraIncrement; }
};
template <> struct default_multiplier<satellite004::uint512> {
    static constexpr satellite004::uint512 multiplier() { return satellite004::kCellMultiplier; }
};
template <> struct default_increment<satellite004::uint512> {
    static constexpr satellite004::uint512 increment() { return satellite004::kCellIncrement; }
};
template <> struct mcg_multiplier<satellite004::uint512> {
    static constexpr satellite004::uint512 multiplier() { return satellite004::kCellMixMultiplier; }
};
template <> struct mcg_unmultiplier<satellite004::uint512> {
    static constexpr satellite004::uint512 unmultiplier() { return satellite004::kCellMixUnmultiplier; }
};

} // namespace pcg_detail

namespace satellite004 {

// THE INVERSE XORSHIFT, BY DOUBLING. pcg_extras::unxorshift undoes `x ^= x >> s` by
// recursion, one level per s bits of width: at 512 bits and a shift of 6 that is 85 levels
// of 512-bit arithmetic, for each of 16384 cells, every 65536 draws -- measured at 595 ns a
// draw on this machine, nearly all of it the table (SCRATCH.md/RANDOM.md). The same inverse
// as x ^= x >> s, x ^= x >> 2s, x ^= x >> 4s ... until the shift passes the width is seven
// steps: (1 + T^s)(1 + T^2s)(1 + T^4s)... is 1 + T^s + T^2s + T^3s + ..., every term the
// inverse needs, and a shift at or past the width is 0 (wide_unsigned.hpp). pcg calls it
// unqualified from inside pcg_detail with `bits` the type's whole width, so this overload,
// in wide_unsigned's own namespace, is found by argument-dependent lookup and preferred to
// the general template -- the C++ way of supplying an operation for one's own type without
// editing the library that calls it. random_cases.cpp holds it to pcg's own, qualified.
template <unsigned Limbs>
constexpr wide_unsigned<Limbs> unxorshift(wide_unsigned<Limbs> x, unsigned bits, unsigned shift)
{
    for (unsigned by = shift; by < bits; by *= 2)
        x ^= x >> by;
    return x;
}

// The base: a 1024-bit LCG with a settable stream, XSL-RR to 512 bits -- pcg64 at eight
// times the width (setseq_xsl_rr_128_64 is pcg64).
using pcg_setseq_xsl_rr_1024_512 = pcg_detail::setseq_base<uint512, uint1024, pcg_detail::xsl_rr_mixin>;

// One extension cell: a 512-bit LCG whose RXS-M-XS output is a permutation of its state,
// which is what lets pcg step a cell from its VALUE (inside_out) -- oneseq_rxs_m_xs_32_32
// at sixteen times the width.
using pcg_oneseq_rxs_m_xs_512_512 = pcg_detail::oneseq_base<uint512, uint512, pcg_detail::rxs_m_xs_mixin>;

// pcg32_k16384, at 512 bits.
using pcg512_k16384 = pcg_detail::extended<14, 16, pcg_setseq_xsl_rr_1024_512, pcg_oneseq_rxs_m_xs_512_512, true>;

static_assert(sizeof(pcg512_k16384::result_type) == 64, "ultra answers 512 bits");
static_assert(pcg512_k16384::period_pow2() == 1024 + 16384 * 512, "the period pcg computes for it");

} // namespace satellite004
