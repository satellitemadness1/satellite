// satellite/satellite_test/speed_values_twins.cpp -- the C++ twins of satellite.test.speed()'s values,
// arithmetic and statements sections: the same work as each speed_<name>.satl, statement for statement, in
// plain C++ built -O2. Each answers exactly the line its program displays, without the newline.
//
// Every loop carries a value that depends on every pass and has no shortcut (a remainder of a running
// product), so the compiler cannot replace a loop by its answer: it has to run it.
//
// Where satl's values are wider than a long long, the twin says what it uses instead:
// unsigned __int128 where every value fits 128 bits, 64-bit limbs where they do not, a double where
// every value is a whole number of sixteenths (exact in binary), and whole numbers of 0.0001% for
// percentages.
//
// No main, no globals, nothing printed: this file is compiled into satl.

#include "speed_values_twins.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

namespace satellite004 {

namespace {
using u128 = unsigned __int128;

// base 10 text of an unsigned __int128 (std::to_string has no overload for it)
std::string text_of(u128 value)
{
    if (value == 0) {
        return "0";
    }
    std::string reversed;
    while (value != 0) {
        reversed += static_cast<char>('0' + static_cast<int>(value % 10));
        value /= 10;
    }
    return std::string(reversed.rbegin(), reversed.rend());
}

// a float as satl shows it: every digit after the point it has, and at least one (2.0, 12.5, -0.125).
// The values given here are whole numbers of sixteenths, so four places hold them exactly.
std::string float_text(double value)
{
    std::string text = std::format("{:.4f}", value);
    while (text.back() == '0' && text[text.size() - 2] != '.') {
        text.pop_back();
    }
    return text;
}

// a percentage held as a whole number of 0.0001%, as satl shows it: 50%, 12.5%, 0.0625%
std::string percent_text(long long units)
{
    std::string sign = units < 0 ? "-" : "";
    long long size = units < 0 ? -units : units;
    std::string text = sign + std::to_string(size / 10000);
    std::string places = std::format("{:04}", size % 10000);
    while (!places.empty() && places.back() == '0') {
        places.pop_back();
    }
    if (!places.empty()) {
        text += "." + places;
    }
    return text + "%";
}

std::string bool_text(bool value)
{
    return value ? "true" : "false";
}

// a whole number of any size, 0 or more: 64-bit limbs, the lowest first, no zero limbs on top
using limbs = std::vector<std::uint64_t>;

// n = n * times + plus
void multiply_add(limbs &n, std::uint64_t times, std::uint64_t plus)
{
    u128 carry = plus;
    for (std::uint64_t &limb : n) {
        carry += static_cast<u128>(limb) * times;
        limb = static_cast<std::uint64_t>(carry);
        carry >>= 64;
    }
    if (carry != 0) {
        n.push_back(static_cast<std::uint64_t>(carry));
    }
}

// n = n / by, answering n % by
std::uint64_t divide(limbs &n, std::uint64_t by)
{
    u128 rest = 0;
    for (std::size_t k = n.size(); k-- > 0;) {
        rest = (rest << 64) | n[k];
        n[k] = static_cast<std::uint64_t>(rest / by);
        rest %= by;
    }
    while (!n.empty() && n.back() == 0) {
        n.pop_back();
    }
    return static_cast<std::uint64_t>(rest);
}

// n = n + m
void add(limbs &n, const limbs &m)
{
    if (n.size() < m.size()) {
        n.resize(m.size(), 0);
    }
    u128 carry = 0;
    for (std::size_t k = 0; k < n.size(); ++k) {
        carry += n[k];
        if (k < m.size()) {
            carry += m[k];
        }
        n[k] = static_cast<std::uint64_t>(carry);
        carry >>= 64;
    }
    if (carry != 0) {
        n.push_back(static_cast<std::uint64_t>(carry));
    }
}

// n % by, n unchanged
std::uint64_t remainder(const limbs &n, std::uint64_t by)
{
    u128 rest = 0;
    for (std::size_t k = n.size(); k-- > 0;) {
        rest = ((rest << 64) | n[k]) % by;
    }
    return static_cast<std::uint64_t>(rest);
}

// how many base 10 digits n has
std::size_t decimal_digits(limbs n)
{
    const std::uint64_t nineteen_digits = 10000000000000000000ULL; // 10^19
    if (n.empty()) {
        return 1;
    }
    std::size_t digits = 0;
    while (n.size() > 1 || n[0] >= nineteen_digits) {
        divide(n, nineteen_digits);
        digits += 19;
    }
    return digits + std::to_string(n[0]).size();
}

// a fraction, compared by what it is worth (bottom > 0)
struct fraction {
    long long top;
    long long bottom;
};

bool operator<(fraction a, fraction b)
{
    return a.top * b.bottom < b.top * a.bottom;
}

bool operator>(fraction a, fraction b)
{
    return b < a;
}

bool operator==(fraction a, fraction b)
{
    return a.top * b.bottom == b.top * a.bottom;
}
} // namespace

std::string speed_while(long long passes)
{
    long long i = 0;
    long long h = 0;
    while (i < passes) {
        i = i + 1;
        h = (h * 3 + i) % 1000003;
    }
    return std::to_string(i) + " " + std::to_string(h);
}

std::string speed_for(long long passes)
{
    long long turns = 0;
    long long h = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        for (long long j = 0; j < 10; j++) {
            h = (h * 7 + i + j) % 1000003;
        }
        turns = turns + 10;
    }
    return std::to_string(turns) + " " + std::to_string(h);
}

