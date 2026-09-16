// The satellite exact arbitrary-precision decimal number implementation.

#include "number.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace satellite {

// ---------------------------------------------------------------------------
// BigInt Implementation
// ---------------------------------------------------------------------------

BigInt::BigInt(uint64_t val)
{
    while (val > 0) {
        limbs.push_back(static_cast<uint32_t>(val % BASE));
        val /= BASE;
    }
}

BigInt BigInt::from_u64(uint64_t val)
{
    return BigInt(val);
}

BigInt BigInt::from_string(const std::string &digits)
{
    BigInt res;
    if (digits.empty() || digits == "0")
        return res;

    int len = static_cast<int>(digits.size());
    for (int i = len; i > 0; i -= DIGITS_PER_LIMB) {
        int start = std::max(0, i - DIGITS_PER_LIMB);
        std::string chunk = digits.substr(start, i - start);
        res.limbs.push_back(static_cast<uint32_t>(std::stoul(chunk)));
    }
    while (res.limbs.size() > 1 && res.limbs.back() == 0)
        res.limbs.pop_back();
    return res;
}

bool BigInt::is_zero() const
{
    return limbs.empty() || (limbs.size() == 1 && limbs[0] == 0);
}

int BigInt::compare(const BigInt &other) const
{
    if (limbs.size() != other.limbs.size())
        return limbs.size() < other.limbs.size() ? -1 : 1;
    for (int i = static_cast<int>(limbs.size()) - 1; i >= 0; --i) {
        if (limbs[i] != other.limbs[i])
            return limbs[i] < other.limbs[i] ? -1 : 1;
    }
    return 0;
}

BigInt BigInt::add(const BigInt &other) const
{
    BigInt res;
    size_t max_size = std::max(limbs.size(), other.limbs.size());
    uint64_t carry = 0;
    for (size_t i = 0; i < max_size || carry; ++i) {
        uint64_t sum = carry;
        if (i < limbs.size()) sum += limbs[i];
        if (i < other.limbs.size()) sum += other.limbs[i];
        res.limbs.push_back(static_cast<uint32_t>(sum % BASE));
        carry = sum / BASE;
    }
    return res;
}

BigInt BigInt::sub(const BigInt &other) const
{
    BigInt res;
    int64_t borrow = 0;
    for (size_t i = 0; i < limbs.size(); ++i) {
        int64_t diff = static_cast<int64_t>(limbs[i]) - borrow;
        if (i < other.limbs.size()) diff -= other.limbs[i];
        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        res.limbs.push_back(static_cast<uint32_t>(diff));
    }
    while (res.limbs.size() > 1 && res.limbs.back() == 0)
        res.limbs.pop_back();
    if (res.limbs.size() == 1 && res.limbs[0] == 0)
        res.limbs.clear();
    return res;
}

BigInt BigInt::mul_u32(uint32_t v) const
{
    BigInt res;
    if (v == 0 || is_zero()) return res;
    uint64_t carry = 0;
    for (size_t i = 0; i < limbs.size() || carry; ++i) {
        uint64_t prod = carry;
        if (i < limbs.size()) prod += static_cast<uint64_t>(limbs[i]) * v;
        res.limbs.push_back(static_cast<uint32_t>(prod % BASE));
        carry = prod / BASE;
    }
    return res;
}

BigInt BigInt::mul(const BigInt &other) const
{
    if (is_zero() || other.is_zero()) return BigInt();
    BigInt res;
    res.limbs.resize(limbs.size() + other.limbs.size(), 0);
    for (size_t i = 0; i < limbs.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < other.limbs.size() || carry; ++j) {
            uint64_t cur = res.limbs[i + j] + carry;
            if (j < other.limbs.size())
                cur += static_cast<uint64_t>(limbs[i]) * other.limbs[j];
            res.limbs[i + j] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
    }
    while (res.limbs.size() > 1 && res.limbs.back() == 0)
        res.limbs.pop_back();
    return res;
}

std::string BigInt::to_digits() const
{
    if (is_zero()) return "0";
    std::string s = std::to_string(limbs.back());
    for (int i = static_cast<int>(limbs.size()) - 2; i >= 0; --i) {
        std::string limb_str = std::to_string(limbs[i]);
        s.append(DIGITS_PER_LIMB - limb_str.size(), '0');
        s.append(limb_str);
    }
    return s;
}

