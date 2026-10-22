// The console's lines. See console_lines.hpp.

#include "console_lines.hpp"

#include <utility>

namespace satellite004::prompt {

namespace {

// HOW MANY BYTES THE WHOLE UTF-8 CHARACTER STARTING AT `at` TAKES, 2 to 4, or 0 when no whole
// character starts there. Table 3-7 row by row, as conversion_loops.hpp's decode reads it: the second
// byte's range depends on the first, which is what refuses an overlong form, a surrogate and anything
// above U+10FFFF; every byte after it is 80 to BF. A byte below 80 is ASCII, and is the caller's.
//
//   first byte   second byte   then
//   C2..DF       80..BF        -
//   E0           A0..BF        80..BF
//   E1..EC       80..BF        80..BF
//   ED           80..9F        80..BF          (no surrogates)
//   EE..EF       80..BF        80..BF
//   F0           90..BF        80..BF 80..BF
//   F1..F3       80..BF        80..BF 80..BF
//   F4           80..8F        80..BF 80..BF   (nothing above U+10FFFF)
std::size_t whole_character_at(const unsigned char *bytes, std::size_t size, std::size_t at)
{
    const unsigned char first = bytes[at];
    const auto between = [&](std::size_t after, unsigned char low, unsigned char high) {
        return at + after < size && bytes[at + after] >= low && bytes[at + after] <= high;
    };
    if (first >= 0xC2 && first <= 0xDF)
        return between(1, 0x80, 0xBF) ? 2 : 0;
    if (first >= 0xE0 && first <= 0xEF) {
        const unsigned char low = first == 0xE0 ? 0xA0 : 0x80, high = first == 0xED ? 0x9F : 0xBF;
        return between(1, low, high) && between(2, 0x80, 0xBF) ? 3 : 0;
    }
    if (first >= 0xF0 && first <= 0xF4) {
        const unsigned char low = first == 0xF0 ? 0x90 : 0x80, high = first == 0xF4 ? 0x8F : 0xBF;
        return between(1, low, high) && between(2, 0x80, 0xBF) && between(3, 0x80, 0xBF) ? 4 : 0;
    }
    return 0;   // 80..BF, a continuation with nothing before it; C0 and C1, always overlong; F5..FF, never UTF-8
}

} // namespace

std::string what_is_kept(const std::string &entered)
{
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(entered.data());
    const std::size_t size = entered.size();
    std::string kept;
    kept.reserve(size);
    for (std::size_t at = 0; at < size;) {
        const unsigned char first = bytes[at];
        // ASCII: kept unless it is U+0000 to U+001F, or U+007F.
        if (first < 0x80) {
            if (first >= 0x20 && first != 0x7F)
                kept += static_cast<char>(first);
            ++at;
            continue;
        }
        const std::size_t length = whole_character_at(bytes, size, at);
        if (length == 0) {   // not the start of a whole character: this byte alone goes
            ++at;
            continue;
        }
        // U+0080 TO U+009F, THE OTHER CONTROL CHARACTERS, are C2 80 to C2 9F -- U+009B among them, which a
        // terminal takes as ESC [ -- and are whole characters that go all the same.
        if (!(length == 2 && first == 0xC2 && bytes[at + 1] <= 0x9F))
            kept.append(entered, at, length);
        at += length;
    }
    return kept;
}

std::string ConsoleLines::add(const std::string &entered)
{
    std::string kept = what_is_kept(entered);
    // NOTHING LEFT IN IT CAN BE REFUSED: from_utf8 refuses only what is not Table 3-7, and what_is_kept
    // keeps nothing else. console_files_cases holds the two together, every two-byte line and a million
    // more.
    satellite_string line;
    std::size_t bad = 0;
    (void)satellite_string::from_utf8(kept, line, bad);
    // A STOP FOR UP AND DOWN (KEY-4) unless it is only spaces -- the trim the session gave History,
    // with its tabs and CRs already gone in what_is_kept -- or the last stop again.
    const bool blank = kept.find_first_not_of(' ') == std::string::npos;
    const std::lock_guard<std::mutex> held(lock_);
    if (!blank && (stops_.empty() || lines_[stops_.back()] != line))
        stops_.push_back(lines_.size());
    lines_.push_back(std::move(line));
    return kept;
}

std::size_t ConsoleLines::stops() const
{
    const std::lock_guard<std::mutex> held(lock_);
    return stops_.size();
}

std::string ConsoleLines::stop_at(std::size_t stop) const
{
    const std::lock_guard<std::mutex> held(lock_);
    return stop < stops_.size() ? lines_[stops_[stop]].to_utf8() : std::string();
}

std::size_t ConsoleLines::size() const
{
    const std::lock_guard<std::mutex> held(lock_);
    return lines_.size();
}

satellite_string ConsoleLines::at(std::size_t index) const
{
    const std::lock_guard<std::mutex> held(lock_);
    return index < lines_.size() ? lines_[index] : satellite_string();
}

} // namespace satellite004::prompt