std::string speed_if_else(long long passes)
{
    long long x = 547311173;
    long long a = 0;
    long long b = 0;
    long long c = 0;
    long long d = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        if (x % 4 == 0) {
            a = a + 1;
        } else if (x % 4 == 1) {
            b = b + 1;
        } else if (x % 4 == 2) {
            c = c + 1;
        } else {
            d = d + 1;
        }
    }
    return std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c) + " " + std::to_string(d);
}

std::string speed_compare(long long passes)
{
    long long x = 547311173;
    long long y = 0;
    long long less = 0;
    long long more = 0;
    long long at_most = 0;
    long long at_least = 0;
    long long same = 0;
    long long differ = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        y = x;
        x = x * 48271 % 2147483647;
        if (x < y) {
            less = less + 1;
        }
        if (x > y) {
            more = more + 1;
        }
        if (x % 1000 <= 499) {
            at_most = at_most + 1;
        }
        if (x % 1000 >= 750) {
            at_least = at_least + 1;
        }
        if (x % 16 == y % 16) {
            same = same + 1;
        }
        if (x % 3 != 0) {
            differ = differ + 1;
        }
    }
    return std::to_string(less) + " " + std::to_string(more) + " " + std::to_string(at_most) + " "
        + std::to_string(at_least) + " " + std::to_string(same) + " " + std::to_string(differ);
}

std::string speed_arithmetic(long long passes)
{
    long long x = 547311173;
    long long q = 0;
    long long r = 0;
    long long s = 0;
    long long t = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        q = x / 1000;
        r = x % 1000;
        long long digit = r % 10;
        s = s + q - r * 3 + digit * digit; // (r % 10) ^ 2
        t = (t * 31 + q / 7 - r) % 1000000007;
    }
    return std::to_string(x) + " " + std::to_string(s) + " " + std::to_string(t);
}

std::string speed_big_numbers(long long passes)
{
    const u128 m = (static_cast<u128>(1) << 100) - 15; // 1267650600228229401496703205361
    const u128 two_to_64 = static_cast<u128>(1) << 64; // 18446744073709551616
    u128 x = 547311173;
    u128 s = 0;
    u128 p = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = (x * 1000003 + static_cast<u128>(i)) % m;
        s = s + x;
        p = (p + x / two_to_64 * (x % two_to_64)) % m;
    }
    return text_of(x) + " " + text_of(s) + " " + text_of(p);
}

std::string speed_huge_numbers(long long passes)
{
    limbs x{1};
    for (int k = 0; k < 111; k++) // 547311173 ^ 111
    {
        multiply_add(x, 547311173, 0);
    }
    limbs y;
    for (long long i = 0; i < passes; i = i + 1) {
        multiply_add(x, 1000003, static_cast<std::uint64_t>(i)); // x = x * 1000003 + i
        divide(x, 1000033);                                      // x = x / 1000033
        add(y, x);                                               // y = y + x
    }
    return std::to_string(remainder(x, 1000000007)) + " " + std::to_string(remainder(y, 1000000007)) + " "
        + std::to_string(decimal_digits(x)) + " " + std::to_string(decimal_digits(y));
}

