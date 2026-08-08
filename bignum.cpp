#include "bignum.hpp"

#include <algorithm>
#include <cstdlib>

namespace satellite {
namespace {

// 10^k for k in [0, 18], which is every power of ten a long long holds.
constexpr long long POW10[] = {
    1LL,
    10LL,
    100LL,
    1000LL,
    10000LL,
    100000LL,
    1000000LL,
    10000000LL,
    100000000LL,
    1000000000LL,
    10000000000LL,
    100000000000LL,
    1000000000000LL,
    10000000000000LL,
    100000000000000LL,
    1000000000000000LL,
    10000000000000000LL,
    100000000000000000LL,
    1000000000000000000LL,
};
constexpr int MAX_POW10 = 18;

// v * 10^k, refusing rather than wrapping. The refusal is what sends the
// operation down the BigInt path instead of producing a wrong answer.
bool scale_ll(long long v, int k, long long &out)
{
    if (k < 0 || k > MAX_POW10)
        return false;
    return !__builtin_mul_overflow(v, POW10[k], &out);
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

} // namespace

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

// ---------------------------------------------------------------------------
// Number
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Number
// ---------------------------------------------------------------------------

int Number::sign() const
{
    if (big_)
        return sig_ < 0 ? -1 : 1;
    return sig_ < 0 ? -1 : (sig_ > 0 ? 1 : 0);
}

BigInt Number::magnitude() const
{
    if (big_)
        return *big_;
    // Taken in unsigned, because -LLONG_MIN has no long long to land in. The
    // constructor keeps LLONG_MIN out of sig_, so this is belt and braces
    // rather than the load-bearing guard.
    const unsigned long long wide =
        sig_ < 0 ? 0ULL - static_cast<unsigned long long>(sig_)
                 : static_cast<unsigned long long>(sig_);
    return BigInt::from_u64(wide);
}

BigInt Number::scaled_magnitude(int target) const
{
    const BigInt mag = magnitude();
    if (exp_ == target || mag.is_zero())
        return mag;
    return BigInt::mul_pow10(mag, static_cast<unsigned>(exp_ - target));
}

Number Number::make(int sign, const BigInt &magnitude, long long exponent)
{
    Number out;
    if (magnitude.is_zero())
        return out;   // zero has no sign, and no exponent worth keeping

    // The exponent is an int, by §8.1's 32-byte layout. Nothing a program can
    // write reaches the ends of it — a literal is bounded by the length of the
    // source and a division by division_digits — so this clamps rather than
    // wraps, and being wrong at 10^2147483647 is a trade worth making.
    if (exponent > INT_MAX)
        exponent = INT_MAX;
    if (exponent < INT_MIN)
        exponent = INT_MIN;
    out.exp_ = static_cast<int>(exponent);

    if (magnitude.fits_ll()) {
        const long long value = magnitude.to_ll();
        out.sig_ = sign < 0 ? -value : value;
        return out;
    }
    out.sig_ = sign;
    out.big_ = std::make_shared<const BigInt>(magnitude);
    return out;
}

bool Number::small_parts(long long &sig, int &exp) const
{
    // The one test that matters. Above, make() stores the SIGN in sig_ when it
    // stores a BigInt, so a caller reading sig_ without checking big_ would get
    // 1 or -1 as the value of every large number.
    if (big_)
        return false;
    sig = sig_;
    exp = exp_;
    return true;
}

Number Number::from_small(long long sig, int exp)
{
    // LLONG_MIN has no positive counterpart, so negated() on it is undefined.
    // The integral constructor sends it big for exactly this reason, and this
    // function has to agree: it takes a caller-supplied long long, so it can be
    // handed a value make() would never produce (fits_ll() caps the magnitude
    // at LLONG_MAX, so make() cannot).
    //
    // Being the inverse of small_parts is not enough here. Round-tripping is
    // preserved either way, and the invariant that every other operation relies
    // on — that sig_ can be negated — is only preserved by promoting.
    if (sig == LLONG_MIN)
        return make(-1,
                    BigInt::from_u64(static_cast<unsigned long long>(LLONG_MAX) + 1),
                    exp);

    Number out;
    out.sig_ = sig;
    // Zero carries no exponent. make() drops it ("zero has no sign, and no
    // exponent worth keeping"), so a from_small that kept one would build a
    // Number that no other path can produce, and compare() and to_string()
    // would both then be reasoning about a shape they have never seen.
    out.exp_ = sig == 0 ? 0 : exp;
    return out;
}

Number Number::from_u64(unsigned long long value)
{
    return make(1, BigInt::from_u64(value), 0);
}

bool Number::parse(const std::string &text, Number &out)
{
    size_t i = 0;
    int sign = 1;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        sign = text[i] == '-' ? -1 : 1;
        i++;
    }

