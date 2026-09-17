// satellite/satellite_variable_string/string_race.cpp -- how fast satellite_string
// decodes and encodes, against plain C++ doing the same work.
//
//     build/string_race <megabytes> <source file>...
//
// THREE TEXTS of <megabytes> each: the source files repeated (ASCII source text;
// the race says how many bytes of it are not ASCII); mixed scripts that all fit 16
// bits (English, Russian, Greek, Chinese, Arabic, Hindi, and a line of the source
// between -- satellite's fast path); and the same with two emoji a line, so the
// string goes wide (satellite's slower path).
//
// THE CONTESTANTS, every one strict (a bad sequence is refused with its offset),
// from plain_conversions.hpp and plain_like_satellite.hpp unless named:
//   satellite  satellite_string::from_utf8 / to_utf8, with the author's table
//   (a)        plain UTF-8 <-> char16_t, no table: THE BAR. Several ways of writing
//              it, one of them satellite's own loops with the table taken out; the
//              fastest of them on each text is what satellite is held to.
//   (b)        strings/satellite_string.cpp's utf8_to_char32 / char32_to_utf8, the
//              32-bit strings satellite had before
//   (c)        plain UTF-8 <-> char32_t, no table, written the fastest (a) ways: the
//              same width as satellite's slower path, to show what four bytes a
//              character costs by itself
//
// FAIR BY CONSTRUCTION: the text is built while running from the files and the
// megabytes on the command line, so nothing can be worked out by the compiler.
// Each decode writes into a string kept from the run before (capacity kept, as a
// reused variable would); each encode returns a new std::string, except (b), whose
// interface fills one it is given (the string an encode replaces is freed inside
// the timing, the same for every contestant). Before any timing every contestant's
// answer is checked against the simplest code's, outside the timing. Every
// contestant then runs 7 times in one order and 7 in the reverse order, and its
// fastest run is its time.

#include "satellite_string.hpp"
#include "plain_conversions.hpp"
#include "plain_like_satellite.hpp"

#include "../../strings/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

using namespace satellite004;

