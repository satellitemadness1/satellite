#pragma once

// The satellite exact arbitrary-precision decimal number -- Milestone 7 Prototype.
//
// DESIGN §8.1: Exact decimal arithmetic, base-10^9 limbs.
// Explicit `positive` bool (defaults to true), magnitude never carries a sign.
// Deleted float constructors prevent silent truncation at compile time.

#include <climits>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace satellite {

// Base-10^9 limb container for arbitrary precision
class BigInt {
public:
    static constexpr uint32_t BASE = 1000000000; // 10^9
    static constexpr int DIGITS_PER_LIMB = 9;

    std::vector<uint32_t> limbs; // Little-endian: limbs[0] is least significant

    BigInt() = default;
    explicit BigInt(uint64_t val);

    static BigInt from_u64(uint64_t val);
    static BigInt from_string(const std::string &digits);

    bool is_zero() const;
    int compare(const BigInt &other) const;

    BigInt add(const BigInt &other) const;
    BigInt sub(const BigInt &other) const; // Requires *this >= other
    BigInt mul(const BigInt &other) const;
    BigInt mul_u32(uint32_t v) const;
    void div_mod(const BigInt &divisor, BigInt &quotient, BigInt &remainder) const;

    std::string to_digits() const;
    bool to_u64(uint64_t &out) const;
};

class Number {
public:
    static constexpr int DEFAULT_DIVISION_DIGITS = 34;
    static constexpr int MAX_DIVISION_DIGITS = 10000;

    Number() : sig_(0), exp_(0), positive(true), big_(nullptr) {}

    template <class T,
              class = std::enable_if_t<
                  std::is_integral_v<T> &&
                  !std::is_same_v<std::remove_cv_t<T>, bool>>>
    Number(T value)
    {
        if constexpr (std::is_signed_v<T>) {
            const long long val = static_cast<long long>(value);
            if (val == 0) {
                positive = true;
                sig_ = 0;
                exp_ = 0;
            } else if (val < 0) {
                positive = false;
                if (val == LLONG_MIN) {
                    sig_ = 0;
                    exp_ = 0;
                    big_ = std::make_shared<const BigInt>(
                        BigInt::from_u64(static_cast<uint64_t>(LLONG_MAX) + 1));
                } else {
                    sig_ = -val;
                    exp_ = 0;
                }
            } else {
                positive = true;
                sig_ = val;
                exp_ = 0;
            }
        } else {
            const uint64_t uval = static_cast<uint64_t>(value);
            positive = true;
            exp_ = 0;
            if (uval <= static_cast<uint64_t>(LLONG_MAX)) {
                sig_ = static_cast<long long>(uval);
            } else {
                sig_ = 0;
                big_ = std::make_shared<const BigInt>(BigInt::from_u64(uval));
            }
        }
    }

    Number(float) = delete;
    Number(double) = delete;
    Number(long double) = delete;

    static Number from_u64(uint64_t val);
    static bool parse(const std::string &text, Number &out);

    std::string to_string() const;

    bool is_zero() const;
    bool is_negative() const { return !positive && !is_zero(); }
    bool is_positive() const { return positive && !is_zero(); }

    bool is_integer() const;
    bool to_integer(long long &out) const;

    Number negated() const;
    Number abs() const;

    static int compare(const Number &a, const Number &b);

    bool operator==(const Number &other) const { return compare(*this, other) == 0; }
    bool operator!=(const Number &other) const { return compare(*this, other) != 0; }
    bool operator<(const Number &other) const { return compare(*this, other) < 0; }
    bool operator<=(const Number &other) const { return compare(*this, other) <= 0; }
    bool operator>(const Number &other) const { return compare(*this, other) > 0; }
    bool operator>=(const Number &other) const { return compare(*this, other) >= 0; }

    Number add(const Number &other) const;
    Number sub(const Number &other) const;
    Number mul(const Number &other) const;
    Number div(const Number &other, int digits = DEFAULT_DIVISION_DIGITS) const;

    Number operator+(const Number &other) const { return add(other); }
    Number operator-(const Number &other) const { return sub(other); }
    Number operator*(const Number &other) const { return mul(other); }
    Number operator/(const Number &other) const { return div(other); }

public:
    long long sig_ = 0;                  // 8 bytes: magnitude when big_ is null (offset 0..7)
    int exp_ = 0;                        // 4 bytes: exponent in base 10 (offset 8..11)
    bool positive = true;                // 1 byte: sign bool (offset 12)
    // 3 bytes padding (offset 13..15)
    std::shared_ptr<const BigInt> big_;  // 16 bytes: arbitrary precision limbs (offset 16..31)

private:
    BigInt get_magnitude() const;
    static Number make_from_bigint(bool pos, const BigInt &mag, int exponent);
    void normalize();
};

static_assert(sizeof(void *) != 8 || sizeof(Number) == 32,
              "Number must be 32 bytes on 64-bit to keep sizeof(Value) == 40");

} // namespace satellite
