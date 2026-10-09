#pragma once
// satellite/satellite_random/wide_unsigned.hpp -- an unsigned integer of one fixed width,
// 64 bits a limb, so that PCG's templates can be instantiated at 512 and 1024 bits.
//
// WHY IT EXISTS. pcg_random.hpp is written "at potentially arbitrary bit sizes" (its own
// words) over whatever integer type it is handed, and upstream hands it uint8_t up to
// __uint128_t. satellite.random.ultra is pcg32_k16384 at 512 bits (pcg_512.hpp), so it
// needs a 512-bit and a 1024-bit unsigned integer with the operators those templates
// write: + - * (modulo the width), the shifts, & | ^ ~, the comparisons, a conversion
// from and to an ordinary integer. This is that and nothing more -- no division, no
// text, no sign -- so it stays small enough to read in one sitting.
//
// FIXED WIDTH, NEVER satellite_number: the generator's arithmetic is modulo 2^1024 by
// design (an LCG wraps), satellite_number never wraps by design, and a number that
// allocates would put the allocator on every draw.
//
// THE THREE RULES THAT MAKE VENDOR'S TEMPLATES COMPILE OVER IT, each one a line in
// pcg_random.hpp that reads differently from how it looks:
//
//   1. THE CONSTRUCTOR FROM AN INTEGER IS EXPLICIT and the conversion TO ONE is not.
//      pcg's `extended` writes `size_t index = state & table_mask;` and
//      `bitcount_t rshift = opbits ? (internal >> (bits - opbits)) & mask : 0;` -- a
//      class type on one side of a conditional with an int on the other. C++ resolves
//      that by converting ONE way, and if both ways were implicit it would be ambiguous
//      and refuse to compile. Explicit in, implicit out, and both lines read as the
//      low limb, which is what they mean.
//   2. EVERY OPERATOR ALSO TAKES AN ORDINARY INTEGER ON EITHER SIDE, as a template that
//      is an exact match. Without those, `lowmask1 - 1` would still compile -- through
//      the implicit conversion in rule 1, as 64-bit arithmetic on the low limb -- and be
//      silently wrong. An exact match beats a conversion, so the wide form is always
//      the one chosen.
//   3. DIVISION IS DELETED. Nothing in the paths satellite uses divides, and the same
//      silent fall-through as rule 2 would otherwise make `x % n` compile as the low
//      limb's remainder. A deleted function is still the best match, so it is an error.
//
// A SHIFT BY THE WHOLE WIDTH OR MORE IS ZERO, where C++ leaves it undefined: pcg's rotate
// shifts by `(-rot) & mask`, which is never the width, but a bignum that answers 0 there
// is one fewer thing to be wrong about.
//
// CONSTEXPR THROUGHOUT, because pcg_random.hpp declares its masks `static constexpr
// state_type`, and because the inverse of the cells' multiplier is worked out by the
// compiler (pcg_512.hpp) rather than typed. unsigned __int128 is the only wider type
// used, for a limb product and a carry -- the same choice satellite_number_limbs.hpp makes.
//
// Written 2026-10-02.

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace satellite004 {

template <unsigned Limbs>
class wide_unsigned {
    static_assert(Limbs >= 2, "one limb is unsigned long long int itself");

public:
    static constexpr unsigned limb_count = Limbs;
    static constexpr unsigned bit_count = Limbs * 64;

    unsigned long long int limb[Limbs];   // least significant first, as satellite_number keeps them

    constexpr wide_unsigned() : limb{} {}

    // Rule 1: explicit. A negative signed integer is sign-extended, so -1 is every bit set.
    template <std::integral Integer>
    explicit constexpr wide_unsigned(Integer value) : limb{}
    {
        limb[0] = static_cast<unsigned long long int>(value);
        if constexpr (std::is_signed_v<Integer>)
            if (value < 0)
                for (unsigned i = 1; i < Limbs; ++i)
                    limb[i] = ~0ull;
    }

    // Narrower or wider: the low limbs are kept, zeros above. pcg's XSL-RR writes
    // `xtype(internal >> bottomspare)` -- the low 512 of a 1024-bit state -- through this.
    template <unsigned Other>
    explicit constexpr wide_unsigned(const wide_unsigned<Other> &other) : limb{}
    {
        for (unsigned i = 0; i < Limbs && i < Other; ++i)
            limb[i] = other.limb[i];
    }

    // From random_constants.hpp's arrays, least significant limb first.
    static constexpr wide_unsigned of_limbs(const unsigned long long int (&least_first)[Limbs])
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i)
            out.limb[i] = least_first[i];
        return out;
    }

    // Rule 1: implicit, and it is the LOW LIMB -- the index into a table, the count of a
    // rotate, both of which pcg masks down first.
    constexpr operator unsigned long long int() const { return limb[0]; }
    explicit constexpr operator bool() const
    {
        for (unsigned i = 0; i < Limbs; ++i)
            if (limb[i] != 0)
                return true;
        return false;
    }

    // --- arithmetic, modulo 2^bit_count ---------------------------------------------

    friend constexpr wide_unsigned operator+(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        unsigned long long int carry = 0;
        for (unsigned i = 0; i < Limbs; ++i) {
            const unsigned __int128 sum = static_cast<unsigned __int128>(a.limb[i]) + b.limb[i] + carry;
            out.limb[i] = static_cast<unsigned long long int>(sum);
            carry = static_cast<unsigned long long int>(sum >> 64);
        }
        return out;
    }

    friend constexpr wide_unsigned operator-(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        unsigned long long int borrow = 0;
        for (unsigned i = 0; i < Limbs; ++i) {
            const unsigned long long int taken = b.limb[i] + borrow;
            const bool wrapped = taken < b.limb[i];              // b.limb[i] was ~0 and borrow 1
            out.limb[i] = a.limb[i] - taken;
            borrow = (a.limb[i] < taken) || wrapped ? 1 : 0;
        }
        return out;
    }

    // Schoolbook, the products above the width dropped: 136 limb products at 1024 bits.
    friend constexpr wide_unsigned operator*(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i) {
            unsigned long long int carry = 0;
            for (unsigned j = 0; i + j < Limbs; ++j) {
                const unsigned __int128 product =
                    static_cast<unsigned __int128>(a.limb[i]) * b.limb[j] + out.limb[i + j] + carry;
                out.limb[i + j] = static_cast<unsigned long long int>(product);
                carry = static_cast<unsigned long long int>(product >> 64);
            }
        }
        return out;
    }

    friend constexpr wide_unsigned operator-(const wide_unsigned &a) { return wide_unsigned() - a; }

    friend constexpr wide_unsigned operator~(const wide_unsigned &a)
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i)
            out.limb[i] = ~a.limb[i];
        return out;
    }

    friend constexpr wide_unsigned operator&(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i)
            out.limb[i] = a.limb[i] & b.limb[i];
        return out;
    }

    friend constexpr wide_unsigned operator|(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i)
            out.limb[i] = a.limb[i] | b.limb[i];
        return out;
    }

    friend constexpr wide_unsigned operator^(const wide_unsigned &a, const wide_unsigned &b)
    {
        wide_unsigned out;
        for (unsigned i = 0; i < Limbs; ++i)
            out.limb[i] = a.limb[i] ^ b.limb[i];
        return out;
    }

    // --- the shifts, by an ordinary count ----------------------------------------------

    template <std::integral Count>
    friend constexpr wide_unsigned operator<<(const wide_unsigned &a, Count by)
    {
        wide_unsigned out;
        if (by < 0 || static_cast<unsigned long long int>(by) >= bit_count)
            return out;
        const unsigned whole = static_cast<unsigned>(by) / 64, part = static_cast<unsigned>(by) % 64;
        for (unsigned i = Limbs; i-- > whole;) {
            out.limb[i] = a.limb[i - whole] << part;
            if (part != 0 && i - whole > 0)
                out.limb[i] |= a.limb[i - whole - 1] >> (64 - part);
        }
        return out;
    }

    template <std::integral Count>
    friend constexpr wide_unsigned operator>>(const wide_unsigned &a, Count by)
    {
        wide_unsigned out;
        if (by < 0 || static_cast<unsigned long long int>(by) >= bit_count)
            return out;
        const unsigned whole = static_cast<unsigned>(by) / 64, part = static_cast<unsigned>(by) % 64;
        for (unsigned i = 0; i + whole < Limbs; ++i) {
            out.limb[i] = a.limb[i + whole] >> part;
            if (part != 0 && i + whole + 1 < Limbs)
                out.limb[i] |= a.limb[i + whole + 1] << (64 - part);
        }
        return out;
    }

    // --- the comparisons -------------------------------------------------------------

    friend constexpr bool operator==(const wide_unsigned &a, const wide_unsigned &b)
    {
        for (unsigned i = 0; i < Limbs; ++i)
            if (a.limb[i] != b.limb[i])
                return false;
        return true;
    }
    friend constexpr bool operator!=(const wide_unsigned &a, const wide_unsigned &b) { return !(a == b); }
    friend constexpr bool operator<(const wide_unsigned &a, const wide_unsigned &b)
    {
        for (unsigned i = Limbs; i-- > 0;)
            if (a.limb[i] != b.limb[i])
                return a.limb[i] < b.limb[i];
        return false;
    }
    friend constexpr bool operator>(const wide_unsigned &a, const wide_unsigned &b) { return b < a; }
    friend constexpr bool operator<=(const wide_unsigned &a, const wide_unsigned &b) { return !(b < a); }
    friend constexpr bool operator>=(const wide_unsigned &a, const wide_unsigned &b) { return !(a < b); }

    // --- rule 2: an ordinary integer on either side, exact matches -----------------------

