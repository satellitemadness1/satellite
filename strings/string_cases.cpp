// satellite-004/strings/string_cases.cpp -- the test harness for satellite_string.
// check_strings.py feeds it one case per line and compares every answer with
// Python's own strict UTF-8 codec. Not part of the interpreter.
//
//   U <hex bytes>        decode UTF-8     -> "ok <code points in hex>" or "bad <byte offset>"
//                        and, when ok, round-trip through UTF-8, bits and bits_to_cxx_str
//   C <hex code points>  encode to UTF-8  -> "ok <hex bytes>" or "bad <character index>"
//   B <bits text>        decode .sati bits -> "ok <code points in hex>" or "bad <offset>"
//   speed <megabytes>    time decoding and bit conversion of multilingual text

#include "satellite_string.hpp"
#include "../machine_codes.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace satellite004;

static std::string hex_bytes(const std::string &s)
{
    static const char *d = "0123456789abcdef";
    std::string h;
    for (unsigned char c : s) { h += d[c >> 4]; h += d[c & 15]; }
    return h;
}

static std::string code_points(const std::u32string &t)
{
    std::string h;
    char buffer[16];
    for (char32_t c : t) {
        std::snprintf(buffer, sizeof buffer, "%s%x", h.empty() ? "" : ",", (unsigned)c);
        h += buffer;
    }
    return h;
}

static int speed(long megabytes)
{
    // English, Russian, Chinese, Arabic, Hindi and two emoji, written as escapes.
    const std::string sample = "Hello, мир! 你好世界 "
                               "مرحبا नमस्ते "
                               "\U0001F30D\U0001F680 satellite.console.display ";
    std::string text;
    while (text.size() < (size_t)megabytes * 1000000)
        text += sample;
    std::u32string wide;
    std::string bits, back;
    size_t bad = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    utf8_to_char32(text, wide, bad);
    auto t1 = std::chrono::high_resolution_clock::now();
    char32_to_bits(wide, bits);
    auto t2 = std::chrono::high_resolution_clock::now();
    bits_to_cxx_str(bits, back, bad);
    auto t3 = std::chrono::high_resolution_clock::now();
    auto mbps = [&](auto a, auto b) { return (text.size() / 1e6) / std::chrono::duration<double>(b - a).count(); };
    std::printf("text: %.1f MB of UTF-8, %zu characters\n", text.size() / 1e6, wide.size());
    std::printf("  UTF-8 -> char32_t:        %7.0f MB/s   (in memory: %.1f MB, x%.2f the UTF-8)\n",
                mbps(t0, t1), wide.size() * 4 / 1e6, wide.size() * 4.0 / text.size());
    std::printf("  char32_t -> .sati bits:   %7.0f MB/s   (bit text: %.1f MB, x%.1f the UTF-8)\n",
                mbps(t1, t2), bits.size() / 1e6, (double)bits.size() / text.size());
    std::printf("  bits_to_cxx_str:          %7.0f MB/s   round trip identical: %s\n",
                mbps(t2, t3), back == text ? "yes" : "NO");
    return back == text ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc == 3 && std::string(argv[1]) == "speed")
        return speed(std::atol(argv[2]));
    std::string line;
    while (std::getline(std::cin, line)) {
        const char kind = line.empty() ? '?' : line[0];
        const std::string rest = line.size() > 2 ? line.substr(2) : "";
        size_t bad = 0;
        if (kind == 'U') {
            std::string bytes;
            for (size_t i = 0; i + 1 < rest.size(); i += 2)
                bytes += (char)std::strtoul(rest.substr(i, 2).c_str(), nullptr, 16);
            std::u32string wide;
            if (utf8_to_char32(bytes, wide, bad) != success) {
                std::cout << "bad " << bad << "\n";
                continue;
            }
            std::string utf8, bits, cxx;
            std::u32string again;
            size_t b2 = 0;
            const bool round = char32_to_utf8(wide, utf8, b2) == success && utf8 == bytes;
            char32_to_bits(wide, bits);
            const bool bits_round = bits_to_char32(bits, again, b2) == success && again == wide &&
                                    bits_to_cxx_str(bits, cxx, b2) == success && cxx == bytes;
            std::cout << "ok " << code_points(wide) << (round && bits_round ? "" : " ROUNDTRIP_FAIL") << "\n";
        } else if (kind == 'C') {
            std::u32string wide;
            size_t start = 0;
            while (start < rest.size()) {
                size_t comma = rest.find(',', start);
                if (comma == std::string::npos)
                    comma = rest.size();
                wide.push_back((char32_t)std::strtoul(rest.substr(start, comma - start).c_str(), nullptr, 16));
                start = comma + 1;
            }
            std::string utf8;
            if (char32_to_utf8(wide, utf8, bad) != success)
                std::cout << "bad " << bad << "\n";
            else
                std::cout << "ok " << hex_bytes(utf8) << "\n";
        } else if (kind == 'B') {
            std::u32string wide;
            if (bits_to_char32(rest, wide, bad) != success)
                std::cout << "bad " << bad << "\n";
            else
                std::cout << "ok " << code_points(wide) << "\n";
        } else {
            std::cout << "unknown case\n";
        }
    }
}