bool BigInt::to_u64(uint64_t &out) const
{
    if (is_zero()) { out = 0; return true; }
    if (limbs.size() > 3) return false;
    uint64_t res = 0;
    uint64_t mult = 1;
    for (size_t i = 0; i < limbs.size(); ++i) {
        if (i == 2 && limbs[i] > 18) return false;
        res += static_cast<uint64_t>(limbs[i]) * mult;
        if (i + 1 < limbs.size()) mult *= BASE;
    }
    out = res;
    return true;
}

// ---------------------------------------------------------------------------
// Number Implementation
// ---------------------------------------------------------------------------

BigInt Number::get_magnitude() const
{
    if (big_) return *big_;
    return BigInt::from_u64(static_cast<uint64_t>(sig_));
}

bool Number::is_zero() const
{
    if (big_) return big_->is_zero();
    return sig_ == 0;
}

void Number::normalize()
{
    if (is_zero()) {
        positive = true;
        sig_ = 0;
        exp_ = 0;
        big_ = nullptr;
    }
}

Number Number::from_u64(uint64_t val)
{
    Number n;
    n.positive = true;
    n.exp_ = 0;
    if (val <= static_cast<uint64_t>(LLONG_MAX)) {
        n.sig_ = static_cast<long long>(val);
    } else {
        n.sig_ = 0;
        n.big_ = std::make_shared<const BigInt>(BigInt::from_u64(val));
    }
    return n;
}

Number Number::negated() const
{
    if (is_zero()) return *this;
    Number res = *this;
    res.positive = !positive;
    return res;
}

Number Number::abs() const
{
    Number res = *this;
    res.positive = true;
    return res;
}

bool Number::is_integer() const
{
    if (exp_ >= 0) return true;
    // Fractional exponent: check if decimal representation ends with enough zeros
    std::string dig = get_magnitude().to_digits();
    int trailing_needed = -exp_;
    if (static_cast<int>(dig.size()) <= trailing_needed) return is_zero();
    for (int i = 0; i < trailing_needed; ++i) {
        if (dig[dig.size() - 1 - i] != '0') return false;
    }
    return true;
}

bool Number::to_integer(long long &out) const
{
    if (!is_integer()) return false;
    BigInt mag = get_magnitude();
    uint64_t uval = 0;
    if (!mag.to_u64(uval)) return false;
    if (exp_ > 0) {
        for (int i = 0; i < exp_; ++i) {
            if (uval > UINT64_MAX / 10) return false;
            uval *= 10;
        }
    } else if (exp_ < 0) {
        for (int i = 0; i < -exp_; ++i) uval /= 10;
    }
    if (positive) {
        if (uval > static_cast<uint64_t>(LLONG_MAX)) return false;
        out = static_cast<long long>(uval);
    } else {
        if (uval > static_cast<uint64_t>(LLONG_MAX) + 1) return false;
        if (uval == static_cast<uint64_t>(LLONG_MAX) + 1) out = LLONG_MIN;
        else out = -static_cast<long long>(uval);
    }
    return true;
}

int Number::compare(const Number &a, const Number &b)
{
    if (a.is_zero() && b.is_zero()) return 0;
    if (a.is_zero()) return b.positive ? -1 : 1;
    if (b.is_zero()) return a.positive ? 1 : -1;
    if (a.positive != b.positive) return a.positive ? 1 : -1;

    // Both same sign: scale magnitudes to same exponent
    BigInt mag_a = a.get_magnitude();
    BigInt mag_b = b.get_magnitude();
    int min_exp = std::min(a.exp_, b.exp_);
    for (int i = 0; i < a.exp_ - min_exp; ++i) mag_a = mag_a.mul_u32(10);
    for (int i = 0; i < b.exp_ - min_exp; ++i) mag_b = mag_b.mul_u32(10);

    int cmp = mag_a.compare(mag_b);
    return a.positive ? cmp : -cmp;
}

Number Number::add(const Number &other) const
{
    if (is_zero()) return other;
    if (other.is_zero()) return *this;

    BigInt mag_a = get_magnitude();
    BigInt mag_b = other.get_magnitude();
    int min_exp = std::min(exp_, other.exp_);
    for (int i = 0; i < exp_ - min_exp; ++i) mag_a = mag_a.mul_u32(10);
    for (int i = 0; i < other.exp_ - min_exp; ++i) mag_b = mag_b.mul_u32(10);

    if (positive == other.positive) {
        BigInt res_mag = mag_a.add(mag_b);
        return make_from_bigint(positive, res_mag, min_exp);
    } else {
        int cmp = mag_a.compare(mag_b);
        if (cmp == 0) return Number(0);
        if (cmp > 0) {
            BigInt res_mag = mag_a.sub(mag_b);
            return make_from_bigint(positive, res_mag, min_exp);
        } else {
            BigInt res_mag = mag_b.sub(mag_a);
            return make_from_bigint(other.positive, res_mag, min_exp);
        }
    }
}