#define SATELLITE_WIDE_MIXED(op)                                                                              \
    template <std::integral Integer>                                                                          \
    friend constexpr auto operator op(const wide_unsigned &a, Integer b) { return a op wide_unsigned(b); }    \
    template <std::integral Integer>                                                                          \
    friend constexpr auto operator op(Integer a, const wide_unsigned &b) { return wide_unsigned(a) op b; }
    SATELLITE_WIDE_MIXED(+)
    SATELLITE_WIDE_MIXED(-)
    SATELLITE_WIDE_MIXED(*)
    SATELLITE_WIDE_MIXED(&)
    SATELLITE_WIDE_MIXED(|)
    SATELLITE_WIDE_MIXED(^)
    SATELLITE_WIDE_MIXED(==)
    SATELLITE_WIDE_MIXED(!=)
    SATELLITE_WIDE_MIXED(<)
    SATELLITE_WIDE_MIXED(>)
    SATELLITE_WIDE_MIXED(<=)
    SATELLITE_WIDE_MIXED(>=)
#undef SATELLITE_WIDE_MIXED

    // --- rule 3: no division ---------------------------------------------------------

    friend wide_unsigned operator/(const wide_unsigned &, const wide_unsigned &) = delete;
    friend wide_unsigned operator%(const wide_unsigned &, const wide_unsigned &) = delete;
    template <std::integral Integer> friend wide_unsigned operator/(const wide_unsigned &, Integer) = delete;
    template <std::integral Integer> friend wide_unsigned operator%(const wide_unsigned &, Integer) = delete;

    // --- the compound forms pcg writes: *= ^= |= &= += -= >>= <<= -------------------------

    constexpr wide_unsigned &operator+=(const wide_unsigned &o) { return *this = *this + o; }
    constexpr wide_unsigned &operator-=(const wide_unsigned &o) { return *this = *this - o; }
    constexpr wide_unsigned &operator*=(const wide_unsigned &o) { return *this = *this * o; }
    constexpr wide_unsigned &operator&=(const wide_unsigned &o) { return *this = *this & o; }
    constexpr wide_unsigned &operator|=(const wide_unsigned &o) { return *this = *this | o; }
    constexpr wide_unsigned &operator^=(const wide_unsigned &o) { return *this = *this ^ o; }
    template <std::integral Count> constexpr wide_unsigned &operator<<=(Count by) { return *this = *this << by; }
    template <std::integral Count> constexpr wide_unsigned &operator>>=(Count by) { return *this = *this >> by; }
};

} // namespace satellite004