    // The digits of the significand, with the decimal point turned into an
    // exponent: "3.14" is 314 times 10^-2, which is already the representation.
    // Nothing is converted and nothing is rounded, which is the whole point.
    std::string digits;
    long long exponent = 0;
    bool any = false;

    for (; i < text.size() && is_digit(text[i]); i++, any = true)
        digits += text[i];

    if (i < text.size() && text[i] == '.') {
        i++;
        for (; i < text.size() && is_digit(text[i]); i++, any = true) {
            digits += text[i];
            exponent--;
        }
    }
    if (!any)
        return false;

    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        i++;
        int exp_sign = 1;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
            exp_sign = text[i] == '-' ? -1 : 1;
            i++;
        }
        if (i >= text.size() || !is_digit(text[i]))
            return false;
        long long value = 0;
        for (; i < text.size() && is_digit(text[i]); i++) {
            value = value * 10 + (text[i] - '0');
            if (value > INT_MAX)
                value = INT_MAX;      // make() clamps the total anyway
        }
        exponent += exp_sign * value;
    }
    if (i != text.size())
        return false;

    out = make(sign, BigInt::from_digits(digits.data(), digits.size()),
               exponent);
    return true;
}

void Number::normalized(std::string &digits, long long &exponent) const
{
    if (big_) {
        digits = big_->to_digits();
    } else {
        const unsigned long long wide =
            sig_ < 0 ? 0ULL - static_cast<unsigned long long>(sig_)
                     : static_cast<unsigned long long>(sig_);
        digits = std::to_string(wide);
    }
    exponent = exp_;

    if (digits == "0") {
        exponent = 0;
        return;
    }

    // Trailing zeros are representation, not value: 2.50 and 2.5 are the same
    // number and only one of them should print. Stripping here rather than
    // after every operation is what keeps add and multiply allocation-free.
    size_t end = digits.size();
    while (end > 1 && digits[end - 1] == '0') {
        end--;
        exponent++;
    }
    digits.resize(end);
}

bool Number::is_integer() const
{
    if (exp_ >= 0)
        return true;
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    return exponent >= 0 || digits == "0";
}

bool Number::to_integer(long long &out) const
{
    if (!is_integer())
        return false;

    // The fast path is every index, every slice bound and every loop counter a
    // program actually writes.
    if (!big_ && exp_ == 0) {
        out = sig_;
        return true;
    }

    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (digits == "0") {
        out = 0;
        return true;
    }
    if (exponent > 32)      // far past anything a long long holds
        return false;

    BigInt mag = BigInt::from_digits(digits.data(), digits.size());
    if (exponent > 0)
        mag = BigInt::mul_pow10(mag, static_cast<unsigned>(exponent));
    if (!mag.fits_ll())
        return false;

    const long long value = mag.to_ll();
    out = sign() < 0 ? -value : value;
    return true;
}

Number Number::abs() const
{
    Number out = *this;
    if (big_)
        out.sig_ = 1;
    else if (out.sig_ < 0)
        out.sig_ = -out.sig_;
    return out;
}

Number Number::negated() const
{
    Number out = *this;
    out.sig_ = -out.sig_;
    return out;
}

Number Number::floor() const { return rounded(Rounding::Floor); }
Number Number::ceil() const { return rounded(Rounding::Ceil); }
Number Number::round() const { return rounded(Rounding::HalfAwayFromZero); }

Number Number::rounded(Rounding mode) const
{
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (exponent >= 0 || digits == "0")
        return *this;

    // The fractional digits are exactly the last (-exponent) of the
    // significand, so dropping the fraction is a decimal-string split and the
    // three modes differ only in whether what was dropped bumps the magnitude.
    const size_t drop = static_cast<size_t>(-exponent);
    const int s = sign();

    std::string whole;
    std::string fraction;
    if (drop >= digits.size()) {
        whole = "0";
        fraction = std::string(drop - digits.size(), '0') + digits;
    } else {
        whole = digits.substr(0, digits.size() - drop);
        fraction = digits.substr(digits.size() - drop);
    }

    const bool has_fraction =
        fraction.find_first_not_of('0') != std::string::npos;

    bool bump = false;
    switch (mode) {
    case Rounding::Floor:
        // Toward negative infinity, so it is the NEGATIVE side that grows:
        // floor(-2.5) is -3 and floor(2.5) is 2.
        bump = has_fraction && s < 0;
        break;
    case Rounding::Ceil:
        bump = has_fraction && s > 0;
        break;
    case Rounding::HalfAwayFromZero:
        bump = !fraction.empty() && fraction[0] >= '5';
        break;
    }

    BigInt mag = BigInt::from_digits(whole.data(), whole.size());
    if (bump)
        mag = BigInt::add(mag, BigInt::from_u64(1));
    return make(s, mag, 0);
}

