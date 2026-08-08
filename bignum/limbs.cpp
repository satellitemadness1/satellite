// Base-10^9 limb arithmetic — everything underneath Number.
//
// Part of bignum/, split from an 855-line bignum.cpp.

#include "bignum_internal.hpp"

namespace satellite {

// v * 10^k, refusing rather than wrapping. Declared in bignum_internal.hpp
// because compare() and add() now live in different files.
bool scale_ll(long long v, int k, long long &out)
{
    if (k < 0 || k > MAX_POW10)
        return false;
    return !__builtin_mul_overflow(v, POW10[k], &out);
}

// ---------------------------------------------------------------------------
// BigInt
// ---------------------------------------------------------------------------

void BigInt::trim()
{
    while (!limbs_.empty() && limbs_.back() == 0)
        limbs_.pop_back();
}

BigInt BigInt::from_u64(unsigned long long value)
{
    BigInt out;
    while (value) {
        out.limbs_.push_back(static_cast<unsigned>(value % BASE));
        value /= BASE;
    }
    return out;
}

BigInt BigInt::from_digits(const char *digits, size_t count)
{
    // Skip leading zeros so the limb count reflects the value rather than the
    // spelling; "0000000000000000001" is one limb, not three.
    size_t begin = 0;
    while (begin < count && digits[begin] == '0')
        begin++;

    BigInt out;
    if (begin == count)
        return out;

    // Right to left in groups of BASE_DIGITS, because the least significant
    // group is the one that has to land whole in limb 0.
    size_t end = count;
    while (end > begin) {
        const size_t start =
            end >= begin + BASE_DIGITS ? end - BASE_DIGITS : begin;
        unsigned limb = 0;
        for (size_t i = start; i < end; i++)
            limb = limb * 10 + static_cast<unsigned>(digits[i] - '0');
        out.limbs_.push_back(limb);
        end = start;
    }
    out.trim();
    return out;
}

size_t BigInt::digit_count() const
{
    if (limbs_.empty())
        return 1;   // "0" is one digit
    size_t count = (limbs_.size() - 1) * BASE_DIGITS;
    unsigned top = limbs_.back();
    while (top) {
        count++;
        top /= 10;
    }
    return count;
}

int BigInt::compare(const BigInt &a, const BigInt &b)
{
    if (a.limbs_.size() != b.limbs_.size())
        return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    for (size_t i = a.limbs_.size(); i-- > 0;)
        if (a.limbs_[i] != b.limbs_[i])
            return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
    return 0;
}

BigInt BigInt::add(const BigInt &a, const BigInt &b)
{
    BigInt out;
    const size_t n = std::max(a.limbs_.size(), b.limbs_.size());
    out.limbs_.reserve(n + 1);

    unsigned carry = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned sum = carry;
        if (i < a.limbs_.size())
            sum += a.limbs_[i];
        if (i < b.limbs_.size())
            sum += b.limbs_[i];
        // No overflow to worry about: two limbs plus a carry is under 2*10^9+1,
        // which is well inside an unsigned.
        carry = sum >= BASE ? 1 : 0;
        out.limbs_.push_back(carry ? sum - BASE : sum);
    }
    if (carry)
        out.limbs_.push_back(carry);
    return out;
}

BigInt BigInt::sub(const BigInt &a, const BigInt &b)
{
    BigInt out;
    out.limbs_.reserve(a.limbs_.size());

    int borrow = 0;
    for (size_t i = 0; i < a.limbs_.size(); i++) {
        long long value = static_cast<long long>(a.limbs_[i]) - borrow;
        if (i < b.limbs_.size())
            value -= b.limbs_[i];
        borrow = value < 0 ? 1 : 0;
        if (borrow)
            value += BASE;
        out.limbs_.push_back(static_cast<unsigned>(value));
    }
    out.trim();
    return out;
}

BigInt BigInt::mul_small(const BigInt &a, unsigned m)
{
    BigInt out;
    if (a.limbs_.empty() || m == 0)
        return out;

    out.limbs_.reserve(a.limbs_.size() + 1);
    unsigned long long carry = 0;
    for (unsigned limb : a.limbs_) {
        const unsigned long long product =
            static_cast<unsigned long long>(limb) * m + carry;
        out.limbs_.push_back(static_cast<unsigned>(product % BASE));
        carry = product / BASE;
    }
    while (carry) {
        out.limbs_.push_back(static_cast<unsigned>(carry % BASE));
        carry /= BASE;
    }
    out.trim();
    return out;
}