Number Number::sub(const Number &other) const
{
    return add(other.negated());
}

Number Number::mul(const Number &other) const
{
    if (is_zero() || other.is_zero()) return Number(0);
    BigInt mag = get_magnitude().mul(other.get_magnitude());
    bool pos = (positive == other.positive);
    return make_from_bigint(pos, mag, exp_ + other.exp_);
}

Number Number::div(const Number &other, int digits) const
{
    if (other.is_zero()) return Number(0); // Handled by runtime error
    if (is_zero()) return Number(0);

    BigInt num = get_magnitude();
    BigInt den = other.get_magnitude();
    int exp_adj = exp_ - other.exp_;

    // Scale num up to produce quotient with required precision digits
    int scale_digits = std::clamp(digits, 1, MAX_DIVISION_DIGITS) + 4;
    for (int i = 0; i < scale_digits; ++i) num = num.mul_u32(10);
    exp_adj -= scale_digits;

    // Simple digit division
    BigInt quotient;
    std::string num_str = num.to_digits();
    std::string q_str;
    BigInt cur;
    for (char c : num_str) {
        cur = cur.mul_u32(10).add(BigInt::from_u64(c - '0'));
        int d = 0;
        while (cur.compare(den) >= 0) {
            cur = cur.sub(den);
            d++;
        }
        q_str.push_back(char('0' + d));
    }
    quotient = BigInt::from_string(q_str);
    bool pos = (positive == other.positive);
    return make_from_bigint(pos, quotient, exp_adj);
}

Number Number::make_from_bigint(bool pos, const BigInt &mag, int exponent)
{
    Number n;
    n.positive = pos;
    n.exp_ = exponent;
    uint64_t uval = 0;
    if (mag.to_u64(uval) && uval <= static_cast<uint64_t>(LLONG_MAX)) {
        n.sig_ = static_cast<long long>(uval);
        n.big_ = nullptr;
    } else {
        n.sig_ = 0;
        n.big_ = std::make_shared<const BigInt>(mag);
    }
    n.normalize();
    return n;
}

std::string Number::to_string() const
{
    if (is_zero()) return "0";
    std::string digits = get_magnitude().to_digits();
    std::string out;
    if (!positive) out.push_back('-');

    if (exp_ == 0) {
        out.append(digits);
    } else if (exp_ > 0) {
        out.append(digits);
        out.append(exp_, '0');
    } else {
        int dec_pos = static_cast<int>(digits.size()) + exp_;
        if (dec_pos > 0) {
            out.append(digits.substr(0, dec_pos));
            out.push_back('.');
            out.append(digits.substr(dec_pos));
        } else {
            out.append("0.");
            out.append(-dec_pos, '0');
            out.append(digits);
        }
        while (out.back() == '0' && out.find('.') != std::string::npos) out.pop_back();
        if (out.back() == '.') out.pop_back();
    }
    return out;
}

bool Number::parse(const std::string &text, Number &out)
{
    if (text.empty()) return false;
    size_t i = 0;
    bool pos = true;
    if (text[i] == '-') { pos = false; i++; }
    else if (text[i] == '+') { i++; }

    std::string digits;
    int exp_val = 0;
    bool has_dot = false;
    int frac_digits = 0;

    while (i < text.size() && (std::isdigit(text[i]) || text[i] == '.')) {
        if (text[i] == '.') {
            if (has_dot) return false;
            has_dot = true;
        } else {
            digits.push_back(text[i]);
            if (has_dot) frac_digits++;
        }
        i++;
    }
    if (digits.empty()) return false;
    exp_val -= frac_digits;

    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        i++;
        int exp_sign = 1;
        if (i < text.size() && text[i] == '-') { exp_sign = -1; i++; }
        else if (i < text.size() && text[i] == '+') { i++; }
        int extra_exp = 0;
        while (i < text.size() && std::isdigit(text[i])) {
            extra_exp = extra_exp * 10 + (text[i] - '0');
            i++;
        }
        exp_val += exp_sign * extra_exp;
    }
    if (i != text.size()) return false;

    BigInt mag = BigInt::from_string(digits);
    out = make_from_bigint(pos, mag, exp_val);
    return true;
}

} // namespace satellite

