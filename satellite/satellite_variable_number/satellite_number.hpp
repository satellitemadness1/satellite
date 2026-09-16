#pragma once
// satellite/satellite_variable_number/satellite_number.hpp -- satellite.variable.number.
//
// (the author, 2026-09-14/15) "satellite_numbers are made by laying unsigned long
// long ints beside each other, and the sign is carried as a bool with the
// object", with "a fast path inside of satellite_number that checks if the
// number fits inside of an unsigned long long int, fast path for negative
// numbers as well".
//
// THE FAST PATH: a magnitude that fits one unsigned long long (0 ..
// 18,446,744,073,709,551,615), negative or not, is held inline: no allocation,
// and + - * on two such numbers never touch the heap unless the answer outgrows
// one limb. Only then does the number spread into more limbs, and it comes back
// to the fast path as soon as it fits again. Nothing ever wraps around.
//
// WHOLE NUMBERS. No upper limit but memory: the limb count is a size_t of a
// std::vector. Zero is never negative (-0 is 0). Division truncates toward zero
// and the remainder takes the dividend's sign, as C++ does (DESIGN decision
// recorded with this file; 003's rule may differ -- see the milestone notes).
//
// digits() and bytes() are satellite_numbers too (DESIGN §4), counted when asked
// rather than stored, so the fast path carries no second number.
//
// Every operation that can refuse answers a machine code (machine_codes.hpp).
//
// INLINE ON PURPOSE (measured, number_race.sh). The one-limb case of + - * and
// compare is written in this header, and forced inline ([[gnu::always_inline]]:
// clang 24 left += and + as calls), so the compiler sees it in the caller's loop;
// everything else is in satellite_number.cpp, satellite_number_divide.cpp and
// satellite_number_text.cpp. The representation is private: large_ is null on the
// fast path, so copying, moving and destroying a one-limb number is a test of one
// pointer and never calls the allocator. i = i + 1 went from 13.0 ns (clang 24)
// and 8.5 ns (g++ 17) with the vector held directly and every operation out of
// line, to 0.90 ns and 1.00 ns; C++ that refuses to wrap takes 0.61 and 0.33 ns.

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace satellite004 {

class satellite_number {
public:
    satellite_number() = default;                                        // 0
    satellite_number(unsigned long long int magnitude, bool negative = false) // the fast path
        : small_(magnitude), negative_(negative && magnitude != 0) {}
    static satellite_number from_signed(signed long long int value)
    {   // 0 - (unsigned) value is the magnitude of every negative value, the most negative included
        return value < 0 ? satellite_number(0ull - (unsigned long long int)value, true)
                         : satellite_number((unsigned long long int)value, false);
    }

    satellite_number(const satellite_number &other) : small_(other.small_), negative_(other.negative_)
    {
        if (other.large_ != nullptr) [[unlikely]]
            large_ = clone_large(other.large_);
    }
    satellite_number(satellite_number &&other) noexcept
        : small_(other.small_), large_(other.large_), negative_(other.negative_) { other.large_ = nullptr; }
    satellite_number &operator=(const satellite_number &other)
    {
        if (large_ != nullptr || other.large_ != nullptr) [[unlikely]] {
            if (&other == this)
                return *this;
            std::vector<unsigned long long int> *copy = other.large_ == nullptr ? nullptr : clone_large(other.large_);
            release_large(large_);
            large_ = copy;
        }
        small_ = other.small_;
        negative_ = other.negative_;
        return *this;
    }
    satellite_number &operator=(satellite_number &&other) noexcept
    {
        std::vector<unsigned long long int> *taken = other.large_;
        other.large_ = nullptr;       // first, so a = std::move(a) keeps its limbs
        if (large_ != nullptr) [[unlikely]]
            release_large(large_);
        small_ = other.small_;
        negative_ = other.negative_;
        large_ = taken;
        return *this;
    }
    // Inline even where the compiler would not: on an exception path an out-of-line
    // destructor takes the number's address, and that alone kept a loop's numbers
    // in memory (clang 24: 2.7 ns an i = i + 1 before, 0.98 ns after).
    [[gnu::always_inline]] ~satellite_number()
    {
        if (large_ != nullptr) [[unlikely]]
            release_large(large_);
    }

    // Decimal text: an optional '-' then one or more digits 0-9, nothing else
    // (no '+', spaces, separators or decimal point). Leading zeros are allowed.
    // Answers success, or int_error (3) with bad_offset at the first byte that
    // is not part of a number (0 for empty text, 1 for "-"); out is untouched.
    static signed long long int from_text(const std::string &text, satellite_number &out, std::size_t &bad_offset);
    std::string to_text() const;

    bool negative() const { return negative_; }
    bool is_zero() const { return large_ == nullptr && small_ == 0; }
    bool fits_one_limb() const { return large_ == nullptr; }
    std::size_t limb_count() const { return large_ == nullptr ? 1 : large_->size(); }
    unsigned long long int limb(std::size_t index) const; // least significant first; 0 past the top

    satellite_number digits() const; // decimal digits of the magnitude; 0 has 1
    satellite_number bytes() const;  // bytes the magnitude's limbs occupy: limb_count() * 8

    [[gnu::always_inline]] satellite_number &operator+=(const satellite_number &other)
    {
        // One test for "both one limb, same sign" (bitwise &, measured faster than &&).
        if ((large_ == nullptr) & (other.large_ == nullptr) & (negative_ == other.negative_)) [[likely]] {
            const unsigned long long int sum = small_ + other.small_;
            if (sum >= small_) [[likely]] { // no carry out of the limb
                small_ = sum;
                return *this;
            }
        } else if (large_ == nullptr && other.large_ == nullptr && small_ > other.small_) {
            small_ -= other.small_; // opposite signs: this sign stays, never 0
            return *this;
        }
        return slow(other, other.negative_, &add_slow);
    }
    [[gnu::always_inline]] satellite_number &operator-=(const satellite_number &other)
    {
        if ((large_ == nullptr) & (other.large_ == nullptr) & (negative_ != other.negative_)) [[likely]] {
            const unsigned long long int sum = small_ + other.small_; // a - (-b) is a + b
            if (sum >= small_) [[likely]] {
                small_ = sum;
                return *this;
            }
        } else if (large_ == nullptr && other.large_ == nullptr && small_ > other.small_) {
            small_ -= other.small_; // same signs: this sign stays, never 0
            return *this;
        }
        return slow(other, !other.negative_, &add_slow);
    }
    [[gnu::always_inline]] satellite_number &operator*=(const satellite_number &other)
    {
        if (large_ == nullptr && other.large_ == nullptr) [[likely]] {
            const unsigned __int128 product = (unsigned __int128)small_ * other.small_;
            if ((unsigned long long int)(product >> 64) == 0) [[likely]] {
                small_ = (unsigned long long int)product;
                negative_ = negative_ != other.negative_ && small_ != 0;
                return *this;
            }
        }
        return slow(other, other.negative_, &multiply_slow);
    }
    // The answer is built from both numbers' fields, so a one-limb left side is
    // never copied first; a left side that is a temporary lends its limbs instead.
    // The slow case is ONE out-of-line call given fields: an inline copy there
    // made g++ 17 spill the caller's loop counter to memory (2.4 ns an i = i + 1
    // against 1.0 ns, number_race.sh).
    [[gnu::always_inline]] friend satellite_number operator+(const satellite_number &left, const satellite_number &right)
    {
        if ((left.large_ == nullptr) & (right.large_ == nullptr) & (left.negative_ == right.negative_)) [[likely]] {
            const unsigned long long int sum = left.small_ + right.small_;
            if (sum >= left.small_) [[likely]]
                return satellite_number(sum, left.negative_, exact{});
        } else if (left.large_ == nullptr && right.large_ == nullptr && left.small_ > right.small_) {
            return satellite_number(left.small_ - right.small_, left.negative_, exact{});
        }
        return add_copy_slow(left.small_, left.large_, left.negative_, right.small_, right.large_, right.negative_);
    }
    [[gnu::always_inline]] friend satellite_number operator+(satellite_number &&left, const satellite_number &right) { left += right; return std::move(left); }
    [[gnu::always_inline]] friend satellite_number operator-(const satellite_number &left, const satellite_number &right)
    {
        if ((left.large_ == nullptr) & (right.large_ == nullptr) & (left.negative_ != right.negative_)) [[likely]] {
            const unsigned long long int sum = left.small_ + right.small_;
            if (sum >= left.small_) [[likely]]
                return satellite_number(sum, left.negative_, exact{});
        } else if (left.large_ == nullptr && right.large_ == nullptr && left.small_ > right.small_) {
            return satellite_number(left.small_ - right.small_, left.negative_, exact{});
        }
        return add_copy_slow(left.small_, left.large_, left.negative_, right.small_, right.large_, !right.negative_);
    }
    [[gnu::always_inline]] friend satellite_number operator-(satellite_number &&left, const satellite_number &right) { left -= right; return std::move(left); }
    [[gnu::always_inline]] friend satellite_number operator*(const satellite_number &left, const satellite_number &right)
    {
        if (left.large_ == nullptr && right.large_ == nullptr) [[likely]] {
            const unsigned __int128 product = (unsigned __int128)left.small_ * right.small_;
            if ((unsigned long long int)(product >> 64) == 0) [[likely]]
                return satellite_number((unsigned long long int)product, left.negative_ != right.negative_);
        }
        return multiply_copy_slow(left.small_, left.large_, left.negative_, right.small_, right.large_, right.negative_);
    }
    [[gnu::always_inline]] friend satellite_number operator*(satellite_number &&left, const satellite_number &right) { left *= right; return std::move(left); }
    satellite_number operator-() const;

    // quotient and remainder at once. Answers success or division_by_zero (22).
    // Any argument may be the same object as another. When quotient and remainder
    // are the same object it is written quotient first, so it holds the remainder.
    static signed long long int divide(const satellite_number &dividend, const satellite_number &divisor,
                                       satellite_number &quotient, satellite_number &remainder);

    // -1, 0 or 1.
    [[gnu::always_inline]] static int compare(const satellite_number &left, const satellite_number &right)
    {
        if (left.large_ == nullptr && right.large_ == nullptr) [[likely]] {
            if (left.negative_ != right.negative_) // zero is never negative
                return left.negative_ ? -1 : 1;
            if (left.small_ == right.small_)
                return 0;
            return (left.small_ < right.small_) != left.negative_ ? -1 : 1;
        }
        return compare_slow(left.small_, left.large_, left.negative_, right.small_, right.large_, right.negative_);
    }
    friend bool operator==(const satellite_number &l, const satellite_number &r) { return compare(l, r) == 0; }
    friend bool operator!=(const satellite_number &l, const satellite_number &r) { return compare(l, r) != 0; }
    friend bool operator<(const satellite_number &l, const satellite_number &r) { return compare(l, r) < 0; }
    friend bool operator<=(const satellite_number &l, const satellite_number &r) { return compare(l, r) <= 0; }
    friend bool operator>(const satellite_number &l, const satellite_number &r) { return compare(l, r) > 0; }
    friend bool operator>=(const satellite_number &l, const satellite_number &r) { return compare(l, r) >= 0; }

private:
    struct exact {}; // the fields are already right: no zero-sign test
    satellite_number(unsigned long long int magnitude, bool negative, exact) : small_(magnitude), negative_(negative) {}

    unsigned long long int small_ = 0;                     // the magnitude while large_ is null
    std::vector<unsigned long long int> *large_ = nullptr; // otherwise, owned: 2 or more limbs, the top one non-zero
    bool negative_ = false;

    // THE SLOW PATHS NEVER SEE THE CALLER'S OBJECTS. An out-of-line call given the
    // address of a loop's own number makes the compiler keep that number in memory
    // on the fast path too (number_race.sh, i = i + 1 with g++ 17: 5.2 ns while +=
    // passed this, 1.3 ns once it did not). So slow() hands a static function the
    // fields of both numbers by value -- the other's read first, since other may
    // be *this -- and the answer comes back by value. This number's limbs pass to
    // the slow path, which owns them from then: if memory runs out (std::bad_alloc)
    // the number is left a valid number, but not its old value.
    using slow_path = satellite_number (*)(unsigned long long int, std::vector<unsigned long long int> *, bool,
                                           unsigned long long int, const std::vector<unsigned long long int> *, bool);
    satellite_number &slow(const satellite_number &other, bool other_negative, slow_path path)
    {
        const unsigned long long int other_small = other.small_;
        const std::vector<unsigned long long int> *other_large = other.large_;
        std::vector<unsigned long long int> *mine = large_;
        large_ = nullptr;
        *this = path(small_, mine, negative_, other_small, other_large, other_negative);
        return *this;
    }
    static satellite_number add_slow(unsigned long long int small, std::vector<unsigned long long int> *large, bool negative,
                                     unsigned long long int other_small, const std::vector<unsigned long long int> *other_large,
                                     bool other_negative);
    static satellite_number multiply_slow(unsigned long long int small, std::vector<unsigned long long int> *large, bool negative,
                                          unsigned long long int other_small, const std::vector<unsigned long long int> *other_large,
                                          bool other_negative);
    // For + - * of two numbers neither of which may change: copy the left one's
    // limbs (if any), then as add_slow / multiply_slow. Subtraction passes the
    // right number's sign flipped.
    static satellite_number add_copy_slow(unsigned long long int small, const std::vector<unsigned long long int> *large, bool negative,
                                          unsigned long long int other_small, const std::vector<unsigned long long int> *other_large,
                                          bool other_negative);
    static satellite_number multiply_copy_slow(unsigned long long int small, const std::vector<unsigned long long int> *large,
                                               bool negative, unsigned long long int other_small,
                                               const std::vector<unsigned long long int> *other_large, bool other_negative);
    void add_in_place(unsigned long long int other_small, const std::vector<unsigned long long int> *other_large, bool other_negative);
    void multiply_in_place(unsigned long long int other_small, const std::vector<unsigned long long int> *other_large, bool other_negative);
    static int compare_slow(unsigned long long int left_small, const std::vector<unsigned long long int> *left_large, bool left_negative,
                            unsigned long long int right_small, const std::vector<unsigned long long int> *right_large, bool right_negative);
    static std::vector<unsigned long long int> *clone_large(const std::vector<unsigned long long int> *large);
    static void release_large(std::vector<unsigned long long int> *large) noexcept; // delete; null is allowed

    struct limb_run; // a view of another number's limbs (satellite_number.cpp)
    const unsigned long long int *limb_data() const { return large_ == nullptr ? &small_ : large_->data(); }
    void grow_to(std::size_t count);  // held as large_ with count limbs, zeros above (slow paths only)
    void settle();                    // no zero top limbs, one limb back inline, and 0 never negative
    void adopt(std::vector<unsigned long long int> &&limbs, bool negative);
    void add_magnitude(const limb_run &other);           // |this| += |other|
    void subtract_magnitude(const limb_run &other);      // |this| -= |other|, |this| >= |other|
    void subtract_from_magnitude(const limb_run &other); // |this| = |other| - |this|, |other| > |this|
    static int compare_magnitude(const satellite_number &left, const satellite_number &right);
};

} // namespace satellite004
