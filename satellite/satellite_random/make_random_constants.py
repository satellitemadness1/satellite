#!/usr/bin/env python3
# satellite/satellite_random/make_random_constants.py -- the five constants the 512-bit
# generator needs, written as random_constants.hpp. Run it and commit both; never edit
# the header.
#
#     python3 satellite/satellite_random/make_random_constants.py            writes the header
#     python3 satellite/satellite_random/make_random_constants.py --check    0 when the header is what it would write
#
# WHY A SCRIPT AND NOT TYPED HEX. PCG's templates take their LCG multipliers and increments
# BY TYPE (pcg_random.hpp, default_multiplier<T>), and upstream published them for 8 to
# 128 bits only. satellite.random.ultra runs that same scheme at 512 bits of output over a
# 1024-bit state (pcg_512.hpp), so it needs constants nobody published:
#
#     ultra_multiplier       1024 bits   the base LCG's multiplier
#     ultra_increment        1024 bits   the base LCG's default increment (the default stream)
#     cell_multiplier         512 bits   each extension cell's own LCG multiplier
#     cell_increment          512 bits   ... and its increment
#     cell_mix_multiplier     512 bits   the odd multiplier inside RXS-M-XS, the cells' output
#                                        permutation; its inverse is worked out at compile time
#
# THEY ARE THE BINARY DIGITS OF PI, in order, after the point: 3584 bits, cut into those
# five, with the low bits each one MUST have set by hand:
#
#     a multiplier is 5 (mod 8)   an LCG modulo 2^k has its full period 2^k exactly when the
#                                 increment is odd and the multiplier is 1 (mod 4) (Hull and
#                                 Dobell, 1962); PCG picks 5 (mod 8) so that the same constant
#                                 also serves a multiplier-only (MCG) generator at 2^(k-2)
#     an increment is odd         the other half of the same theorem
#     the mix multiplier is odd   so it has an inverse modulo 2^512, which the extension
#                                 table's "inside out" stepping needs (pcg_random.hpp,
#                                 inside_out::external_step calls unoutput)
#
# PI, BECAUSE IT IS NOBODY'S CHOICE. A multiplier typed by hand is a number somebody could
# have chosen for a reason they did not write down; the digits of pi are the standard
# "nothing up my sleeve" source (Blowfish's tables are the same digits), and anyone can
# recompute them. What pi does NOT buy is a spectral test: nobody has measured the lattice
# of a 1024-bit LCG with this multiplier, and this file does not claim one. What the
# generator rests on instead is the period theorem above, the XSL-RR output permutation,
# and the 16384-word extension table XORed over every output -- the same three things
# pcg32_k16384 rests on -- and random_cases.cpp's battery, run on every check.
#
# THE ARITHMETIC IS INTEGERS ONLY, Machin's formula at 3584 + 128 bits, so it is exact to
# far below the last bit kept and runs the same on CPython and PyPy. The first 128 bits are
# asserted against the four words every Blowfish implementation carries.
#
# Written 2026-10-02.

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.join(HERE, "random_constants.hpp")

BITS = 1024 + 1024 + 512 + 512 + 512     # 3584
GUARD = 128
BLOWFISH_P1_TO_P4 = 0x243F6A8885A308D313198A2E03707344   # pi's first 128 fraction bits, hex

CUTS = [
    ("ultra_multiplier", 1024, "multiplier"),
    ("ultra_increment", 1024, "increment"),
    ("cell_multiplier", 512, "multiplier"),
    ("cell_increment", 512, "increment"),
    ("cell_mix_multiplier", 512, "odd"),
]


def arctan_of_inverse(x, scale):
    """arctan(1/x) * scale, by the series, in integers. Each term is floored, so the answer
    is below the truth by less than the number of terms -- a few hundred units of 2^-GUARD."""
    total, term, k, sign = 0, scale // x, 0, 1
    square = x * x
    while term:
        total += sign * (term // (2 * k + 1))
        term //= square
        k += 1
        sign = -sign
    return total


def pi_fraction_bits(bits):
    """floor(frac(pi) * 2^bits): the first `bits` binary digits of pi after the point."""
    scale = 1 << (bits + GUARD)
    pi_scaled = 16 * arctan_of_inverse(5, scale) - 4 * arctan_of_inverse(239, scale)   # Machin, 1706
    whole = pi_scaled >> GUARD
    fraction = whole - (3 << bits)
    if not 0 <= fraction < (1 << bits):
        sys.exit("make_random_constants.py: pi did not come out between 3 and 4")
    return fraction


def limbs_of(value, bits):
    """64-bit limbs, LEAST significant first -- satellite_number's order and wide_unsigned's."""
    return [(value >> (64 * i)) & 0xFFFFFFFFFFFFFFFF for i in range(bits // 64)]


def constants():
    fraction = pi_fraction_bits(BITS)
    if fraction >> (BITS - 128) != BLOWFISH_P1_TO_P4:
        sys.exit("make_random_constants.py: the first 128 bits are not pi's")
    out, taken = [], 0
    for name, width, must_be in CUTS:
        value = (fraction >> (BITS - taken - width)) & ((1 << width) - 1)
        taken += width
        if must_be == "multiplier":
            value = (value & ~7) | 5
        else:
            value |= 1
        out.append((name, width, must_be, value))
    return out


def header_text():
    lines = [
        "#pragma once",
        "// satellite/satellite_random/random_constants.hpp -- GENERATED by make_random_constants.py.",
        "// Do not edit: the script says what these are and why, and check.sh refuses a header the",
        "// script would not write.",
        "//",
        "// The binary digits of pi after the point, %d of them, cut into the five constants the" % BITS,
        "// 512-bit generator needs (pcg_512.hpp), least significant limb first; a multiplier's low",
        "// three bits are 101 and an increment's low bit is 1, set by the script, which is what",
        "// gives each LCG its full period.",
        "",
        "namespace satellite004::random_constants {",
        "",
    ]
    for name, width, must_be, value in constants():
        lines.append("// %d bits, %s: 0x%0*X" % (width, {"multiplier": "5 (mod 8)", "increment": "odd",
                                                          "odd": "odd"}[must_be], width // 4, value))
        lines.append("inline constexpr unsigned long long int %s[%d] = {" % (name, width // 64))
        limbs = limbs_of(value, width)
        for i in range(0, len(limbs), 2):
            pair = ", ".join("0x%016Xull" % limb for limb in limbs[i:i + 2])
            lines.append("    %s," % pair)
        lines.append("};")
        lines.append("")
    lines.append("} // namespace satellite004::random_constants")
    lines.append("")
    return "\n".join(lines)


def main():
    text = header_text()
    if "--check" in sys.argv[1:]:
        try:
            with open(HEADER, encoding="utf-8") as handle:
                same = handle.read() == text
        except OSError:
            same = False
        print("random_constants.hpp: %s" % ("as generated" if same else "DIFFERS from what the script writes"))
        sys.exit(0 if same else 1)
    with open(HEADER, "w", encoding="utf-8") as handle:
        handle.write(text)
    print("random_constants.hpp: %d bits of pi in %d constants" % (BITS, len(CUTS)))


if __name__ == "__main__":
    main()