int Number::compare(const Number &a, const Number &b)
{
    // The hot path: two small numbers at the same scale, which is every
    // `i < rounds` in every loop.
    if (!a.big_ && !b.big_ && a.exp_ == b.exp_)
        return a.sig_ < b.sig_ ? -1 : (a.sig_ > b.sig_ ? 1 : 0);

    const int sa = a.sign();
    const int sb = b.sign();
    if (sa != sb)
        return sa < sb ? -1 : 1;
    if (sa == 0)
        return 0;

    const int target = std::min(a.exp_, b.exp_);

    // Still small, just at different scales — `x < 2.5`. Aligning in a long
    // long keeps it allocation-free.
    if (!a.big_ && !b.big_) {
        long long left = 0;
        long long right = 0;
        if (scale_ll(a.sig_, a.exp_ - target, left) &&
            scale_ll(b.sig_, b.exp_ - target, right))
            return left < right ? -1 : (left > right ? 1 : 0);
    }

    const int c = BigInt::compare(a.scaled_magnitude(target),
                                  b.scaled_magnitude(target));
    return sa < 0 ? -c : c;
}

Number Number::add(const Number &a, const Number &b)
{
    // The hot path, and the reason the small representation exists: two
    // long longs and an overflow check, with no allocation and no atomic.
    if (!a.big_ && !b.big_) {
        const int target = std::min(a.exp_, b.exp_);
        long long left = 0;
        long long right = 0;
        long long sum = 0;
        if (scale_ll(a.sig_, a.exp_ - target, left) &&
            scale_ll(b.sig_, b.exp_ - target, right) &&
            !__builtin_add_overflow(left, right, &sum)) {
            Number out;
            out.sig_ = sum;
            out.exp_ = sum == 0 ? 0 : target;
            return out;
        }
    }

    const int sa = a.sign();
    const int sb = b.sign();
    if (sa == 0)
        return b;
    if (sb == 0)
        return a;

    // Align to the LOWER exponent, so no digit is ever discarded: this is what
    // makes 1e20 + 1 exact where a double loses the 1 entirely.
    const int target = std::min(a.exp_, b.exp_);
    const BigInt ma = a.scaled_magnitude(target);
    const BigInt mb = b.scaled_magnitude(target);

    if (sa == sb)
        return make(sa, BigInt::add(ma, mb), target);

    const int c = BigInt::compare(ma, mb);
    if (c == 0)
        return Number();
    if (c > 0)
        return make(sa, BigInt::sub(ma, mb), target);
    return make(sb, BigInt::sub(mb, ma), target);
}

Number Number::sub(const Number &a, const Number &b)
{
    return add(a, b.negated());
}

Number Number::mul(const Number &a, const Number &b)
{
    if (!a.big_ && !b.big_) {
        long long product = 0;
        const long long exponent = static_cast<long long>(a.exp_) + b.exp_;
        if (!__builtin_mul_overflow(a.sig_, b.sig_, &product) &&
            exponent >= INT_MIN && exponent <= INT_MAX) {
            Number out;
            out.sig_ = product;
            out.exp_ = product == 0 ? 0 : static_cast<int>(exponent);
            return out;
        }
    }
    return make(a.sign() * b.sign(), BigInt::mul(a.magnitude(), b.magnitude()),
                static_cast<long long>(a.exp_) + b.exp_);
}

