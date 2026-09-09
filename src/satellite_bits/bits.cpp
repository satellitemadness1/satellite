// The bit run's five operations and the hex run's four. See
// satellite_bits/bits.hpp for what the two types are, why the bits are packed,
// and why hex HOLDS a bit run rather than deriving from one.

#include "satellite_bits/bits.hpp"

#include <string>

namespace satellite::bits {

namespace {

// How many bits go into one Number step. THE CONVERSION IS CHUNKED RATHER THAN
// BIT BY BIT, and the reason is the shape of the arithmetic: `shift_left` on a
// bignum is O(size), so folding one bit at a time over an n-bit run is O(n^2)
// -- a 4,000-bit literal would do four thousand shifts of a growing number.
// Taking 32 bits per step makes it n/32 of them. 32 and not 63 because the
// intermediate `chunk` must not overflow while it is being built and 32 leaves
// the question unaskable.
constexpr unsigned kChunkBits = 32;

} // namespace

bool parse_binary(std::string_view digits, BitRun &out)
{
    out.bits.clear();
    out.bits.reserve(digits.size());
    for (const char c : digits) {
        if (c != '0' && c != '1')
            return false;
        out.bits.push_back(c == '1');
    }
    return true;
}

std::string digits_of(const BitRun &run)
{
    std::string text;
    text.reserve(run.bits.size());
    for (const bool bit : run.bits)
        text.push_back(bit ? '1' : '0');
    return text;
}

std::string text_of(const BitRun &run)
{
    return "b" + digits_of(run);
}

Number value_of(const BitRun &run)
{
    Number total; // zero
    size_t at = 0;
    while (at < run.bits.size()) {
        const size_t take = run.bits.size() - at < kChunkBits
                                ? run.bits.size() - at
                                : kChunkBits;
        unsigned long long chunk = 0;
        for (size_t i = 0; i < take; i++)
            chunk = (chunk << 1) | (run.bits[at + i] ? 1ull : 0ull);
        // `total * 2^take + chunk` -- shift_left IS multiplication by a power
        // of two and is exact at every width (DESIGN §5.5's "no bit is ever
        // lost", which is a fact about the NUMBER and is why this loop needs
        // no carry of its own).
        total = Number::add(Number::shift_left(total, static_cast<unsigned>(take)),
                            Number::from_u64(chunk));
        at += take;
    }
    return total;
}

Number digits_as_number(const BitRun &run)
{
    // THE DIGITS, PARSED AS DECIMAL. A run of `0`s and `1`s is always a legal
    // decimal number, so the parse cannot fail -- but it is checked rather
    // than asserted, because a zero answer is a wrong answer a reader would
    // have to catch downstream, and this way an empty run answers zero and
    // nothing else can.
    Number value;
    const std::string text = digits_of(run);
    if (text.empty() || !Number::parse(text, value))
        return Number();
    return value;
}

bool to_bytes(const BitRun &run, std::string &out)
{
    if (run.bits.size() % 8 != 0)
        return false;
    out.clear();
    out.reserve(run.bits.size() / 8);
    for (size_t at = 0; at < run.bits.size(); at += 8) {
        unsigned char byte = 0;
        for (size_t i = 0; i < 8; i++)
            byte = static_cast<unsigned char>((byte << 1) |
                                              (run.bits[at + i] ? 1u : 0u));
        out.push_back(static_cast<char>(byte));
    }
    return true;
}

// ---------------------------------------------------------------------------

namespace {

// How many bits one hex digit is worth. NOT A TUNABLE -- 2^4 is 16 and there
// are sixteen hex digits, so this is the only value that makes the round trip
// exact, which is the property the whole type is built on.
constexpr size_t kBitsPerDigit = 4;

// One hex digit's value, or 16 when the character is not one. Sixteen and not
// -1 so the caller's check is a comparison against the radix rather than
// against a sentinel that has to be remembered.
unsigned digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return static_cast<unsigned>(c - '0');
    if (c >= 'a' && c <= 'f')
        return static_cast<unsigned>(c - 'a') + 10u;
    if (c >= 'A' && c <= 'F')
        return static_cast<unsigned>(c - 'A') + 10u;
    return 16u;
}

} // namespace

bool parse_hex(std::string_view digits, HexRun &out)
{
    out.bits.bits.clear();
    out.bits.bits.reserve(digits.size() * kBitsPerDigit);
    for (const char c : digits) {
        const unsigned value = digit_value(c);
        if (value >= 16u)
            return false;
        // MOST SIGNIFICANT BIT OF THE DIGIT FIRST, which is what keeps the
        // whole run in the order it was written: `xF0` must expand to
        // `b11110000` and not `b00001111`, and it is the same order the digit
        // is read back in below.
        for (size_t bit = kBitsPerDigit; bit-- > 0;)
            out.bits.bits.push_back(((value >> bit) & 1u) != 0);
    }
    return true;
}

std::string digits_of(const HexRun &run)
{
    static const char kDigits[] = "0123456789ABCDEF";
    std::string text;
    text.reserve(run.digits());
    // THE LOOP CANNOT LEAVE A PARTIAL DIGIT because the width is a multiple of
    // four by construction (bits.hpp's invariant) -- the `+ 4 <=` is what makes
    // that a fact of this loop rather than a fact somebody has to remember.
    for (size_t at = 0; at + kBitsPerDigit <= run.bits.bits.size();
         at += kBitsPerDigit) {
        unsigned value = 0;
        for (size_t i = 0; i < kBitsPerDigit; i++)
            value = (value << 1) | (run.bits.bits[at + i] ? 1u : 0u);
        text.push_back(kDigits[value]);
    }
    return text;
}

std::string text_of(const HexRun &run)
{
    return "x" + digits_of(run);
}

bool to_hex(const BitRun &run, HexRun &out)
{
    if (run.bits.size() % kBitsPerDigit != 0)
        return false;
    out.bits = run;
    return true;
}

} // namespace satellite::bits