std::string speed_trillion(long long passes)
{
    const u128 trillion = 1000000000000;
    u128 mix = 547311173;
    u128 sum = 0;
    u128 big = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        mix = (mix * trillion + static_cast<u128>(i)) % 1000000000039;
        sum = sum + mix + trillion;
        big = big + mix * trillion;
    }
    return text_of(mix) + " " + text_of(sum) + " " + text_of(big);
}

std::string speed_float(long long passes)
{
    double f = 0.25;
    double s = 0.0;
    double t = 0.0;
    for (long long i = 0; i < passes; i = i + 1) {
        f = std::fmod(f * 3 + 0.75, 1000.75);
        s = s + f / 4 - 0.5;
        t = t + f * 0.5;
    }
    return float_text(f) + " " + float_text(s) + " " + float_text(t);
}

std::string speed_percent(long long passes)
{
    // a percentage is held as a whole number of 0.0001%, so 100% is 1000000
    const long long hundred_percent = 1000000;
    long long x = 547311173;
    long long base = 0;
    long long total = 0;
    long long rate = 0;
    long long parts = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        base = x % 1000 * 8;
        total = total + base * 250000 / hundred_percent;                     // base * 25%
        total = total + base * (hundred_percent + 125000) / hundred_percent; // base + 12.5%
        total = total - base * (hundred_percent - 500000) / hundred_percent; // base - 50%
        total = total + base * hundred_percent / 500000;                     // base / 50%
        rate = rate + 5000;                                                  // rate + 0.5%
        if (rate > hundred_percent) {
            rate = rate - hundred_percent;
        }
        parts = parts + rate * 500000 / hundred_percent / 4; // rate * 50% / 4
    }
    return std::to_string(total) + " " + percent_text(rate) + " " + percent_text(parts);
}

std::string speed_hex_binary(long long passes)
{
    const long long multiplier = 0x10DCD;
    const long long step = 0b1011;
    const long long mask = 0x0000FFFF;
    long long x = 0x2A;
    long long sum = 0b0;
    long long high = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = (x * multiplier + step) % 0xFFFFFFFB;
        sum = sum + x % mask - 0b101;
        if (x > 0x7FFFFFFF) {
            high = high + 0b1;
        }
    }
    return std::to_string(x) + " " + std::to_string(sum) + " " + std::to_string(high);
}

std::string speed_bool(long long passes)
{
    long long x = 547311173;
    bool odd = false;
    bool flip = false;
    bool run = true;
    long long count = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        odd = x % 2 == 1;
        flip = flip != odd;
        run = !(run == flip);
        if (run) {
            count = count + 1;
        }
    }
    return std::to_string(count) + " " + bool_text(odd) + " " + bool_text(flip) + " " + bool_text(run);
}

std::string speed_conversions(long long passes)
{
    long long x = 547311173;
    std::string text;
    long long back = 0;
    long long total = 0;
    long long sizes = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        text = std::to_string(x);
        back = std::stoll(text);
        total = total + back + (1 + std::stoll(text));
        sizes = sizes
            + static_cast<long long>(text.size() + std::format("{:X}", x).size() + std::format("{:b}", x).size());
    }
    return std::to_string(total) + " " + std::to_string(sizes);
}

std::string speed_fraction(long long passes)
{
    long long x = 547311173;
    fraction f{0, 1};
    long long below = 0;
    long long above = 0;
    long long same = 0;
    for (long long i = 0; i < passes; i = i + 1) {
        x = x * 48271 % 2147483647;
        f = fraction{x % 100, 1};
        if (f < fraction{100, 3}) {
            below = below + 1;
        }
        if (f > fraction{200, 3}) {
            above = above + 1;
        }
        if (f == fraction{50, 2}) {
            same = same + 1;
        }
    }
    return std::to_string(below) + " " + std::to_string(above) + " " + std::to_string(same);
}

} // namespace satellite004