BigInt BigInt::mul(const BigInt &a, const BigInt &b)
{
    BigInt out;
    if (a.limbs_.empty() || b.limbs_.empty())
        return out;

    // Schoolbook. The intermediate is a uint64 and every partial product is
    // under 10^18, which is the reason base 10^9 was chosen over base 10^18.
    std::vector<unsigned long long> acc(a.limbs_.size() + b.limbs_.size(), 0);
    for (size_t i = 0; i < a.limbs_.size(); i++) {
        unsigned long long carry = 0;
        for (size_t j = 0; j < b.limbs_.size(); j++) {
            const unsigned long long product =
                static_cast<unsigned long long>(a.limbs_[i]) * b.limbs_[j] +
                acc[i + j] + carry;
            acc[i + j] = product % BASE;
            carry = product / BASE;
        }
        size_t at = i + b.limbs_.size();
        while (carry) {
            const unsigned long long product = acc[at] + carry;
            acc[at] = product % BASE;
            carry = product / BASE;
            at++;
        }
    }

    out.limbs_.reserve(acc.size());
    for (unsigned long long limb : acc)
        out.limbs_.push_back(static_cast<unsigned>(limb));
    out.trim();
    return out;
}

BigInt BigInt::mul_pow10(const BigInt &a, unsigned k)
{
    if (a.limbs_.empty())
        return BigInt();

    // A whole limb per BASE_DIGITS is a shift, not a multiply — the one thing
    // base 10^9 exists to make free.
    BigInt out;
    const unsigned whole = k / BASE_DIGITS;
    const unsigned rest = k % BASE_DIGITS;

    out.limbs_.assign(whole, 0u);
    out.limbs_.insert(out.limbs_.end(), a.limbs_.begin(), a.limbs_.end());

    if (rest) {
        unsigned factor = 1;
        for (unsigned i = 0; i < rest; i++)
            factor *= 10;
        out = mul_small(out, factor);
    }
    return out;
}

void BigInt::divmod(const BigInt &a, const BigInt &b, BigInt &q, BigInt &r)
{
    q = BigInt();
    r = BigInt();
    if (b.limbs_.empty() || compare(a, b) < 0) {
        r = a;
        return;
    }

    // Long division one DECIMAL digit at a time, rather than Knuth's algorithm
    // D in base 10^9. The inner loop runs at most nine times because the
    // remainder is always under the divisor before the digit is brought down,
    // so the cost is nine limb subtractions per digit — and getting nine
    // comparisons right is a different proposition from getting base-10^9
    // quotient estimation and its correction step right.
    const std::string digits = a.to_digits();
    std::string quotient;
    quotient.reserve(digits.size());

    BigInt remainder;
    for (char c : digits) {
        remainder = mul_small(remainder, 10);
        remainder = add(remainder, from_u64(static_cast<unsigned>(c - '0')));

        unsigned digit = 0;
        while (compare(remainder, b) >= 0) {
            remainder = sub(remainder, b);
            digit++;
        }
        quotient.push_back(static_cast<char>('0' + digit));
    }

    q = from_digits(quotient.data(), quotient.size());
    r = remainder;
}

std::string BigInt::to_digits() const
{
    if (limbs_.empty())
        return "0";

    std::string out = std::to_string(limbs_.back());
    out.reserve(out.size() + (limbs_.size() - 1) * BASE_DIGITS);
    for (size_t i = limbs_.size() - 1; i-- > 0;) {
        // Every limb but the most significant is zero-padded to its full width;
        // dropping the padding would concatenate 1 and 000000002 as 12.
        const std::string limb = std::to_string(limbs_[i]);
        out.append(BASE_DIGITS - limb.size(), '0');
        out += limb;
    }
    return out;
}

bool BigInt::fits_ll() const
{
    if (limbs_.size() > 3)
        return false;
    unsigned long long value = 0;
    for (size_t i = limbs_.size(); i-- > 0;) {
        if (value > (ULLONG_MAX - limbs_[i]) / BASE)
            return false;
        value = value * BASE + limbs_[i];
    }
    return value <= static_cast<unsigned long long>(LLONG_MAX);
}

long long BigInt::to_ll() const
{
    unsigned long long value = 0;
    for (size_t i = limbs_.size(); i-- > 0;)
        value = value * BASE + limbs_[i];
    return static_cast<long long>(value);
}

} // namespace satellite
