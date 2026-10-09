// satellite/satellite_variable_number/satellite_number.cpp -- satellite.variable.number:
// the slow paths of copying, + - * and compare, and limbs and bytes. The fast
// paths (one limb each side) are inline in satellite_number.hpp. Division is in
// satellite_number_divide.cpp and decimal text in satellite_number_text.cpp, so
// each file stays near the author's 300-line target.
//
// A slow path is reached only when a number has more than one limb or a one-limb
// answer is not simple (a carry out of the limb, a sign that changes). A one-limb
// answer still never touches the heap; only an answer that outgrows one limb
// spreads into large_, and settle() brings it back inline once it fits again.
//
// ALIASING. a += a, a -= a and a *= a are all correct. The inline slow() in the
// header passes both numbers' fields here, so for a += a the other number's
// large_ is the very vector the working number owns: the magnitude loops look its
// storage up after any growth, read each limb before writing the same limb, and
// multiplication writes a fresh run.

#include "satellite_number.hpp"
#include "satellite_number_limbs.hpp"

#include <utility>

namespace satellite004 {

using number_limbs::double_limb;
using number_limbs::limb_type;

// Another number's limbs, seen through its fields: its own small_ is copied into
// the slow path's frame, and large_ is looked up afresh on every data() call.
struct satellite_number::limb_run {
    const limb_type *small;
    const std::vector<limb_type> *large;
    const limb_type *data() const { return large == nullptr ? small : large->data(); }
    std::size_t count() const { return large == nullptr ? 1 : large->size(); }
};

std::vector<limb_type> *satellite_number::clone_large(const std::vector<limb_type> *large)
{
    return new std::vector<limb_type>(*large);
}

void satellite_number::release_large(std::vector<limb_type> *large) noexcept
{
    delete large;
}

unsigned long long int satellite_number::limb(std::size_t index) const
{
    if (large_ == nullptr)
        return index == 0 ? small_ : 0;
    return index < large_->size() ? (*large_)[index] : 0;
}

satellite_number satellite_number::bytes() const
{
    satellite_number count((unsigned long long int)limb_count());
    count *= satellite_number(8); // exact even for a count near 2^61 limbs
    return count;
}

void satellite_number::grow_to(std::size_t count)
{
    if (large_ == nullptr) {
        large_ = new std::vector<limb_type>(count, 0);
        (*large_)[0] = small_;
        small_ = 0;
    } else if (large_->size() < count) {
        large_->resize(count, 0);
    }
}

void satellite_number::settle()
{
    if (large_ != nullptr) {
        while (!large_->empty() && large_->back() == 0)
            large_->pop_back();
        if (large_->size() <= 1) {
            small_ = large_->empty() ? 0 : (*large_)[0];
            release_large(large_); // give the memory back
            large_ = nullptr;
        }
    }
    if (is_zero())
        negative_ = false;
}

void satellite_number::adopt(std::vector<limb_type> &&limbs, bool negative)
{
    while (!limbs.empty() && limbs.back() == 0)
        limbs.pop_back();
    negative_ = negative;
    if (limbs.size() <= 1) {
        small_ = limbs.empty() ? 0 : limbs[0];
        release_large(large_);
        large_ = nullptr;
    } else {
        small_ = 0;
        if (large_ == nullptr)
            large_ = new std::vector<limb_type>(std::move(limbs));
        else
            *large_ = std::move(limbs);
    }
    if (is_zero())
        negative_ = false;
}

int satellite_number::compare_magnitude(const satellite_number &left, const satellite_number &right)
{
    return number_limbs::compare_runs(left.limb_data(), left.limb_count(), right.limb_data(), right.limb_count());
}

int satellite_number::compare_slow(limb_type left_small, const std::vector<limb_type> *left_large, bool left_negative,
                                   limb_type right_small, const std::vector<limb_type> *right_large, bool right_negative)
{
    if (left_negative != right_negative) // zero is never negative, so the negative one is smaller
        return left_negative ? -1 : 1;
    const limb_run left{&left_small, left_large}, right{&right_small, right_large};
    const int magnitude = number_limbs::compare_runs(left.data(), left.count(), right.data(), right.count());
    return left_negative ? -magnitude : magnitude;
}

satellite_number satellite_number::operator-() const
{
    satellite_number negated(*this);
    if (!negated.is_zero())
        negated.negative_ = !negated.negative_;
    return negated;
}

// |this| += |other|. Each limb of other is read before the same limb of this is
// written, and other's storage is looked up after grow_to, so other may be this number.
void satellite_number::add_magnitude(const limb_run &other)
{
    const std::size_t other_count = other.count();
    const std::size_t count = limb_count();
    grow_to((count > other_count ? count : other_count) + 1);
    const limb_type *from = other.data();
    limb_type *to = large_->data();
    limb_type carry = 0;
    std::size_t index = 0;
    for (; index < other_count; index++)
        to[index] = number_limbs::add_with_carry(to[index], from[index], carry);
    for (; carry != 0; index++) // the top limb grow_to added always stops the carry
        to[index] = number_limbs::add_with_carry(to[index], 0, carry);
    settle();
}

// |this| -= |other| where |this| >= |other|; other may be this number (the answer is 0).
void satellite_number::subtract_magnitude(const limb_run &other)
{
    if (large_ == nullptr) { // then other is one limb too: no heap
        small_ -= *other.small;
        settle();
        return;
    }
    const std::size_t other_count = other.count();
    const limb_type *from = other.data();
    limb_type *to = large_->data();
    limb_type borrow = 0;
    std::size_t index = 0;
    for (; index < other_count; index++)
        to[index] = number_limbs::subtract_with_borrow(to[index], from[index], borrow);
    for (; borrow != 0; index++) // |this| >= |other|, so a higher limb always stops the borrow
        to[index] = number_limbs::subtract_with_borrow(to[index], 0, borrow);
    settle();
}

// |this| = |other| - |this| where |other| > |this|, so other is never this number.
void satellite_number::subtract_from_magnitude(const limb_run &other)
{
    if (other.large == nullptr) { // then this is one limb too: no heap
        small_ = *other.small - small_;
        settle();
        return;
    }
    const std::size_t other_count = other.count();
    grow_to(other_count); // zeros above this number's own limbs
    const limb_type *from = other.data();
    limb_type *to = large_->data();
    limb_type borrow = 0;
    for (std::size_t index = 0; index < other_count; index++)
        to[index] = number_limbs::subtract_with_borrow(from[index], to[index], borrow);
    settle();
}

// this + other, where other counts as negative when other_negative is set: += passes
// other's own sign, -= the opposite. Everything the inline paths leave comes here.
// The static slow paths the header's slow() calls: the working number takes over
// large (the caller has let go of it), works in place, and is returned.
satellite_number satellite_number::add_slow(limb_type small, std::vector<limb_type> *large, bool negative, limb_type other_small,
                                            const std::vector<limb_type> *other_large, bool other_negative)
{
    satellite_number work;
    work.small_ = small;
    work.large_ = large;
    work.negative_ = negative;
    work.add_in_place(other_small, other_large, other_negative);
    return work;
}

satellite_number satellite_number::multiply_slow(limb_type small, std::vector<limb_type> *large, bool negative,
                                                 limb_type other_small, const std::vector<limb_type> *other_large,
                                                 bool other_negative)
{
    satellite_number work;
    work.small_ = small;
    work.large_ = large;
    work.negative_ = negative;
    work.multiply_in_place(other_small, other_large, other_negative);
    return work;
}

satellite_number satellite_number::add_copy_slow(limb_type small, const std::vector<limb_type> *large, bool negative,
                                                 limb_type other_small, const std::vector<limb_type> *other_large, bool other_negative)
{
    return add_slow(small, large == nullptr ? nullptr : clone_large(large), negative, other_small, other_large, other_negative);
}

satellite_number satellite_number::multiply_copy_slow(limb_type small, const std::vector<limb_type> *large, bool negative,
                                                      limb_type other_small, const std::vector<limb_type> *other_large,
                                                      bool other_negative)
{
    return multiply_slow(small, large == nullptr ? nullptr : clone_large(large), negative, other_small, other_large, other_negative);
}

void satellite_number::add_in_place(limb_type other_small, const std::vector<limb_type> *other_large, bool other_negative)
{
    if (other_large == nullptr && other_small == 0)
        return;
    const limb_run other{&other_small, other_large};
    if (negative_ == other_negative) {
        add_magnitude(other);
        return;
    }
    if (number_limbs::compare_runs(limb_data(), limb_count(), other.data(), other.count()) >= 0) {
        subtract_magnitude(other); // keeps this sign; settle() clears it at 0
    } else {
        subtract_from_magnitude(other);
        negative_ = other_negative;
    }
}

void satellite_number::multiply_in_place(limb_type other_small, const std::vector<limb_type> *other_large, bool other_negative)
{
    const bool sign = negative_ != other_negative;
    const limb_run other{&other_small, other_large};
    // Schoolbook: every limb of one number times every limb of the other, into a
    // fresh run, so other may be *this. Two one-limb numbers come here only when
    // the product needs two limbs.
    const std::size_t count = limb_count(), other_count = other.count();
    const limb_type *left = limb_data(), *right = other.data();
    std::vector<limb_type> product(count + other_count, 0);
    for (std::size_t i = 0; i < count; i++) {
        const limb_type multiplier = left[i];
        if (multiplier == 0)
            continue;
        limb_type carry = 0;
        for (std::size_t j = 0; j < other_count; j++) {
            const double_limb step = (double_limb)multiplier * right[j] + product[i + j] + carry;
            product[i + j] = (limb_type)step;
            carry = (limb_type)(step >> 64);
        }
        product[i + other_count] = carry;
    }
    adopt(std::move(product), sign);
}

} // namespace satellite004