Number Number::divide(const Number &a, const Number &b, int digits)
{
    // A zero divisor is a satellite error with a span attached, so the caller
    // has already rejected it; returning zero here is what happens if it did
    // not, and it is better than dividing by nothing.
    if (a.is_zero() || b.is_zero())
        return Number();

    if (digits < 1)
        digits = 1;
    if (digits > MAX_DIVISION_DIGITS)
        digits = MAX_DIVISION_DIGITS;

    const BigInt ma = a.magnitude();
    const BigInt mb = b.magnitude();

    // Scale the dividend until the quotient is guaranteed at least `digits` + 1
    // significant digits. The extra one is what the rounding decision is made
    // on, and it is why this is +1 rather than exact.
    long long scale = static_cast<long long>(digits) + 1 +
                      static_cast<long long>(mb.digit_count()) -
                      static_cast<long long>(ma.digit_count());
    if (scale < 0)
        scale = 0;

    BigInt quotient;
    BigInt remainder;
    BigInt::divmod(scale ? BigInt::mul_pow10(ma, static_cast<unsigned>(scale))
                         : ma,
                   mb, quotient, remainder);

    long long exponent = static_cast<long long>(a.exp_) - b.exp_ - scale;
    std::string qd = quotient.to_digits();

    // Exact divisions — every division by a power of ten, which is every
    // nanoseconds-to-seconds conversion — leave a zero remainder and keep every
    // digit. Only a non-terminating one is cut.
    if (!remainder.is_zero() && qd.size() > static_cast<size_t>(digits)) {
        const size_t keep = static_cast<size_t>(digits);
        const bool up = qd[keep] >= '5';
        exponent += static_cast<long long>(qd.size() - keep);
        qd.resize(keep);

        BigInt kept = BigInt::from_digits(qd.data(), qd.size());
        if (up)
            kept = BigInt::add(kept, BigInt::from_u64(1));
        qd = kept.to_digits();
    }

    // Strip trailing zeros before building the result, so an exact division
    // comes back in the small representation instead of carrying the scaling
    // around as a bignum for the rest of the program.
    size_t end = qd.size();
    while (end > 1 && qd[end - 1] == '0') {
        end--;
        exponent++;
    }
    qd.resize(end);

    return make(a.sign() * b.sign(),
                BigInt::from_digits(qd.data(), qd.size()), exponent);
}

Number Number::modulo(const Number &a, const Number &b)
{
    if (a.is_zero() || b.is_zero())
        return Number();

    // Truncating remainder, so the sign follows the DIVIDEND — the same rule
    // fmod has, and the one the evaluator's % already documented.
    const int target = std::min(a.exp_, b.exp_);
    BigInt quotient;
    BigInt remainder;
    BigInt::divmod(a.scaled_magnitude(target), b.scaled_magnitude(target),
                   quotient, remainder);
    return make(a.sign(), remainder, target);
}

std::string Number::to_string() const
{
    // The hot path: an integer in the small representation, which is every
    // counter and every small literal, and the one this is asked for a million
    // times in a printing loop.
    if (!big_ && exp_ == 0)
        return std::to_string(sig_);

    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (digits == "0")
        return "0";

    const std::string sign = is_negative() ? "-" : "";

    // How many digits sit before the decimal point. The value's magnitude is in
    // [10^(point-1), 10^point).
    const long long point = static_cast<long long>(digits.size()) + exponent;

    // §8.1.1 fixed the rule as "print the value", and set the notation boundary
    // at [1e-6, 1e21) because outside it "digits stop being informative — 1e308
    // in fixed notation is 309 characters". That reasoning was written when a
    // number was a double, where a 33-digit integer HAD no 33 informative
    // digits. It does now, so the boundary is restated in the terms the
    // reasoning was always really about:
    //
    //     switch to scientific when the PADDING ZEROS would outnumber the
    //     information, never when the information itself is long.
    //
    // 1e308 is one significant digit and 308 zeros, so it is still scientific.
    // 30! is 26 significant digits and 7 zeros, so it prints whole — which
    // under the old phrasing it would not have, and an exact type that renders
    // an exact integer as 2.6525285981219105863630848e+32 is hiding the answer
    // it went to the trouble of computing.
    //
    // Both thresholds are the old boundaries restated, so every value that
    // printed fixed before still does: 20 trailing zeros is 1e20, and 5 leading
    // zeros is 1e-6.
    constexpr long long MAX_TRAILING_ZEROS = 20;
    constexpr long long MAX_LEADING_ZEROS = 5;

    const long long width = static_cast<long long>(digits.size());
    const bool fixed = point <= 0 ? -point <= MAX_LEADING_ZEROS
                                  : point - width <= MAX_TRAILING_ZEROS;
    if (fixed) {
        if (point <= 0)
            return sign + "0." + std::string(static_cast<size_t>(-point), '0') +
                   digits;
        if (static_cast<size_t>(point) >= digits.size())
            return sign + digits +
                   std::string(static_cast<size_t>(point) - digits.size(), '0');
        return sign + digits.substr(0, static_cast<size_t>(point)) + "." +
               digits.substr(static_cast<size_t>(point));
    }

    std::string out = sign + digits.substr(0, 1);
    if (digits.size() > 1)
        out += "." + digits.substr(1);

    const long long e = point - 1;
    out += "e";
    out += e < 0 ? "-" : "+";
    out += std::to_string(e < 0 ? -e : e);
    return out;
}

} // namespace satellite