namespace {

struct contestant {
    const char *name;
    char group;                 // 's' satellite, 'a', 'b' or 'c'
    std::function<void()> run;  // the work, and all that is timed
    std::function<bool()> right; // whether the last run's answer is right; never timed
    double best_seconds = 1e300;
};

void race(std::vector<contestant> &contestants)
{
    for (int order = 0; order < 2; order++)
        for (int run = 0; run < 7; run++)
            for (std::size_t k = 0; k < contestants.size(); k++) {
                contestant &c = contestants[order == 0 ? k : contestants.size() - 1 - k];
                const auto begin = std::chrono::steady_clock::now();
                c.run();
                const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
                c.best_seconds = std::min(c.best_seconds, seconds);
            }
}

double fastest(const std::vector<contestant> &contestants, char group)
{
    double best = 1e300;
    for (const contestant &c : contestants)
        if (c.group == group)
            best = std::min(best, c.best_seconds);
    return best;
}

// Answers false when any contestant's answer is wrong; nothing is timed then.
bool race_and_report(const char *what, std::vector<contestant> &contestants, std::size_t bytes)
{
    for (const contestant &c : contestants)
        if (c.run(), !c.right()) {
            std::printf("  %s: %s gave a WRONG answer; nothing timed\n", what, c.name);
            return false;
        }
    race(contestants);
    const double bar = fastest(contestants, 'a');
    std::printf("  %s (every answer checked first: all right)\n", what);
    for (const contestant &c : contestants)
        std::printf("    %-36s %8.1f ms %8.0f MB/s   x%.3f of the fastest (a)\n", c.name, c.best_seconds * 1e3,
                    bytes / 1e6 / c.best_seconds, c.best_seconds / bar);
    const double satellite = fastest(contestants, 's');
    std::printf("    => satellite is x%.3f of the fastest (a), x%.3f of the fastest (c), x%.3f of (b)\n", satellite / bar,
                satellite / fastest(contestants, 'c'), satellite / fastest(contestants, 'b'));
    return true;
}

bool race_text(const char *title, const std::string &text)
{
    std::size_t not_ascii = 0;
    for (unsigned char b : text)
        not_ascii += b >= 0x80;
    // The answers every contestant must give, from the simplest code.
    std::u16string units16;
    std::u32string units32;
    std::size_t bad = 0;
    if (plain_decode_push_back(text, units16, bad) != success || utf8_to_char32(text, units32, bad) != success) {
        std::printf("%s: the text is refused at byte %zu\n", title, bad);
        return false;
    }
    satellite_string satellite;
    std::u16string plain16;
    std::u32string plain32, wide;
    std::string back, encoded;
    signed long long int code = success;
    // One walk over the units, never code_at_unchecked(k): on a wide string that
    // walks from the front for every k, and the check would never finish.
    const auto satellite_right = [&] {
        bool right = satellite.size() == units32.size();
        std::size_t unit = 0, width = 0;
        for (std::size_t k = 0; right && k < units32.size(); k++, unit += width)
            right = satellite_string::unicode_of(satellite.code_at_unit(unit, width)) == units32[k];
        return right && unit == satellite.units();
    };
    satellite_string::from_utf8(text, satellite, bad);
    std::printf("%s: %.1f MB, %zu bytes not ASCII, %zu characters, satellite fast(): %s\n", title, text.size() / 1e6,
                not_ascii, units32.size(), satellite.fast() ? "yes" : "no");

    std::vector<contestant> decoders = {
        {"satellite from_utf8", 's', [&] { code = satellite_string::from_utf8(text, satellite, bad); }, [&] { return code == success && satellite_right(); }},
        {"(a) push_back", 'a', [&] { code = plain_decode_push_back(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) pointer, resize", 'a', [&] { code = plain_decode<char16_t, 1, false>(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) pointer, resize, ASCII 8", 'a', [&] { code = plain_decode<char16_t, 8, false>(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) pointer, no zeros", 'a', [&] { code = plain_decode<char16_t, 1, true>(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) pointer, no zeros, ASCII 8", 'a', [&] { code = plain_decode<char16_t, 8, true>(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) pointer, no zeros, ASCII 16", 'a', [&] { code = plain_decode<char16_t, 16, true>(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(a) satellite's loops, no table", 'a', [&] { code = plain_decode_like_satellite(text, plain16, bad); }, [&] { return code == success && plain16 == units16; }},
        {"(b) 32-bit utf8_to_char32", 'b', [&] { code = utf8_to_char32(text, wide, bad); }, [&] { return code == success && wide == units32; }},
        {"(c) char32_t, no zeros, ASCII 8", 'c', [&] { code = plain_decode<char32_t, 8, true>(text, plain32, bad); }, [&] { return code == success && plain32 == units32; }},
        {"(c) char32_t, no zeros, ASCII 16", 'c', [&] { code = plain_decode<char32_t, 16, true>(text, plain32, bad); }, [&] { return code == success && plain32 == units32; }},
        {"(c) satellite's loops, no table", 'c', [&] { code = plain_decode_like_satellite(text, plain32, bad); }, [&] { return code == success && plain32 == units32; }},
    };
    if (!race_and_report("decode UTF-8 ->", decoders, text.size()))
        return false;

    satellite_string::from_utf8(text, satellite, bad);      // the encoders' inputs
    plain16 = units16;
    plain32 = units32;
    std::vector<contestant> encoders = {
        {"satellite to_utf8", 's', [&] { encoded = satellite.to_utf8(); }, [&] { return encoded == text; }},
        {"(a) push_back", 'a', [&] { encoded = plain_encode_push_back(plain16); }, [&] { return encoded == text; }},
        {"(a) pointer, resize", 'a', [&] { encoded = plain_encode<char16_t, 1, 0>(plain16); }, [&] { return encoded == text; }},
        {"(a) pointer, resize, ASCII 16", 'a', [&] { encoded = plain_encode<char16_t, 16, 0>(plain16); }, [&] { return encoded == text; }},
        {"(a) pointer, no zeros", 'a', [&] { encoded = plain_encode<char16_t, 1, 1>(plain16); }, [&] { return encoded == text; }},
        {"(a) pointer, no zeros, ASCII 8", 'a', [&] { encoded = plain_encode<char16_t, 8, 1>(plain16); }, [&] { return encoded == text; }},
        {"(a) pointer, no zeros, ASCII 16", 'a', [&] { encoded = plain_encode<char16_t, 16, 1>(plain16); }, [&] { return encoded == text; }},
        {"(a) ASCII-first size, ASCII 8", 'a', [&] { encoded = plain_encode<char16_t, 8, 2>(plain16); }, [&] { return encoded == text; }},
        {"(a) ASCII-first size, ASCII 16", 'a', [&] { encoded = plain_encode<char16_t, 16, 2>(plain16); }, [&] { return encoded == text; }},
        {"(a) satellite's loops, no table", 'a', [&] { encoded = plain_encode_like_satellite(plain16); }, [&] { return encoded == text; }},
        {"(b) 32-bit char32_to_utf8", 'b', [&] { code = char32_to_utf8(plain32, back, bad); }, [&] { return code == success && back == text; }},
        {"(c) char32_t, no zeros, ASCII 16", 'c', [&] { encoded = plain_encode<char32_t, 16, 1>(plain32); }, [&] { return encoded == text; }},
        {"(c) char32_t, ASCII-first, ASCII 16", 'c', [&] { encoded = plain_encode<char32_t, 16, 2>(plain32); }, [&] { return encoded == text; }},
        {"(c) satellite's loops, no table", 'c', [&] { encoded = plain_encode_like_satellite(plain32); }, [&] { return encoded == text; }},
    };
    return race_and_report("encode -> UTF-8", encoders, text.size());
}

std::string repeat_to(const std::string &sample, std::size_t bytes)
{
    std::string text;
    text.reserve(bytes + sample.size());
    while (text.size() < bytes)
        text += sample;                                     // whole samples, so no character is cut
    return text;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <megabytes> <source file>...\n", argv[0]);
        return 1;
    }
    const std::size_t bytes = std::strtoull(argv[1], nullptr, 10) * 1000000;
    std::string source;
    for (int k = 2; k < argc; k++) {
        std::ifstream file(argv[k], std::ios::binary);
        if (!file) {
            std::fprintf(stderr, "cannot read %s\n", argv[k]);
            return 1;
        }
        source.append(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    // Written as escapes so this file stays ASCII: Russian, Greek, Chinese, Arabic
    // and Hindi words -- every character below U+FFFF.
    const std::string scripts = "The satellite reads \u043C\u0438\u0440, \u03BA\u03CC\u03C3\u03BC\u03BF\u03C2, "
                                "\u4F60\u597D\u4E16\u754C, \u0645\u0631\u062D\u0628\u0627 and "
                                "\u0928\u092E\u0938\u094D\u0924\u0947.\n";
    const std::string emoji = "\U0001F30D " + scripts.substr(0, scripts.size() - 1) + " \U0001F680\n";
    // A line of the source between script lines, cut where a character starts.
    std::size_t cut = std::min<std::size_t>(200, source.size());
    while (cut > 0 && cut < source.size() && (static_cast<unsigned char>(source[cut]) & 0xC0) == 0x80)
        cut--;
    const std::string code_line = source.substr(0, cut) + "\n";
    bool all_right = race_text("ASCII source text", repeat_to(source, bytes));
    all_right = race_text("mixed scripts, all 16-bit", repeat_to(scripts + code_line, bytes)) && all_right;
    all_right = race_text("mixed scripts with emoji (wide)", repeat_to(emoji + code_line, bytes)) && all_right;
    return all_right ? 0 : 1;
}
